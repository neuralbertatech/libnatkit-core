#include <libnatkit-core.hpp>

namespace nat {
namespace core {

const std::string NatSignalFrameDataSchemaV1Descriptor::name =
    "NatSignalFrameDataSchemaV1Descriptor";
const uint16_t NatSignalFrameDataSchemaV1Descriptor::descriptorVersion = 1;

namespace {

SchemaFieldDescriptor buildNatSignalFrameDescriptorRootField() {
  const auto sampleField = std::make_shared<SchemaFieldDescriptor>(
      "sample",
      "Sample",
      FieldValueType::Float32,
      "One numeric derived sample");

  const auto samplesField = SchemaFieldDescriptor(
      "samples",
      "Samples",
      FieldValueType::Array,
      "Per-channel numeric samples",
      std::string{},
      false,
      std::vector<std::string>{},
      std::vector<SchemaFieldDescriptor>{},
      sampleField);

  const auto channelObject = std::make_shared<SchemaFieldDescriptor>(
      "channel",
      "Channel",
      FieldValueType::Object,
      "One numeric signal channel",
      std::string{},
      false,
      std::vector<std::string>{},
      std::vector<SchemaFieldDescriptor>{
          SchemaFieldDescriptor(
              "label", "Label", FieldValueType::String, "Channel label", "", true),
          samplesField});

  return SchemaFieldDescriptor(
      "root",
      "Signal Frame",
      FieldValueType::Object,
      "Descriptor for generic numeric multi-channel signal frames",
      std::string{},
      false,
      std::vector<std::string>{},
      std::vector<SchemaFieldDescriptor>{
          SchemaFieldDescriptor(
              "device_id",
              "Device ID",
              FieldValueType::String,
              "Upstream producer or source identifier"),
          SchemaFieldDescriptor(
              "seq_no",
              "Sequence Number",
              FieldValueType::Uint64,
              "Frame sequence number"),
          SchemaFieldDescriptor(
              "device_ts_us",
              "Device Timestamp",
              FieldValueType::Uint64,
              "Producer timestamp in microseconds",
              "us"),
          SchemaFieldDescriptor(
              "sample_rate_hz",
              "Sample Rate",
              FieldValueType::Uint32,
              "Per-channel sample rate",
              "Hz"),
          SchemaFieldDescriptor(
              "channels",
              "Channels",
              FieldValueType::Array,
              "Signal channels carried in this frame",
              std::string{},
              false,
              std::vector<std::string>{},
              std::vector<SchemaFieldDescriptor>{},
              channelObject)});
}

} // namespace

std::string NatSignalFrameDataSchemaV1Descriptor::getTargetSchemaName() const {
  return NatSignalFrameDataSchemaV1::name;
}

uint16_t NatSignalFrameDataSchemaV1Descriptor::getDescriptorVersion() const {
  return descriptorVersion;
}

const SchemaFieldDescriptor &
NatSignalFrameDataSchemaV1Descriptor::getRootField() const {
  static const SchemaFieldDescriptor rootField =
      buildNatSignalFrameDescriptorRootField();
  return rootField;
}

Optional<FieldValueRef> NatSignalFrameDataSchemaV1Descriptor::tryGetFieldValue(
    const Schema &record,
    const std::string &path) const {
  const auto schemaPathMaybe = SchemaPath::parse(path);
  if (!schemaPathMaybe.has_value()) {
    return {};
  }

  const NatSignalFrameDataSchemaV1 *signalRecord =
      dynamic_cast<const NatSignalFrameDataSchemaV1 *>(&record);
  if (signalRecord == nullptr) {
    return {};
  }

  const auto &segments = schemaPathMaybe.value().getSegments();
  if (segments.empty() || segments[0].isArrayIndex) {
    return {};
  }

  const auto &samples = signalRecord->getSamples();

  if (segments.size() == 1) {
    if (segments[0].fieldId == "device_id") {
      return FieldValueRef::fromString(signalRecord->getDeviceId());
    }
    if (segments[0].fieldId == "seq_no") {
      return FieldValueRef::fromUint64(signalRecord->getSeqNo());
    }
    if (segments[0].fieldId == "device_ts_us") {
      return FieldValueRef::fromUint64(signalRecord->getDeviceTsUs());
    }
    if (segments[0].fieldId == "sample_rate_hz") {
      return FieldValueRef::fromUint32(signalRecord->getSampleRateHz());
    }
    if (segments[0].fieldId == "channels") {
      return FieldValueRef::fromArray(signalRecord->getChannelCount());
    }
    return {};
  }

  if (segments[0].fieldId != "channels" || !segments[1].isArrayIndex) {
    return {};
  }

  const uint32_t channelIndex = segments[1].arrayIndex;
  if (channelIndex >= signalRecord->getChannelCount()) {
    return {};
  }

  if (segments.size() == 2) {
    return FieldValueRef::fromObject();
  }

  if (segments[2].isArrayIndex) {
    return {};
  }

  if (segments[2].fieldId == "label" && segments.size() == 3) {
    return FieldValueRef::fromString(signalRecord->getChannelLabels()[channelIndex]);
  }

  if (segments[2].fieldId != "samples") {
    return {};
  }

  if (segments.size() == 3) {
    return FieldValueRef::fromArray(signalRecord->getSamplesPerChannel());
  }

  if (segments.size() != 4 || !segments[3].isArrayIndex) {
    return {};
  }

  const uint32_t sampleIndex = segments[3].arrayIndex;
  if (sampleIndex >= signalRecord->getSamplesPerChannel()) {
    return {};
  }

  const size_t flattenedIndex =
      static_cast<size_t>(channelIndex) * signalRecord->getSamplesPerChannel() +
      sampleIndex;
  if (flattenedIndex >= samples.size()) {
    return {};
  }

  return FieldValueRef::fromFloat32(samples[flattenedIndex]);
}

void NatSignalFrameDataSchemaV1Descriptor::registerWithRegistry(
    DataSchemaDescriptorRegistry &registry) {
  registry.registerDescriptor(
      std::shared_ptr<const DataSchemaDescriptor>(
          new NatSignalFrameDataSchemaV1Descriptor()));
}

} // namespace core
} // namespace nat
