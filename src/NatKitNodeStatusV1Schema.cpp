#include <libnatkit-core.hpp>

#include <cstdio>
#include <cstring>
#include <sstream>

namespace nat {
namespace core {

const std::string NatKitNodeStatusV1Schema::name = "NatKitNodeStatusV1";
const size_t NatKitNodeStatusV1Schema::kWireSize = 192;
const size_t NatKitNodeStatusV1Schema::kLegacyWireSize = 168;

namespace {

// ⚠️ Explicit little-endian readers. The wire bytes are a memcpy of the
// firmware's struct built for xtensa; decoding them by declaring a matching
// struct here would make correctness depend on this compiler agreeing about
// padding and endianness. These offsets are pinned by a test that decodes a
// frame captured from the live rig.
uint8_t rd8(const uint8_t* b, size_t o) { return b[o]; }
int8_t rdi8(const uint8_t* b, size_t o) { return static_cast<int8_t>(b[o]); }

uint16_t rd16(const uint8_t* b, size_t o) {
  return static_cast<uint16_t>(b[o]) | (static_cast<uint16_t>(b[o + 1]) << 8);
}

uint32_t rd32(const uint8_t* b, size_t o) {
  return static_cast<uint32_t>(b[o]) | (static_cast<uint32_t>(b[o + 1]) << 8) |
         (static_cast<uint32_t>(b[o + 2]) << 16) |
         (static_cast<uint32_t>(b[o + 3]) << 24);
}

uint64_t rd64(const uint8_t* b, size_t o) {
  uint64_t v = 0;
  for (size_t i = 0; i < 8; ++i) {
    v |= static_cast<uint64_t>(b[o + i]) << (8 * i);
  }
  return v;
}

void wr16(std::vector<uint8_t>& b, size_t o, uint16_t v) {
  b[o] = static_cast<uint8_t>(v & 0xff);
  b[o + 1] = static_cast<uint8_t>((v >> 8) & 0xff);
}

void wr32(std::vector<uint8_t>& b, size_t o, uint32_t v) {
  for (size_t i = 0; i < 4; ++i) b[o + i] = static_cast<uint8_t>((v >> (8 * i)) & 0xff);
}

void wr64(std::vector<uint8_t>& b, size_t o, uint64_t v) {
  for (size_t i = 0; i < 8; ++i) b[o + i] = static_cast<uint8_t>((v >> (8 * i)) & 0xff);
}

// Offsets of UplinkNodeStatus (natKit-IMU/firmware-idf/main/uplink.hpp), with
// SyncState (espnow_link.hpp, 88 bytes) inlined at 48. The two 4-byte holes at 44
// and elsewhere are alignment padding the firmware's compiler inserts; they are
// named here so a future reader does not mistake them for spare fields.
namespace off {
constexpr size_t kDeviceId = 0;
constexpr size_t kMac = 8;
constexpr size_t kLeafNoiseFloorDbm = 14;
constexpr size_t kReserved0 = 15;
constexpr size_t kDataFrames = 16;
constexpr size_t kSeqGaps = 20;
constexpr size_t kSeqDuplicates = 24;
constexpr size_t kSeqRestarts = 28;
constexpr size_t kHeartbeats = 32;
// 36..39 padding (last_seen_us is 8-aligned)
constexpr size_t kLastSeenUs = 40;
// --- SyncState begins at 48 ---
constexpr size_t kSyncDeviceId = 48;
constexpr size_t kSyncEpoch = 56;
// 60..63 padding
constexpr size_t kSyncRefLocalUs = 64;
constexpr size_t kSyncRefOffsetUs = 72;
constexpr size_t kSyncSkewPpb = 80;
constexpr size_t kSyncResidualRmsNs = 84;
constexpr size_t kSyncPeakResidualNs = 88;
// 92..95 padding
constexpr size_t kSyncLastBeaconLocalUs = 96;
constexpr size_t kBeaconsSeen = 104;
constexpr size_t kBeaconsMissed = 108;
constexpr size_t kPairsUsed = 112;
constexpr size_t kPairsOrphaned = 116;
constexpr size_t kOutliersRejected = 120;
constexpr size_t kEpochChanges = 124;
constexpr size_t kMacSpreadUs = 128;
constexpr size_t kSamplesUsed = 132;
constexpr size_t kQuality = 134;
constexpr size_t kImplausibleResiduals = 135;
// --- SyncState ends at 136 ---
constexpr size_t kSyncValid = 136;
constexpr size_t kRssiLast = 137;
constexpr size_t kRssiBest = 138;
constexpr size_t kRssiWorst = 139;
constexpr size_t kRssiSeen = 140;
constexpr size_t kLeafScanChannel = 141;
constexpr size_t kLeafRssiOfPrimary = 142;
constexpr size_t kLeafTxPowerQuarterDbm = 143;
constexpr size_t kLeafFramesBuilt = 144;
constexpr size_t kLeafFramesDropped = 148;
constexpr size_t kLeafSendFailures = 152;
constexpr size_t kLeafChannelHops = 156;
constexpr size_t kPublishNoSync = 160;
constexpr size_t kPublishNoShift = 164;
// --- added by TEC-NATKIT-52; present only in a 192-byte frame ---
constexpr size_t kProbeErrorSumUs = 168;
constexpr size_t kProbeErrorSumSq = 176;
constexpr size_t kProbeErrorCount = 184;
// 188..191 padding (reserved_probe)
}  // namespace off

}  // namespace

bool NatKitNodeStatusV1Schema::isSerializationTypeSupported(
    const SerializationType type) const {
  return type == SerializationType::Binary || type == SerializationType::Json;
}

std::string NatKitNodeStatusV1Schema::getName() const { return name; }

// The primary's clock is the one axis these share, so last_seen_us is the
// timestamp the time model should use.
uint64_t NatKitNodeStatusV1Schema::getTimestampUs() const { return lastSeenUs; }

Optional<NatKitNodeStatusV1Schema> NatKitNodeStatusV1Schema::decodeBinary(
    const std::vector<uint8_t>& message) {
  // ⚠️ One of TWO exact sizes, and nothing in between. A short read would decode
  // the leading fields fine and invent the rest, which is worse than refusing --
  // the plausible half is what gets quoted. But the fleet is flashed one board at
  // a time, so the pre-TEC-NATKIT-52 size stays valid: refusing it would blank the
  // panel for every leaf not yet done.
  if (message.size() != kWireSize && message.size() != kLegacyWireSize) {
    return Optional<NatKitNodeStatusV1Schema>();
  }
  const uint8_t* b = message.data();
  NatKitNodeStatusV1Schema s;
  s.deviceId = rd64(b, off::kDeviceId);
  std::memcpy(s.mac, b + off::kMac, 6);
  {
    char macbuf[18];
    std::snprintf(macbuf, sizeof(macbuf), "%02x:%02x:%02x:%02x:%02x:%02x",
                  s.mac[0], s.mac[1], s.mac[2], s.mac[3], s.mac[4], s.mac[5]);
    s.macText = macbuf;
  }
  s.leafNoiseFloorDbm = rdi8(b, off::kLeafNoiseFloorDbm);
  s.dataFrames = rd32(b, off::kDataFrames);
  s.seqGaps = rd32(b, off::kSeqGaps);
  s.seqDuplicates = rd32(b, off::kSeqDuplicates);
  s.seqRestarts = rd32(b, off::kSeqRestarts);
  s.heartbeats = rd32(b, off::kHeartbeats);
  s.lastSeenUs = rd64(b, off::kLastSeenUs);
  s.syncDeviceId = rd64(b, off::kSyncDeviceId);
  s.syncEpoch = rd32(b, off::kSyncEpoch);
  s.syncRefLocalUs = rd64(b, off::kSyncRefLocalUs);
  s.syncRefOffsetUs = static_cast<int64_t>(rd64(b, off::kSyncRefOffsetUs));
  s.syncSkewPpb = static_cast<int32_t>(rd32(b, off::kSyncSkewPpb));
  s.syncResidualRmsNs = rd32(b, off::kSyncResidualRmsNs);
  s.syncPeakResidualNs = rd32(b, off::kSyncPeakResidualNs);
  s.syncLastBeaconLocalUs = rd64(b, off::kSyncLastBeaconLocalUs);
  s.beaconsSeen = rd32(b, off::kBeaconsSeen);
  s.beaconsMissed = rd32(b, off::kBeaconsMissed);
  s.pairsUsed = rd32(b, off::kPairsUsed);
  s.pairsOrphaned = rd32(b, off::kPairsOrphaned);
  s.outliersRejected = rd32(b, off::kOutliersRejected);
  s.epochChanges = rd32(b, off::kEpochChanges);
  s.macSpreadUs = rd32(b, off::kMacSpreadUs);
  s.samplesUsed = rd16(b, off::kSamplesUsed);
  s.quality = rd8(b, off::kQuality);
  s.implausibleResiduals = rd8(b, off::kImplausibleResiduals);
  s.syncValid = rd8(b, off::kSyncValid);
  s.rssiLast = rdi8(b, off::kRssiLast);
  s.rssiBest = rdi8(b, off::kRssiBest);
  s.rssiWorst = rdi8(b, off::kRssiWorst);
  s.rssiSeen = rd8(b, off::kRssiSeen);
  s.leafScanChannel = rd8(b, off::kLeafScanChannel);
  s.leafRssiOfPrimary = rdi8(b, off::kLeafRssiOfPrimary);
  s.leafTxPowerQuarterDbm = rd8(b, off::kLeafTxPowerQuarterDbm);
  s.leafFramesBuilt = rd32(b, off::kLeafFramesBuilt);
  s.leafFramesDropped = rd32(b, off::kLeafFramesDropped);
  s.leafSendFailures = rd32(b, off::kLeafSendFailures);
  s.leafChannelHops = rd32(b, off::kLeafChannelHops);
  s.publishNoSync = rd32(b, off::kPublishNoSync);
  s.publishNoShift = rd32(b, off::kPublishNoShift);
  s.wireSize = message.size();
  // ⚠️ The flag, not the values, is what a reader must consult. Leaving the sums
  // at zero and saying nothing would read as "the probe measured nothing".
  s.hasProbeSums = message.size() == kWireSize;
  if (s.hasProbeSums) {
    s.probeErrorSumUs = static_cast<int64_t>(rd64(b, off::kProbeErrorSumUs));
    s.probeErrorSumSq = rd64(b, off::kProbeErrorSumSq);
    s.probeErrorCount = rd32(b, off::kProbeErrorCount);
  }
  return Optional<NatKitNodeStatusV1Schema>(s);
}

std::vector<uint8_t> NatKitNodeStatusV1Schema::encodeBinary() const {
  // ⚠️ Emit the size this record CAME FROM, so decode->encode is byte-identical
  // for a legacy frame too. Always emitting the new size would make the round trip
  // lossy in the one direction that matters: a captured frame from the deployed
  // firmware is the only fixture there is until the fleet is flashed.
  const size_t size = wireSize == kLegacyWireSize ? kLegacyWireSize : kWireSize;
  std::vector<uint8_t> b(size, 0);
  wr64(b, off::kDeviceId, deviceId);
  std::memcpy(b.data() + off::kMac, mac, 6);
  b[off::kLeafNoiseFloorDbm] = static_cast<uint8_t>(leafNoiseFloorDbm);
  wr32(b, off::kDataFrames, dataFrames);
  wr32(b, off::kSeqGaps, seqGaps);
  wr32(b, off::kSeqDuplicates, seqDuplicates);
  wr32(b, off::kSeqRestarts, seqRestarts);
  wr32(b, off::kHeartbeats, heartbeats);
  wr64(b, off::kLastSeenUs, lastSeenUs);
  wr64(b, off::kSyncDeviceId, syncDeviceId);
  wr32(b, off::kSyncEpoch, syncEpoch);
  wr64(b, off::kSyncRefLocalUs, syncRefLocalUs);
  wr64(b, off::kSyncRefOffsetUs, static_cast<uint64_t>(syncRefOffsetUs));
  wr32(b, off::kSyncSkewPpb, static_cast<uint32_t>(syncSkewPpb));
  wr32(b, off::kSyncResidualRmsNs, syncResidualRmsNs);
  wr32(b, off::kSyncPeakResidualNs, syncPeakResidualNs);
  wr64(b, off::kSyncLastBeaconLocalUs, syncLastBeaconLocalUs);
  wr32(b, off::kBeaconsSeen, beaconsSeen);
  wr32(b, off::kBeaconsMissed, beaconsMissed);
  wr32(b, off::kPairsUsed, pairsUsed);
  wr32(b, off::kPairsOrphaned, pairsOrphaned);
  wr32(b, off::kOutliersRejected, outliersRejected);
  wr32(b, off::kEpochChanges, epochChanges);
  wr32(b, off::kMacSpreadUs, macSpreadUs);
  wr16(b, off::kSamplesUsed, samplesUsed);
  b[off::kQuality] = quality;
  b[off::kImplausibleResiduals] = implausibleResiduals;
  b[off::kSyncValid] = syncValid;
  b[off::kRssiLast] = static_cast<uint8_t>(rssiLast);
  b[off::kRssiBest] = static_cast<uint8_t>(rssiBest);
  b[off::kRssiWorst] = static_cast<uint8_t>(rssiWorst);
  b[off::kRssiSeen] = rssiSeen;
  b[off::kLeafScanChannel] = leafScanChannel;
  b[off::kLeafRssiOfPrimary] = static_cast<uint8_t>(leafRssiOfPrimary);
  b[off::kLeafTxPowerQuarterDbm] = leafTxPowerQuarterDbm;
  wr32(b, off::kLeafFramesBuilt, leafFramesBuilt);
  wr32(b, off::kLeafFramesDropped, leafFramesDropped);
  wr32(b, off::kLeafSendFailures, leafSendFailures);
  wr32(b, off::kLeafChannelHops, leafChannelHops);
  wr32(b, off::kPublishNoSync, publishNoSync);
  wr32(b, off::kPublishNoShift, publishNoShift);
  if (size == kWireSize) {
    wr64(b, off::kProbeErrorSumUs, static_cast<uint64_t>(probeErrorSumUs));
    wr64(b, off::kProbeErrorSumSq, probeErrorSumSq);
    wr32(b, off::kProbeErrorCount, probeErrorCount);
  }
  return b;
}

std::string NatKitNodeStatusV1Schema::toJson() const {
  std::ostringstream o;
  char macbuf[18];
  std::snprintf(macbuf, sizeof(macbuf), "%02x:%02x:%02x:%02x:%02x:%02x",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  o << "{\"device_id\":" << deviceId
    << ",\"mac\":\"" << macbuf << "\""
    << ",\"leaf_noise_floor_dbm\":" << static_cast<int>(leafNoiseFloorDbm)
    << ",\"data_frames\":" << dataFrames
    << ",\"seq_gaps\":" << seqGaps
    << ",\"seq_duplicates\":" << seqDuplicates
    << ",\"seq_restarts\":" << seqRestarts
    << ",\"heartbeats\":" << heartbeats
    << ",\"last_seen_us\":" << lastSeenUs
    << ",\"sync\":{\"epoch\":" << syncEpoch
    << ",\"ref_local_us\":" << syncRefLocalUs
    << ",\"ref_offset_us\":" << syncRefOffsetUs
    << ",\"skew_ppb\":" << syncSkewPpb
    << ",\"residual_rms_ns\":" << syncResidualRmsNs
    << ",\"peak_residual_ns\":" << syncPeakResidualNs
    << ",\"last_beacon_local_us\":" << syncLastBeaconLocalUs
    << ",\"beacons_seen\":" << beaconsSeen
    << ",\"beacons_missed\":" << beaconsMissed
    << ",\"pairs_used\":" << pairsUsed
    << ",\"pairs_orphaned\":" << pairsOrphaned
    << ",\"outliers_rejected\":" << outliersRejected
    << ",\"epoch_changes\":" << epochChanges
    << ",\"mac_spread_us\":" << macSpreadUs
    << ",\"samples_used\":" << samplesUsed
    << ",\"quality\":" << static_cast<int>(quality)
    << ",\"implausible_residuals\":" << static_cast<int>(implausibleResiduals)
    << ",\"valid\":" << (syncValid ? "true" : "false") << "}"
    << ",\"rssi_last\":" << static_cast<int>(rssiLast)
    << ",\"rssi_best\":" << static_cast<int>(rssiBest)
    << ",\"rssi_worst\":" << static_cast<int>(rssiWorst)
    << ",\"rssi_seen\":" << static_cast<int>(rssiSeen)
    << ",\"leaf_scan_channel\":" << static_cast<int>(leafScanChannel)
    << ",\"leaf_rssi_of_primary\":" << static_cast<int>(leafRssiOfPrimary)
    << ",\"leaf_tx_power_quarter_dbm\":" << static_cast<int>(leafTxPowerQuarterDbm)
    << ",\"leaf_frames_built\":" << leafFramesBuilt
    << ",\"leaf_frames_dropped\":" << leafFramesDropped
    << ",\"leaf_send_failures\":" << leafSendFailures
    << ",\"leaf_channel_hops\":" << leafChannelHops
    << ",\"publish_no_sync\":" << publishNoSync
    << ",\"publish_no_shift\":" << publishNoShift
    << ",\"has_probe_sums\":" << (hasProbeSums ? "true" : "false")
    << ",\"probe_error_sum_us\":" << probeErrorSumUs
    << ",\"probe_error_sum_sq\":" << probeErrorSumSq
    << ",\"probe_error_count\":" << probeErrorCount
    << "}";
  return o.str();
}

std::string NatKitNodeStatusV1Schema::toString() const { return toJson(); }

std::unique_ptr<message_t> NatKitNodeStatusV1Schema::encodeToBytes(
    const SerializationType& type) const {
  // ⚠️ No std::make_unique: libnatkit-core builds as C++11 (-std=c++11), a
  // constraint inherited from being shared with embedded targets.
  if (type == SerializationType::Binary) {
    return std::unique_ptr<message_t>(new message_t(encodeBinary()));
  }
  if (type == SerializationType::Json) {
    const std::string json = toJson();
    return std::unique_ptr<message_t>(new message_t(json.begin(), json.end()));
  }
  return nullptr;
}

Optional<std::shared_ptr<Schema>> NatKitNodeStatusV1Schema::tryDecode(
    const std::vector<uint8_t>& message,
    const SerializationType& type) const {
  if (type != SerializationType::Binary) {
    return Optional<std::shared_ptr<Schema>>();
  }
  auto decoded = decodeBinary(message);
  if (!decoded.has_value()) {
    return Optional<std::shared_ptr<Schema>>();
  }
  return Optional<std::shared_ptr<Schema>>(
      std::make_shared<NatKitNodeStatusV1Schema>(decoded.value()));
}

void NatKitNodeStatusV1Schema::registerWithRegistry(Registry &registry) {
    const decoder_t decoder = [](const message_t &message,
                                 const SerializationType &type) {
        if (type != SerializationType::Binary) {
            return Optional<std::unique_ptr<Schema>>{};
        }
        auto decoded = decodeBinary(message);
        if (!decoded.has_value()) {
            return Optional<std::unique_ptr<Schema>>{};
        }
        std::unique_ptr<Schema> converted(new NatKitNodeStatusV1Schema(decoded.value()));
        return Optional<std::unique_ptr<Schema>>{std::move(converted)};
    };
    registry.registerDecoder(name, SerializationType::Binary, decoder);
}

}  // namespace core
}  // namespace nat
