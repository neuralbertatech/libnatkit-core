#include <libnatkit-core.hpp>

#include <sstream>

namespace nat {
namespace core {

const std::string NatKitPrimaryStatusV1Schema::name = "NatKitPrimaryStatusV1";
const size_t NatKitPrimaryStatusV1Schema::kWireSize = 144;

namespace {

// ⚠️ Same reasoning as NatKitNodeStatusV1Schema: the wire bytes are a memcpy of
// the firmware's struct, so every field is read at an explicit offset in
// explicit little-endian rather than by declaring a struct here and trusting
// this compiler to lay it out identically. The offsets below were validated
// against a 144-byte frame captured from the live rig before this file existed.
uint8_t rd8(const uint8_t* b, size_t o) { return b[o]; }
int8_t rdi8(const uint8_t* b, size_t o) { return static_cast<int8_t>(b[o]); }

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

void wr32(std::vector<uint8_t>& b, size_t o, uint32_t v) {
  for (size_t i = 0; i < 4; ++i) b[o + i] = static_cast<uint8_t>((v >> (8 * i)) & 0xff);
}

void wr64(std::vector<uint8_t>& b, size_t o, uint64_t v) {
  for (size_t i = 0; i < 8; ++i) b[o + i] = static_cast<uint8_t>((v >> (8 * i)) & 0xff);
}

// Offsets of UplinkPrimaryStatus (natKit-IMU/firmware-idf/main/uplink.hpp).
// Unlike the node status there is no nested struct here, but there are two holes
// a reader should not mistake for spare capacity: bytes 86..87 are the
// firmware's declared `reserved[2]`, and 140..143 is tail padding the 8-byte
// alignment of the u64 members forces onto a 140-byte body.
namespace off {
constexpr size_t kDeviceId = 0;
constexpr size_t kUptimeUs = 8;
constexpr size_t kEpoch = 16;
constexpr size_t kFreeHeap = 20;
constexpr size_t kMinFreeHeap = 24;
constexpr size_t kNodesKnown = 28;
constexpr size_t kNodesRejected = 32;
constexpr size_t kUnknownPackets = 36;
constexpr size_t kFramesQueued = 40;
constexpr size_t kFramesSent = 44;
constexpr size_t kFramesDropped = 48;
constexpr size_t kWriteTimeouts = 52;
constexpr size_t kBytesSent = 56;
constexpr size_t kCoherenceTypicalUs = 64;
constexpr size_t kCoherenceBoundUs = 68;
constexpr size_t kCoherenceWorstUs = 72;
constexpr size_t kCoherenceSamples = 76;
constexpr size_t kCoherenceQuality = 80;
constexpr size_t kCoherenceMeasured = 81;
constexpr size_t kRegistrySealed = 82;
constexpr size_t kNoiseFloorDbm = 83;
constexpr size_t kChipTempC = 84;
constexpr size_t kChipTempErr = 85;
// 86..87 reserved[2] — spare bytes the firmware declares, not padding
constexpr size_t kCommandsReceived = 88;
constexpr size_t kCommandsRelayed = 92;
constexpr size_t kCommandsMalformed = 96;
constexpr size_t kCommandsUnknownDevice = 100;
constexpr size_t kCommandsSendFailed = 104;
constexpr size_t kCommandSubscriptions = 108;
constexpr size_t kCommandAnswersReceived = 112;
constexpr size_t kCommandAnswersPublished = 116;
constexpr size_t kCommandAnswersDuplicate = 120;
constexpr size_t kCommandsDelivered = 124;
constexpr size_t kCommandRetransmits = 128;
constexpr size_t kCommandsUndelivered = 132;
constexpr size_t kResetReason = 136;
}  // namespace off

}  // namespace

bool NatKitPrimaryStatusV1Schema::isSerializationTypeSupported(
    const SerializationType type) const {
  return type == SerializationType::Binary || type == SerializationType::Json;
}

std::string NatKitPrimaryStatusV1Schema::getName() const { return name; }

// The hub reports its own monotonic clock, which is the axis every leaf's
// last_seen_us is expressed on, so it is the timestamp for the time model.
uint64_t NatKitPrimaryStatusV1Schema::getTimestampUs() const { return uptimeUs; }

Optional<NatKitPrimaryStatusV1Schema> NatKitPrimaryStatusV1Schema::decodeBinary(
    const std::vector<uint8_t>& message) {
  // ⚠️ Exact size or nothing — see NatKitNodeStatusV1Schema::decodeBinary.
  if (message.size() != kWireSize) {
    return Optional<NatKitPrimaryStatusV1Schema>();
  }
  const uint8_t* b = message.data();
  NatKitPrimaryStatusV1Schema s;
  s.deviceId = rd64(b, off::kDeviceId);
  s.uptimeUs = rd64(b, off::kUptimeUs);
  s.epoch = rd32(b, off::kEpoch);
  s.freeHeap = rd32(b, off::kFreeHeap);
  s.minFreeHeap = rd32(b, off::kMinFreeHeap);
  s.nodesKnown = rd32(b, off::kNodesKnown);
  s.nodesRejected = rd32(b, off::kNodesRejected);
  s.unknownPackets = rd32(b, off::kUnknownPackets);
  s.framesQueued = rd32(b, off::kFramesQueued);
  s.framesSent = rd32(b, off::kFramesSent);
  s.framesDropped = rd32(b, off::kFramesDropped);
  s.writeTimeouts = rd32(b, off::kWriteTimeouts);
  s.bytesSent = rd64(b, off::kBytesSent);
  s.coherenceTypicalUs = rd32(b, off::kCoherenceTypicalUs);
  s.coherenceBoundUs = rd32(b, off::kCoherenceBoundUs);
  s.coherenceWorstUs = rd32(b, off::kCoherenceWorstUs);
  s.coherenceSamples = rd32(b, off::kCoherenceSamples);
  s.coherenceQuality = rd8(b, off::kCoherenceQuality);
  s.coherenceMeasured = rd8(b, off::kCoherenceMeasured);
  s.registrySealed = rd8(b, off::kRegistrySealed);
  s.noiseFloorDbm = rdi8(b, off::kNoiseFloorDbm);
  s.chipTempC = rdi8(b, off::kChipTempC);
  s.chipTempErr = rd8(b, off::kChipTempErr);
  s.commandsReceived = rd32(b, off::kCommandsReceived);
  s.commandsRelayed = rd32(b, off::kCommandsRelayed);
  s.commandsMalformed = rd32(b, off::kCommandsMalformed);
  s.commandsUnknownDevice = rd32(b, off::kCommandsUnknownDevice);
  s.commandsSendFailed = rd32(b, off::kCommandsSendFailed);
  s.commandSubscriptions = rd32(b, off::kCommandSubscriptions);
  s.commandAnswersReceived = rd32(b, off::kCommandAnswersReceived);
  s.commandAnswersPublished = rd32(b, off::kCommandAnswersPublished);
  s.commandAnswersDuplicate = rd32(b, off::kCommandAnswersDuplicate);
  s.commandsDelivered = rd32(b, off::kCommandsDelivered);
  s.commandRetransmits = rd32(b, off::kCommandRetransmits);
  s.commandsUndelivered = rd32(b, off::kCommandsUndelivered);
  s.resetReason = rd32(b, off::kResetReason);
  return Optional<NatKitPrimaryStatusV1Schema>(s);
}

std::vector<uint8_t> NatKitPrimaryStatusV1Schema::encodeBinary() const {
  std::vector<uint8_t> b(kWireSize, 0);
  wr64(b, off::kDeviceId, deviceId);
  wr64(b, off::kUptimeUs, uptimeUs);
  wr32(b, off::kEpoch, epoch);
  wr32(b, off::kFreeHeap, freeHeap);
  wr32(b, off::kMinFreeHeap, minFreeHeap);
  wr32(b, off::kNodesKnown, nodesKnown);
  wr32(b, off::kNodesRejected, nodesRejected);
  wr32(b, off::kUnknownPackets, unknownPackets);
  wr32(b, off::kFramesQueued, framesQueued);
  wr32(b, off::kFramesSent, framesSent);
  wr32(b, off::kFramesDropped, framesDropped);
  wr32(b, off::kWriteTimeouts, writeTimeouts);
  wr64(b, off::kBytesSent, bytesSent);
  wr32(b, off::kCoherenceTypicalUs, coherenceTypicalUs);
  wr32(b, off::kCoherenceBoundUs, coherenceBoundUs);
  wr32(b, off::kCoherenceWorstUs, coherenceWorstUs);
  wr32(b, off::kCoherenceSamples, coherenceSamples);
  b[off::kCoherenceQuality] = coherenceQuality;
  b[off::kCoherenceMeasured] = coherenceMeasured;
  b[off::kRegistrySealed] = registrySealed;
  b[off::kNoiseFloorDbm] = static_cast<uint8_t>(noiseFloorDbm);
  b[off::kChipTempC] = static_cast<uint8_t>(chipTempC);
  b[off::kChipTempErr] = chipTempErr;
  wr32(b, off::kCommandsReceived, commandsReceived);
  wr32(b, off::kCommandsRelayed, commandsRelayed);
  wr32(b, off::kCommandsMalformed, commandsMalformed);
  wr32(b, off::kCommandsUnknownDevice, commandsUnknownDevice);
  wr32(b, off::kCommandsSendFailed, commandsSendFailed);
  wr32(b, off::kCommandSubscriptions, commandSubscriptions);
  wr32(b, off::kCommandAnswersReceived, commandAnswersReceived);
  wr32(b, off::kCommandAnswersPublished, commandAnswersPublished);
  wr32(b, off::kCommandAnswersDuplicate, commandAnswersDuplicate);
  wr32(b, off::kCommandsDelivered, commandsDelivered);
  wr32(b, off::kCommandRetransmits, commandRetransmits);
  wr32(b, off::kCommandsUndelivered, commandsUndelivered);
  wr32(b, off::kResetReason, resetReason);
  return b;
}

std::string NatKitPrimaryStatusV1Schema::toJson() const {
  std::ostringstream o;
  o << "{\"device_id\":" << deviceId
    << ",\"uptime_us\":" << uptimeUs
    << ",\"epoch\":" << epoch
    << ",\"free_heap\":" << freeHeap
    << ",\"min_free_heap\":" << minFreeHeap
    << ",\"nodes_known\":" << nodesKnown
    << ",\"nodes_rejected\":" << nodesRejected
    << ",\"unknown_packets\":" << unknownPackets
    << ",\"frames_queued\":" << framesQueued
    << ",\"frames_sent\":" << framesSent
    << ",\"frames_dropped\":" << framesDropped
    << ",\"write_timeouts\":" << writeTimeouts
    << ",\"bytes_sent\":" << bytesSent
    << ",\"coherence_typical_us\":" << coherenceTypicalUs
    << ",\"coherence_bound_us\":" << coherenceBoundUs
    << ",\"coherence_worst_us\":" << coherenceWorstUs
    << ",\"coherence_samples\":" << coherenceSamples
    << ",\"coherence_quality\":" << static_cast<int>(coherenceQuality)
    << ",\"coherence_measured\":" << static_cast<int>(coherenceMeasured)
    << ",\"registry_sealed\":" << static_cast<int>(registrySealed)
    << ",\"noise_floor_dbm\":" << static_cast<int>(noiseFloorDbm)
    << ",\"chip_temp_c\":" << static_cast<int>(chipTempC)
    << ",\"chip_temp_err\":" << static_cast<int>(chipTempErr)
    << ",\"commands_received\":" << commandsReceived
    << ",\"commands_relayed\":" << commandsRelayed
    << ",\"commands_malformed\":" << commandsMalformed
    << ",\"commands_unknown_device\":" << commandsUnknownDevice
    << ",\"commands_send_failed\":" << commandsSendFailed
    << ",\"command_subscriptions\":" << commandSubscriptions
    << ",\"command_answers_received\":" << commandAnswersReceived
    << ",\"command_answers_published\":" << commandAnswersPublished
    << ",\"command_answers_duplicate\":" << commandAnswersDuplicate
    << ",\"commands_delivered\":" << commandsDelivered
    << ",\"command_retransmits\":" << commandRetransmits
    << ",\"commands_undelivered\":" << commandsUndelivered
    << ",\"reset_reason\":" << resetReason
    << "}";
  return o.str();
}

std::string NatKitPrimaryStatusV1Schema::toString() const { return toJson(); }

std::unique_ptr<message_t> NatKitPrimaryStatusV1Schema::encodeToBytes(
    const SerializationType& type) const {
  // ⚠️ No std::make_unique: libnatkit-core builds as C++11.
  if (type == SerializationType::Binary) {
    return std::unique_ptr<message_t>(new message_t(encodeBinary()));
  }
  if (type == SerializationType::Json) {
    const std::string json = toJson();
    return std::unique_ptr<message_t>(new message_t(json.begin(), json.end()));
  }
  return nullptr;
}

Optional<std::shared_ptr<Schema>> NatKitPrimaryStatusV1Schema::tryDecode(
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
      std::make_shared<NatKitPrimaryStatusV1Schema>(decoded.value()));
}

void NatKitPrimaryStatusV1Schema::registerWithRegistry(Registry &registry) {
    const decoder_t decoder = [](const message_t &message,
                                 const SerializationType &type) {
        if (type != SerializationType::Binary) {
            return Optional<std::unique_ptr<Schema>>{};
        }
        auto decoded = decodeBinary(message);
        if (!decoded.has_value()) {
            return Optional<std::unique_ptr<Schema>>{};
        }
        std::unique_ptr<Schema> converted(new NatKitPrimaryStatusV1Schema(decoded.value()));
        return Optional<std::unique_ptr<Schema>>{std::move(converted)};
    };
    registry.registerDecoder(name, SerializationType::Binary, decoder);
}

}  // namespace core
}  // namespace nat
