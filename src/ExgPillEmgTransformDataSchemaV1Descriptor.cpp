#include <libnatkit-core.hpp>

namespace nat {
namespace core {

const std::string ExgPillEmgTransformDataSchemaV1Descriptor::name =
    "ExgPillEmgTransformDataSchemaV1Descriptor";
const uint16_t ExgPillEmgTransformDataSchemaV1Descriptor::descriptorVersion =
    1;

namespace {

SchemaFieldDescriptor buildTransformEmgDescriptorRootField() {
  const auto sampleField = std::make_shared<SchemaFieldDescriptor>(
      "sample",
      "Sample",
      FieldValueType::Float32,
      "One transformed EMG sample");

  const auto samplesField = SchemaFieldDescriptor(
      "samples",
      "Samples",
      FieldValueType::Array,
      "Per-channel transformed EMG samples",
      std::string{},
      false,
      std::vector<std::string>{},
      std::vector<SchemaFieldDescriptor>{},
      sampleField);

  const auto channelObject = std::make_shared<SchemaFieldDescriptor>(
      "channel",
      "Channel",
      FieldValueType::Object,
      "One transformed EMG channel",
      std::string{},
      false,
      std::vector<std::string>{},
      std::vector<SchemaFieldDescriptor>{
          SchemaFieldDescriptor(
              "label", "Label", FieldValueType::String, "Channel label"),
          samplesField});

  return SchemaFieldDescriptor(
      "root",
      "Transformed EMG Frame",
      FieldValueType::Object,
      "Descriptor for transformed EXG Pill EMG frames",
      std::string{},
      false,
      std::vector<std::string>{},
      std::vector<SchemaFieldDescriptor>{
          SchemaFieldDescriptor(
              "device_id",
              "Device ID",
              FieldValueType::String,
              "Source EMG device identifier"),
          SchemaFieldDescriptor(
              "seq_no",
              "Sequence Number",
              FieldValueType::Uint64,
              "Monotonic frame sequence number"),
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
              "Transformed EMG channels carried in this frame",
              std::string{},
              false,
              std::vector<std::string>{},
              std::vector<SchemaFieldDescriptor>{},
              channelObject)});
}

} // namespace

std::string ExgPillEmgTransformDataSchemaV1Descriptor::getTargetSchemaName()
    const {
  return ExgPillEmgTransformDataSchemaV1::name;
}

uint16_t ExgPillEmgTransformDataSchemaV1Descriptor::getDescriptorVersion()
    const {
  return descriptorVersion;
}

const SchemaFieldDescriptor &
ExgPillEmgTransformDataSchemaV1Descriptor::getRootField() const {
  static const SchemaFieldDescriptor rootField =
      buildTransformEmgDescriptorRootField();
  return rootField;
}

Optional<FieldValueRef>
ExgPillEmgTransformDataSchemaV1Descriptor::tryGetFieldValue(
    const Schema &record,
    const std::string &path) const {
  const auto schemaPathMaybe = SchemaPath::parse(path);
  if (!schemaPathMaybe.has_value()) {
    return {};
  }

  const ExgPillEmgTransformDataSchemaV1 *emgRecord =
      dynamic_cast<const ExgPillEmgTransformDataSchemaV1 *>(&record);
  if (emgRecord == nullptr) {
    return {};
  }

  const auto &segments = schemaPathMaybe.value().getSegments();
  if (segments.empty() || segments[0].isArrayIndex) {
    return {};
  }

  const auto &samples = emgRecord->getSamples();

  if (segments.size() == 1) {
    if (segments[0].fieldId == "device_id") {
      return FieldValueRef::fromString(emgRecord->getDeviceId());
    }
    if (segments[0].fieldId == "seq_no") {
      return FieldValueRef::fromUint64(emgRecord->getSeqNo());
    }
    if (segments[0].fieldId == "device_ts_us") {
      return FieldValueRef::fromUint64(emgRecord->getDeviceTsUs());
    }
    if (segments[0].fieldId == "sample_rate_hz") {
      return FieldValueRef::fromUint32(emgRecord->getSampleRateHz());
    }
    if (segments[0].fieldId == "channels") {
      return FieldValueRef::fromArray(emgRecord->getChannelCount());
    }
    return {};
  }

  if (segments[0].fieldId != "channels" || !segments[1].isArrayIndex) {
    return {};
  }

  const uint32_t channelIndex = segments[1].arrayIndex;
  if (channelIndex >= emgRecord->getChannelCount()) {
    return {};
  }

  if (segments.size() == 2) {
    return FieldValueRef::fromObject();
  }

  if (segments[2].isArrayIndex) {
    return {};
  }

  if (segments[2].fieldId == "label" && segments.size() == 3) {
    return FieldValueRef::fromString(
        emgRecord->getChannelLabels()[channelIndex]);
  }

  if (segments[2].fieldId != "samples") {
    return {};
  }

  if (segments.size() == 3) {
    return FieldValueRef::fromArray(emgRecord->getSamplesPerChannel());
  }

  if (segments.size() != 4 || !segments[3].isArrayIndex) {
    return {};
  }

  const uint32_t sampleIndex = segments[3].arrayIndex;
  if (sampleIndex >= emgRecord->getSamplesPerChannel()) {
    return {};
  }

  const size_t flattenedIndex =
      static_cast<size_t>(channelIndex) * emgRecord->getSamplesPerChannel() +
      sampleIndex;
  if (flattenedIndex >= samples.size()) {
    return {};
  }

  return FieldValueRef::fromFloat32(samples[flattenedIndex]);
}

void ExgPillEmgTransformDataSchemaV1Descriptor::registerWithRegistry(
    DataSchemaDescriptorRegistry &registry) {
  registry.registerDescriptor(
      std::shared_ptr<const DataSchemaDescriptor>(
          new ExgPillEmgTransformDataSchemaV1Descriptor()));
}

} // namespace core
} // namespace nat
