#include <libnatkit-core.hpp>

namespace nat {
namespace core {

const std::string NatKitNodeStatusV1Descriptor::name = "NatKitNodeStatusV1Descriptor";
const uint16_t NatKitNodeStatusV1Descriptor::descriptorVersion = 1;

namespace {

SchemaFieldDescriptor counter(const std::string &id, const std::string &label,
                             const std::string &description) {
  return SchemaFieldDescriptor(id, label, FieldValueType::Uint32, description);
}

SchemaFieldDescriptor dbm(const std::string &id, const std::string &label,
                          const std::string &description) {
  return SchemaFieldDescriptor(id, label, FieldValueType::Int16, description, "dBm");
}

// The leaf's clock fit as the primary holds it. Nested rather than flattened
// because it IS a separate object on the wire (SyncState), and flattening it here
// would hide that a field belongs to the fit rather than to delivery.
SchemaFieldDescriptor buildSyncField() {
  return SchemaFieldDescriptor(
      "sync", "Clock fit", FieldValueType::Object,
      "The leaf's clock fit, as the primary holds it", std::string{}, false, {},
      {
          SchemaFieldDescriptor("epoch", "Epoch", FieldValueType::Uint32,
                                "Fit epoch; a change means the fit was rebuilt"),
          SchemaFieldDescriptor("ref_local_us", "Anchor", FieldValueType::Uint64,
                                "Fit anchor in the leaf's own clock", "us"),
          // ⚠️ Float64, not because these are floats but because FieldValueType
          // has no signed 32/64-bit member. Exact at these magnitudes (a double is
          // exact to 2^53; offsets run to ~1e9 us and skew to ~1e5 ppb) -- and
          // Int16 would silently WRAP skew_ppb, which is routinely +45000 against
          // an Int16 ceiling of 32767.
          SchemaFieldDescriptor("ref_offset_us", "Offset at anchor",
                                FieldValueType::Float64,
                                "primary_us - local_us at the anchor", "us"),
          SchemaFieldDescriptor("skew_ppb", "Skew", FieldValueType::Float64,
                                "Relative crystal rate", "ppb"),
          // ⚠️ Saturates at 0xFFFFFFFF, which is 4.29 SECONDS, not milliseconds
          // (TEC-NATKIT-47). A saturated value means the window's samples sit
          // seconds off their own line -- treat it as "no usable fit", not as a
          // large-but-real residual.
          SchemaFieldDescriptor("residual_rms_ns", "Residual RMS",
                                FieldValueType::Uint32,
                                "Fit residual; saturates at 0xFFFFFFFF = 4.29 s", "ns"),
          SchemaFieldDescriptor("peak_residual_ns", "Peak residual",
                                FieldValueType::Uint32, "Worst residual in the window", "ns"),
          SchemaFieldDescriptor("last_beacon_local_us", "Last beacon",
                                FieldValueType::Uint64, "Last beacon in the leaf's clock", "us"),
          counter("beacons_seen", "Beacons seen", "Beacons the leaf received"),
          // ⚠️ Cumulative since the primary booted, like beacons_seen: a consumer
          // wanting a rate must DIFFERENCE two samples. Reading either as a
          // current figure produced a wrong conclusion on TEC-NATKIT-50.
          counter("beacons_missed", "Beacons missed", "Cumulative since boot; difference two samples for a rate"),
          counter("pairs_used", "Pairs used", "Beacon pairs that fed the fit"),
          counter("pairs_orphaned", "Pairs orphaned", "Beacon pairs discarded unmatched"),
          counter("outliers_rejected", "Outliers rejected", "Samples the fit gate refused"),
          counter("epoch_changes", "Epoch changes", "Times the fit was rebuilt"),
          SchemaFieldDescriptor("mac_spread_us", "MAC spread", FieldValueType::Uint32,
                                "Receive-callback jitter, measured", "us"),
          SchemaFieldDescriptor("samples_used", "Samples in fit", FieldValueType::Uint32,
                                "Points in the current fit"),
          SchemaFieldDescriptor("quality", "Quality", FieldValueType::Uint32,
                                "SyncQuality: graded on the fit, not the sample count"),
          counter("implausible_residuals", "Windows discarded",
                  "Windows thrown away for an impossible residual; 0 vs non-0 is the reading"),
          SchemaFieldDescriptor("valid", "Valid", FieldValueType::Bool,
                                "Whether the primary holds a usable fit for this leaf"),
      });
}

SchemaFieldDescriptor buildRootField() {
  return SchemaFieldDescriptor(
      "root", "Node health", FieldValueType::Object,
      "Per-leaf health as the primary sees it, published ~1 Hz on "
      "Log-<id>-Binary-NatKitNodeStatusV1",
      std::string{}, false, {},
      {
          SchemaFieldDescriptor("device_id", "Device id", FieldValueType::Uint64,
                                "The leaf's device id; matches the topic"),
          SchemaFieldDescriptor("mac", "MAC", FieldValueType::String, "The leaf's MAC address"),
          dbm("leaf_noise_floor_dbm", "Leaf noise floor",
              "The PHY's own floor at the leaf; the control for the hub's figure"),
          counter("data_frames", "Frames delivered", "Frames that reached the primary"),
          // ⚠️ Gaps are counted at the hub against the leaf's sequence numbers, so
          // they measure DELIVERY, not whether the leaf sampled.
          counter("seq_gaps", "Sequence gaps", "Missing sequence numbers at the hub"),
          counter("seq_duplicates", "Duplicates", "Repeated sequence numbers"),
          counter("seq_restarts", "Restarts", "Sequence went backwards: the leaf rebooted"),
          counter("heartbeats", "Heartbeats", "Heartbeat packets received"),
          SchemaFieldDescriptor("last_seen_us", "Last seen", FieldValueType::Uint64,
                                "Last packet, in the PRIMARY's clock", "us"),
          buildSyncField(),
          dbm("rssi_last", "RSSI", "Most recent received signal strength at the hub"),
          dbm("rssi_best", "RSSI best", "Best seen"),
          // ⚠️ best vs worst is the diagnostic: a fixed attenuation is an offset,
          // a wide swing is an intermittent connection (TEC-NATKIT-50).
          dbm("rssi_worst", "RSSI worst", "Worst seen; a wide best-worst swing means intermittent"),
          SchemaFieldDescriptor("rssi_seen", "RSSI samples", FieldValueType::Uint32,
                                "Packets the RSSI figures are drawn from"),
          SchemaFieldDescriptor("leaf_scan_channel", "Scanning channel", FieldValueType::Uint32,
                                "Non-zero while that leaf is hopping channels"),
          dbm("leaf_rssi_of_primary", "Leaf hears hub at",
              "Reciprocal link: how loudly the leaf hears the primary"),
          SchemaFieldDescriptor("leaf_tx_power_quarter_dbm", "Leaf TX power",
                                FieldValueType::Uint32, "Leaf transmit power", "quarter-dBm"),
          // ⚠️ built vs sent is THE pair that separates a sampling fault from a
          // radio fault: a leaf that builds normally and sends nothing has a
          // transmit problem, not a sensor problem (TEC-NATKIT-50).
          counter("leaf_frames_built", "Frames built", "Frames the leaf assembled"),
          counter("leaf_frames_dropped", "Frames dropped", "Dropped from a full queue on the leaf"),
          counter("leaf_send_failures", "Send failures", "ESP-NOW transmit failures on the leaf"),
          counter("leaf_channel_hops", "Channel hops", "Times the leaf changed channel"),
          counter("publish_no_sync", "Held: no fit", "Frames held because no clock fit existed"),
          counter("publish_no_shift", "Held: rewrite refused",
                  "Fit present but the timestamp rewrite was refused"),
      });
}

const SchemaFieldDescriptor &rootField() {
  static const SchemaFieldDescriptor field = buildRootField();
  return field;
}

}  // namespace

std::string NatKitNodeStatusV1Descriptor::getTargetSchemaName() const {
  return NatKitNodeStatusV1Schema::name;
}

uint16_t NatKitNodeStatusV1Descriptor::getDescriptorVersion() const {
  return descriptorVersion;
}

const SchemaFieldDescriptor &NatKitNodeStatusV1Descriptor::getRootField() const {
  return rootField();
}

Optional<FieldValueRef> NatKitNodeStatusV1Descriptor::tryGetFieldValue(
    const Schema &record,
    const std::string &path) const {
  const auto schemaPathMaybe = SchemaPath::parse(path);
  if (!schemaPathMaybe.has_value()) {
    return {};
  }
  const NatKitNodeStatusV1Schema *s =
      (record.getName() == NatKitNodeStatusV1Schema::name
           ? static_cast<const NatKitNodeStatusV1Schema *>(&record)
           : nullptr);
  if (s == nullptr) {
    return {};
  }
  const auto &seg = schemaPathMaybe.value().getSegments();
  if (seg.empty() || seg[0].isArrayIndex) {
    return {};
  }

  if (seg.size() == 1) {
    const std::string &f = seg[0].fieldId;
    if (f == "device_id") return FieldValueRef::fromUint64(s->deviceId);
    // Points at the record's own member: FieldValueRef does not own the string.
    if (f == "mac") return FieldValueRef::fromString(s->macText);
    if (f == "leaf_noise_floor_dbm") return FieldValueRef::fromInt16(s->leafNoiseFloorDbm);
    if (f == "data_frames") return FieldValueRef::fromUint32(s->dataFrames);
    if (f == "seq_gaps") return FieldValueRef::fromUint32(s->seqGaps);
    if (f == "seq_duplicates") return FieldValueRef::fromUint32(s->seqDuplicates);
    if (f == "seq_restarts") return FieldValueRef::fromUint32(s->seqRestarts);
    if (f == "heartbeats") return FieldValueRef::fromUint32(s->heartbeats);
    if (f == "last_seen_us") return FieldValueRef::fromUint64(s->lastSeenUs);
    if (f == "sync") return FieldValueRef::fromObject();
    if (f == "rssi_last") return FieldValueRef::fromInt16(s->rssiLast);
    if (f == "rssi_best") return FieldValueRef::fromInt16(s->rssiBest);
    if (f == "rssi_worst") return FieldValueRef::fromInt16(s->rssiWorst);
    if (f == "rssi_seen") return FieldValueRef::fromUint32(s->rssiSeen);
    if (f == "leaf_scan_channel") return FieldValueRef::fromUint32(s->leafScanChannel);
    if (f == "leaf_rssi_of_primary") return FieldValueRef::fromInt16(s->leafRssiOfPrimary);
    if (f == "leaf_tx_power_quarter_dbm")
      return FieldValueRef::fromUint32(s->leafTxPowerQuarterDbm);
    if (f == "leaf_frames_built") return FieldValueRef::fromUint32(s->leafFramesBuilt);
    if (f == "leaf_frames_dropped") return FieldValueRef::fromUint32(s->leafFramesDropped);
    if (f == "leaf_send_failures") return FieldValueRef::fromUint32(s->leafSendFailures);
    if (f == "leaf_channel_hops") return FieldValueRef::fromUint32(s->leafChannelHops);
    if (f == "publish_no_sync") return FieldValueRef::fromUint32(s->publishNoSync);
    if (f == "publish_no_shift") return FieldValueRef::fromUint32(s->publishNoShift);
    return {};
  }

  if (seg.size() == 2 && seg[0].fieldId == "sync" && !seg[1].isArrayIndex) {
    const std::string &f = seg[1].fieldId;
    if (f == "epoch") return FieldValueRef::fromUint32(s->syncEpoch);
    if (f == "ref_local_us") return FieldValueRef::fromUint64(s->syncRefLocalUs);
    if (f == "ref_offset_us") return FieldValueRef::fromFloat64(static_cast<double>(s->syncRefOffsetUs));
    if (f == "skew_ppb") return FieldValueRef::fromFloat64(static_cast<double>(s->syncSkewPpb));
    if (f == "residual_rms_ns") return FieldValueRef::fromUint32(s->syncResidualRmsNs);
    if (f == "peak_residual_ns") return FieldValueRef::fromUint32(s->syncPeakResidualNs);
    if (f == "last_beacon_local_us") return FieldValueRef::fromUint64(s->syncLastBeaconLocalUs);
    if (f == "beacons_seen") return FieldValueRef::fromUint32(s->beaconsSeen);
    if (f == "beacons_missed") return FieldValueRef::fromUint32(s->beaconsMissed);
    if (f == "pairs_used") return FieldValueRef::fromUint32(s->pairsUsed);
    if (f == "pairs_orphaned") return FieldValueRef::fromUint32(s->pairsOrphaned);
    if (f == "outliers_rejected") return FieldValueRef::fromUint32(s->outliersRejected);
    if (f == "epoch_changes") return FieldValueRef::fromUint32(s->epochChanges);
    if (f == "mac_spread_us") return FieldValueRef::fromUint32(s->macSpreadUs);
    if (f == "samples_used") return FieldValueRef::fromUint32(s->samplesUsed);
    if (f == "quality") return FieldValueRef::fromUint32(s->quality);
    if (f == "implausible_residuals") return FieldValueRef::fromUint32(s->implausibleResiduals);
    if (f == "valid") return FieldValueRef::fromBool(s->syncValid != 0);
    return {};
  }

  return {};
}

void NatKitNodeStatusV1Descriptor::registerWithRegistry(
    DataSchemaDescriptorRegistry &registry) {
  registry.registerDescriptor(
      std::shared_ptr<const DataSchemaDescriptor>(new NatKitNodeStatusV1Descriptor()));
}

}  // namespace core
}  // namespace nat
