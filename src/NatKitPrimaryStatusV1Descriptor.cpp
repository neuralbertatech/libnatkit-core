#include <libnatkit-core.hpp>

namespace nat {
namespace core {

const std::string NatKitPrimaryStatusV1Descriptor::name =
    "NatKitPrimaryStatusV1Descriptor";
const uint16_t NatKitPrimaryStatusV1Descriptor::descriptorVersion = 1;

namespace {

// Flat, unlike the node status: UplinkPrimaryStatus has no nested struct, so
// there is nothing to group that would not be grouping invented here.
//
// ⚠️ Almost everything below is CUMULATIVE SINCE THE HUB BOOTED. A consumer
// wanting a rate must difference two samples; reading one as a current figure is
// the mistake TEC-NATKIT-50 was. The four exceptions are marked GAUGE in their
// descriptions: nodes_known, frames_queued and command_subscriptions are counts
// of what exists right now, and the coherence figures describe a window.
SchemaFieldDescriptor buildRootField() {
  return SchemaFieldDescriptor(
      "root", "Hub health", FieldValueType::Object,
      "The hub's own health, published ~1 Hz on "
      "Log-<id>-Binary-NatKitPrimaryStatusV1",
      std::string{}, false, {},
      {
          SchemaFieldDescriptor("device_id", "Device id", FieldValueType::Uint64,
                                "The hub's own id, matching its topic prefix"),
          SchemaFieldDescriptor("uptime_us", "Uptime", FieldValueType::Uint64,
                                "The hub's monotonic clock: the axis every leaf's last_seen_us is on", "us"),
          SchemaFieldDescriptor("epoch", "Sync epoch", FieldValueType::Uint32,
                                "The hub's beacon epoch; a change means every leaf's fit was rebuilt"),
          SchemaFieldDescriptor("free_heap", "Free heap", FieldValueType::Uint32,
                                "Heap free now", "bytes"),
          SchemaFieldDescriptor("min_free_heap", "Heap low water", FieldValueType::Uint32,
                                "Least heap ever free since boot; the figure that predicts an OOM", "bytes"),
          SchemaFieldDescriptor("nodes_known", "Nodes known", FieldValueType::Uint32,
                                "GAUGE, not a counter: leaves in the registry right now"),
          SchemaFieldDescriptor("nodes_rejected", "Nodes rejected", FieldValueType::Uint32,
                                "Cumulative: registrations refused (registry full, or sealed)"),
          SchemaFieldDescriptor("unknown_packets", "Unknown packets", FieldValueType::Uint32,
                                "Cumulative: ESP-NOW packets from an unregistered sender"),
          // ⚠️ CUMULATIVE, and UNDERCOUNTS. Not the queue's depth: it counts
          // frames enqueued since boot. It is incremented with a plain ++ on a
          // shared uint32_t from two different tasks -- the ESP-NOW receive
          // callback on the WiFi task and the 1 Hz status loop -- so increments
          // are lost to the race. On the live rig it reads ~30 BELOW
          // frames_sent while frames_dropped is 0, which the code makes
          // otherwise impossible. Never use it as the denominator of a delivery
          // ratio (TEC-NATKIT-75); frames_sent and frames_dropped are the
          // single-writer pair worth trusting.
          SchemaFieldDescriptor("frames_queued", "Frames enqueued", FieldValueType::Uint32,
                                "Cumulative and racy: undercounts, reads below frames_sent; "
                                "do not use as a denominator"),
          SchemaFieldDescriptor("frames_sent", "Frames sent", FieldValueType::Uint32,
                                "Cumulative, single-writer: frames the active transport accepted "
                                "(a UART write, or an MQTT publish in gateway mode)"),
          SchemaFieldDescriptor("frames_dropped", "Frames dropped", FieldValueType::Uint32,
                                "Cumulative: frames discarded because the queue was full"),
          SchemaFieldDescriptor("write_timeouts", "Write timeouts", FieldValueType::Uint32,
                                "Cumulative: uplink writes that timed out"),
          SchemaFieldDescriptor("bytes_sent", "Bytes sent", FieldValueType::Uint64,
                                "Cumulative bytes on the uplink", "bytes"),
          SchemaFieldDescriptor("coherence_typical_us", "Coherence typical", FieldValueType::Uint32,
                                "Rig-wide clock agreement, typical case", "us"),
          SchemaFieldDescriptor("coherence_bound_us", "Coherence bound", FieldValueType::Uint32,
                                "The figure to quote as the rig's synchronisation: a bound, not a mean", "us"),
          SchemaFieldDescriptor("coherence_worst_us", "Coherence worst", FieldValueType::Uint32,
                                "Worst pairwise disagreement observed in the window", "us"),
          SchemaFieldDescriptor("coherence_samples", "Coherence samples", FieldValueType::Uint32,
                                "Points behind the coherence figures; a low count makes them soft"),
          SchemaFieldDescriptor("coherence_quality", "Coherence quality", FieldValueType::Uint32,
                                "SyncQuality grade for the rig as a whole"),
          SchemaFieldDescriptor("coherence_measured", "Coherence measured", FieldValueType::Bool,
                                "False means the coherence fields are stale, not zero"),
          SchemaFieldDescriptor("registry_sealed", "Registry sealed", FieldValueType::Bool,
                                "Whether the hub is refusing new leaf registrations"),
          SchemaFieldDescriptor("noise_floor_dbm", "Noise floor", FieldValueType::Int16,
                                "The hub's own noise floor", "dBm"),
          SchemaFieldDescriptor("chip_temp_c", "Die temperature", FieldValueType::Int16,
                                "Only meaningful when chip_temp_err is 0", "C"),
          SchemaFieldDescriptor("chip_temp_err", "Temp read error", FieldValueType::Bool,
                                "Non-zero means the temperature sensor read failed; ignore chip_temp_c"),
          SchemaFieldDescriptor("commands_received", "Commands received", FieldValueType::Uint32,
                                "Cumulative: commands arriving from the server"),
          SchemaFieldDescriptor("commands_relayed", "Commands relayed", FieldValueType::Uint32,
                                "Cumulative: commands forwarded to a leaf over ESP-NOW"),
          SchemaFieldDescriptor("commands_malformed", "Commands malformed", FieldValueType::Uint32,
                                "Cumulative: commands that would not parse"),
          SchemaFieldDescriptor("commands_unknown_device", "Commands for unknown device", FieldValueType::Uint32,
                                "Cumulative: commands addressed to a leaf not in the registry"),
          SchemaFieldDescriptor("commands_send_failed", "Command sends failed", FieldValueType::Uint32,
                                "Cumulative: ESP-NOW send returned an error"),
          SchemaFieldDescriptor("command_subscriptions", "Command subscriptions", FieldValueType::Uint32,
                                "GAUGE: command topics the hub is subscribed to"),
          SchemaFieldDescriptor("command_answers_received", "Answers received", FieldValueType::Uint32,
                                "Cumulative: replies received from leaves"),
          SchemaFieldDescriptor("command_answers_published", "Answers published", FieldValueType::Uint32,
                                "Cumulative: replies forwarded to the server"),
          SchemaFieldDescriptor("command_answers_duplicate", "Answers duplicate", FieldValueType::Uint32,
                                "Cumulative: repeat replies suppressed"),
          SchemaFieldDescriptor("commands_delivered", "Commands delivered", FieldValueType::Uint32,
                                "Cumulative: commands a leaf acknowledged"),
          SchemaFieldDescriptor("command_retransmits", "Command retransmits", FieldValueType::Uint32,
                                "Cumulative: resends after no acknowledgement"),
          SchemaFieldDescriptor("commands_undelivered", "Commands undelivered", FieldValueType::Uint32,
                                "Cumulative: commands given up on; the one to alarm on"),
          // ⚠️ The rig-wide coherence accumulators (TEC-NATKIT-52). coherence_bound_us
          // above is an average since boot: two readings a day apart differ mostly
          // because more history accumulated, not because the rig changed.
          SchemaFieldDescriptor("has_coherence_sums", "Reports coherence sums",
                                FieldValueType::Bool,
                                "False on firmware predating them — NOT the same as zero"),
          SchemaFieldDescriptor("spread_sum_us", "Pair-spread sum",
                                FieldValueType::Float64,
                                "Sum of pairwise spreads; difference two samples for a "
                                "windowed mean", "us"),
          SchemaFieldDescriptor("spread_sum_sq", "Pair-spread sum of squares",
                                FieldValueType::Uint64,
                                "With the sum and markers_paired, gives a windowed sd"),
          SchemaFieldDescriptor("markers_paired", "Markers paired", FieldValueType::Uint32,
                                "Samples behind the two sums"),
          SchemaFieldDescriptor("reset_reason", "Reset reason", FieldValueType::Uint32,
                                "esp_reset_reason() from the hub's last boot; 1 = power-on"),
      });
}

const SchemaFieldDescriptor &rootField() {
  static const SchemaFieldDescriptor field = buildRootField();
  return field;
}

}  // namespace

std::string NatKitPrimaryStatusV1Descriptor::getTargetSchemaName() const {
  return NatKitPrimaryStatusV1Schema::name;
}

uint16_t NatKitPrimaryStatusV1Descriptor::getDescriptorVersion() const {
  return descriptorVersion;
}

const SchemaFieldDescriptor &NatKitPrimaryStatusV1Descriptor::getRootField() const {
  return rootField();
}

Optional<FieldValueRef> NatKitPrimaryStatusV1Descriptor::tryGetFieldValue(
    const Schema &record,
    const std::string &path) const {
  const auto schemaPathMaybe = SchemaPath::parse(path);
  if (!schemaPathMaybe.has_value()) {
    return {};
  }
  const NatKitPrimaryStatusV1Schema *s =
      (record.getName() == NatKitPrimaryStatusV1Schema::name
           ? static_cast<const NatKitPrimaryStatusV1Schema *>(&record)
           : nullptr);
  if (s == nullptr) {
    return {};
  }
  const auto &seg = schemaPathMaybe.value().getSegments();
  if (seg.size() != 1 || seg[0].isArrayIndex) {
    return {};
  }
  const std::string &f = seg[0].fieldId;
    if (f == "device_id") return FieldValueRef::fromUint64(s->deviceId);
    if (f == "uptime_us") return FieldValueRef::fromUint64(s->uptimeUs);
    if (f == "epoch") return FieldValueRef::fromUint32(s->epoch);
    if (f == "free_heap") return FieldValueRef::fromUint32(s->freeHeap);
    if (f == "min_free_heap") return FieldValueRef::fromUint32(s->minFreeHeap);
    if (f == "nodes_known") return FieldValueRef::fromUint32(s->nodesKnown);
    if (f == "nodes_rejected") return FieldValueRef::fromUint32(s->nodesRejected);
    if (f == "unknown_packets") return FieldValueRef::fromUint32(s->unknownPackets);
    if (f == "frames_queued") return FieldValueRef::fromUint32(s->framesQueued);
    if (f == "frames_sent") return FieldValueRef::fromUint32(s->framesSent);
    if (f == "frames_dropped") return FieldValueRef::fromUint32(s->framesDropped);
    if (f == "write_timeouts") return FieldValueRef::fromUint32(s->writeTimeouts);
    if (f == "bytes_sent") return FieldValueRef::fromUint64(s->bytesSent);
    if (f == "coherence_typical_us") return FieldValueRef::fromUint32(s->coherenceTypicalUs);
    if (f == "coherence_bound_us") return FieldValueRef::fromUint32(s->coherenceBoundUs);
    if (f == "coherence_worst_us") return FieldValueRef::fromUint32(s->coherenceWorstUs);
    if (f == "coherence_samples") return FieldValueRef::fromUint32(s->coherenceSamples);
    if (f == "coherence_quality") return FieldValueRef::fromUint32(s->coherenceQuality);
    if (f == "coherence_measured") return FieldValueRef::fromBool(s->coherenceMeasured != 0);
    if (f == "registry_sealed") return FieldValueRef::fromBool(s->registrySealed != 0);
    if (f == "noise_floor_dbm") return FieldValueRef::fromInt16(s->noiseFloorDbm);
    if (f == "chip_temp_c") return FieldValueRef::fromInt16(s->chipTempC);
    if (f == "chip_temp_err") return FieldValueRef::fromBool(s->chipTempErr != 0);
    if (f == "commands_received") return FieldValueRef::fromUint32(s->commandsReceived);
    if (f == "commands_relayed") return FieldValueRef::fromUint32(s->commandsRelayed);
    if (f == "commands_malformed") return FieldValueRef::fromUint32(s->commandsMalformed);
    if (f == "commands_unknown_device") return FieldValueRef::fromUint32(s->commandsUnknownDevice);
    if (f == "commands_send_failed") return FieldValueRef::fromUint32(s->commandsSendFailed);
    if (f == "command_subscriptions") return FieldValueRef::fromUint32(s->commandSubscriptions);
    if (f == "command_answers_received") return FieldValueRef::fromUint32(s->commandAnswersReceived);
    if (f == "command_answers_published") return FieldValueRef::fromUint32(s->commandAnswersPublished);
    if (f == "command_answers_duplicate") return FieldValueRef::fromUint32(s->commandAnswersDuplicate);
    if (f == "commands_delivered") return FieldValueRef::fromUint32(s->commandsDelivered);
    if (f == "command_retransmits") return FieldValueRef::fromUint32(s->commandRetransmits);
    if (f == "commands_undelivered") return FieldValueRef::fromUint32(s->commandsUndelivered);
    if (f == "reset_reason") return FieldValueRef::fromUint32(s->resetReason);
    if (f == "has_coherence_sums") return FieldValueRef::fromBool(s->hasCoherenceSums);
    if (f == "spread_sum_us")
      return FieldValueRef::fromFloat64(static_cast<double>(s->spreadSumUs));
    if (f == "spread_sum_sq") return FieldValueRef::fromUint64(s->spreadSumSq);
    if (f == "markers_paired") return FieldValueRef::fromUint32(s->markersPaired);
  return {};
}

void NatKitPrimaryStatusV1Descriptor::registerWithRegistry(
    DataSchemaDescriptorRegistry &registry) {
  registry.registerDescriptor(std::shared_ptr<const DataSchemaDescriptor>(
      new NatKitPrimaryStatusV1Descriptor()));
}

}  // namespace core
}  // namespace nat
