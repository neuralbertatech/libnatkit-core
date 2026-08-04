#include <libnatkit-core.hpp>

namespace nat {
namespace core {

const std::string NatImuBulkDataSchemaDescriptor::name =
    "NatImuBulkDataSchemaDescriptor";
const uint16_t NatImuBulkDataSchemaDescriptor::descriptorVersion = 1;

namespace {

// One filterable channel: an array of Float32 samples across the frame.
SchemaFieldDescriptor buildImuAxisArrayField(
    const std::string &fieldId,
    const std::string &label,
    const std::string &unit) {
  return SchemaFieldDescriptor(
      fieldId,
      label,
      FieldValueType::Array,
      "Per-sample values across the frame",
      unit,
      false,
      {},
      {},
      std::make_shared<SchemaFieldDescriptor>(
          "sample", "Sample", FieldValueType::Float32, "One float sample"));
}

SchemaFieldDescriptor buildNatImuBulkRootField() {
  return SchemaFieldDescriptor(
      "root",
      "IMU Frame",
      FieldValueType::Object,
      "Channel-major view of a natKit IMU bulk frame (per-axis sample arrays)",
      std::string{},
      false,
      {},
      {
          SchemaFieldDescriptor(
              "seq_no",
              "Sequence",
              FieldValueType::Uint64,
              "Per-device frame sequence number"),
          SchemaFieldDescriptor(
              "device_ts_us",
              "Device Timestamp",
              FieldValueType::Uint64,
              "First-sample timestamp",
              "us"),
          SchemaFieldDescriptor(
              "sample_rate_hz",
              "Sample Rate",
              FieldValueType::Uint32,
              "Declared per-frame sample rate",
              "Hz"),
          buildImuAxisArrayField("accel_x", "Accel X", "m/s^2"),
          buildImuAxisArrayField("accel_y", "Accel Y", "m/s^2"),
          buildImuAxisArrayField("accel_z", "Accel Z", "m/s^2"),
          buildImuAxisArrayField("gyro_x", "Gyro X", "rad/s"),
          buildImuAxisArrayField("gyro_y", "Gyro Y", "rad/s"),
          buildImuAxisArrayField("gyro_z", "Gyro Z", "rad/s"),
      });
}

// Flattened index into NatImuDataSchema::getData() (accel 0-2, gyro 3-5) for each
// channel field id; -1 if the field is not a per-axis array.
int axisIndexForField(const std::string &fieldId) {
  if (fieldId == "accel_x") return 0;
  if (fieldId == "accel_y") return 1;
  if (fieldId == "accel_z") return 2;
  if (fieldId == "gyro_x") return 3;
  if (fieldId == "gyro_y") return 4;
  if (fieldId == "gyro_z") return 5;
  return -1;
}

} // namespace

std::string NatImuBulkDataSchemaDescriptor::getTargetSchemaName() const {
  return NatImuBulkDataSchema::name;
}

uint16_t NatImuBulkDataSchemaDescriptor::getDescriptorVersion() const {
  return descriptorVersion;
}

const SchemaFieldDescriptor &
NatImuBulkDataSchemaDescriptor::getRootField() const {
  static const SchemaFieldDescriptor rootField = buildNatImuBulkRootField();
  return rootField;
}

Optional<FieldValueRef> NatImuBulkDataSchemaDescriptor::tryGetFieldValue(
    const Schema &record,
    const std::string &path) const {
  const auto schemaPathMaybe = SchemaPath::parse(path);
  if (!schemaPathMaybe.has_value()) {
    return {};
  }

  const NatImuBulkDataSchema *bulk =
      (record.getName() == NatImuBulkDataSchema::name
           ? static_cast<const NatImuBulkDataSchema *>(&record)
           : nullptr);
  if (bulk == nullptr) {
    return {};
  }

  const auto &segments = schemaPathMaybe.value().getSegments();
  if (segments.empty() || segments[0].isArrayIndex) {
    return {};
  }

  const std::string &field = segments[0].fieldId;

  if (segments.size() == 1) {
    if (field == "seq_no") {
      return FieldValueRef::fromUint64(bulk->getSeqNo());
    }
    if (field == "device_ts_us") {
      return FieldValueRef::fromUint64(bulk->getDeviceTsUs());
    }
    if (field == "sample_rate_hz") {
      return FieldValueRef::fromUint32(bulk->getSampleRateHz());
    }
    if (axisIndexForField(field) >= 0) {
      return FieldValueRef::fromArray(bulk->getSampleCount());
    }
    return {};
  }

  if (segments.size() == 2 && segments[1].isArrayIndex) {
    const int axisIndex = axisIndexForField(field);
    if (axisIndex < 0) {
      return {};
    }
    const size_t sampleIndex = segments[1].arrayIndex;
    if (sampleIndex >= bulk->getSampleCount()) {
      return {};
    }
    const NatImuDataSchema *samples = bulk->getSamples();
    return FieldValueRef::fromFloat32(
        samples[sampleIndex].getData()[axisIndex]);
  }

  return {};
}

void NatImuBulkDataSchemaDescriptor::registerWithRegistry(
    DataSchemaDescriptorRegistry &registry) {
  registry.registerDescriptor(
      std::shared_ptr<const DataSchemaDescriptor>(
          new NatImuBulkDataSchemaDescriptor()));
}

} // namespace core
} // namespace nat
