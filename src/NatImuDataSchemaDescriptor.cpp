#include <libnatkit-core.hpp>

namespace nat {
namespace core {

const std::string NatImuDataSchemaDescriptor::name = "NatImuDataSchemaDescriptor";
const uint16_t NatImuDataSchemaDescriptor::descriptorVersion = 1;

namespace {

SchemaFieldDescriptor buildNatImuRootField() {
  return SchemaFieldDescriptor(
      "root",
      "IMU Sample",
      FieldValueType::Object,
      "Descriptor for natKit IMU samples",
      std::string{},
      false,
      std::vector<std::string>{},
      std::vector<SchemaFieldDescriptor>{
          SchemaFieldDescriptor(
              "time", "Timestamp", FieldValueType::Uint64, "Sample timestamp", "us"),
          SchemaFieldDescriptor(
              "accel",
              "Accelerometer",
              FieldValueType::Object,
              "Accelerometer vector",
              "m/s^2",
              false,
              {},
              {
                  SchemaFieldDescriptor("x", "X", FieldValueType::Float32),
                  SchemaFieldDescriptor("y", "Y", FieldValueType::Float32),
                  SchemaFieldDescriptor("z", "Z", FieldValueType::Float32),
              }),
          SchemaFieldDescriptor(
              "gyro",
              "Gyroscope",
              FieldValueType::Object,
              "Gyroscope vector",
              "rad/s",
              false,
              {},
              {
                  SchemaFieldDescriptor("x", "X", FieldValueType::Float32),
                  SchemaFieldDescriptor("y", "Y", FieldValueType::Float32),
                  SchemaFieldDescriptor("z", "Z", FieldValueType::Float32),
              }),
          SchemaFieldDescriptor(
              "quat",
              "Rotation Quaternion",
              FieldValueType::Object,
              "Rotation quaternion",
              std::string{},
              false,
              {},
              {
                  SchemaFieldDescriptor("real", "Real", FieldValueType::Float32),
                  SchemaFieldDescriptor("i", "I", FieldValueType::Float32),
                  SchemaFieldDescriptor("j", "J", FieldValueType::Float32),
                  SchemaFieldDescriptor("k", "K", FieldValueType::Float32),
              }),
          SchemaFieldDescriptor(
              "accuracies",
              "Accuracies",
              FieldValueType::Object,
              "Sensor accuracies",
              std::string{},
              false,
              {},
              {
                  SchemaFieldDescriptor(
                      "accelerometer",
                      "Accelerometer",
                      FieldValueType::Uint32,
                      "Accelerometer accuracy enum"),
                  SchemaFieldDescriptor(
                      "gyroscope",
                      "Gyroscope",
                      FieldValueType::Uint32,
                      "Gyroscope accuracy enum"),
                  SchemaFieldDescriptor(
                      "rotation",
                      "Rotation",
                      FieldValueType::Uint32,
                      "Rotation accuracy enum"),
              }),
          SchemaFieldDescriptor(
              "has_data",
              "Has Data",
              FieldValueType::Object,
              "Data availability bits",
              std::string{},
              false,
              {},
              {
                  SchemaFieldDescriptor(
                      "accelerometer", "Accelerometer", FieldValueType::Bool),
                  SchemaFieldDescriptor(
                      "gyroscope", "Gyroscope", FieldValueType::Bool),
                  SchemaFieldDescriptor("rotation", "Rotation", FieldValueType::Bool),
              })});
}

Optional<FieldValueRef> getImuVectorValue(
    const float *data,
    const std::string &groupName,
    const std::string &axisName) {
  size_t baseIndex = 0;
  if (groupName == "accel") {
    baseIndex = 0;
  } else if (groupName == "gyro") {
    baseIndex = 3;
  } else if (groupName == "quat") {
    baseIndex = 6;
  } else {
    return {};
  }

  size_t axisOffset = 0;
  if (axisName == "x") {
    axisOffset = 0;
  } else if (axisName == "y") {
    axisOffset = 1;
  } else if (axisName == "z") {
    axisOffset = 2;
  } else if (groupName == "quat" && axisName == "real") {
    axisOffset = 0;
  } else if (groupName == "quat" && axisName == "i") {
    axisOffset = 1;
  } else if (groupName == "quat" && axisName == "j") {
    axisOffset = 2;
  } else if (groupName == "quat" && axisName == "k") {
    axisOffset = 3;
  } else {
    return {};
  }

  return FieldValueRef::fromFloat32(data[baseIndex + axisOffset]);
}

} // namespace

std::string NatImuDataSchemaDescriptor::getTargetSchemaName() const {
  return NatImuDataSchema::name;
}

uint16_t NatImuDataSchemaDescriptor::getDescriptorVersion() const {
  return descriptorVersion;
}

const SchemaFieldDescriptor &NatImuDataSchemaDescriptor::getRootField() const {
  static const SchemaFieldDescriptor rootField = buildNatImuRootField();
  return rootField;
}

Optional<FieldValueRef> NatImuDataSchemaDescriptor::tryGetFieldValue(
    const Schema &record,
    const std::string &path) const {
  const auto schemaPathMaybe = SchemaPath::parse(path);
  if (!schemaPathMaybe.has_value()) {
    return {};
  }

  const NatImuDataSchema *imuRecord =
      (record.getName() == NatImuDataSchema::name ? static_cast<const NatImuDataSchema *>(&record) : nullptr);
  if (imuRecord == nullptr) {
    return {};
  }

  const auto &segments = schemaPathMaybe.value().getSegments();
  if (segments.empty() || segments[0].isArrayIndex) {
    return {};
  }

  if (segments.size() == 1) {
    if (segments[0].fieldId == "time") {
      return FieldValueRef::fromUint64(static_cast<uint64_t>(imuRecord->getTime()));
    }
    if (segments[0].fieldId == "accel" || segments[0].fieldId == "gyro" ||
        segments[0].fieldId == "quat" || segments[0].fieldId == "accuracies" ||
        segments[0].fieldId == "has_data") {
      return FieldValueRef::fromObject();
    }
    return {};
  }

  if (segments[1].isArrayIndex) {
    return {};
  }

  const float *data = imuRecord->getData();
  if (segments[0].fieldId == "accel" || segments[0].fieldId == "gyro" ||
      segments[0].fieldId == "quat") {
    if (segments.size() != 2) {
      return {};
    }
    return getImuVectorValue(data, segments[0].fieldId, segments[1].fieldId);
  }

  if (segments.size() != 2) {
    return {};
  }

  if (segments[0].fieldId == "accuracies") {
    if (segments[1].fieldId == "accelerometer") {
      return FieldValueRef::fromUint32(
          static_cast<uint32_t>(imuRecord->getAccelerationAccuracy()));
    }
    if (segments[1].fieldId == "gyroscope") {
      return FieldValueRef::fromUint32(
          static_cast<uint32_t>(imuRecord->getGyroscopeAccuracy()));
    }
    if (segments[1].fieldId == "rotation") {
      return FieldValueRef::fromUint32(
          static_cast<uint32_t>(imuRecord->getRotationAccuracy()));
    }
  }

  if (segments[0].fieldId == "has_data") {
    if (segments[1].fieldId == "accelerometer") {
      return FieldValueRef::fromBool(imuRecord->wasDataSetForAcceleration());
    }
    if (segments[1].fieldId == "gyroscope") {
      return FieldValueRef::fromBool(imuRecord->wasDataSetForGryoscope());
    }
    if (segments[1].fieldId == "rotation") {
      return FieldValueRef::fromBool(imuRecord->wasDataSetForRotation());
    }
  }

  return {};
}

void NatImuDataSchemaDescriptor::registerWithRegistry(
    DataSchemaDescriptorRegistry &registry) {
  registry.registerDescriptor(
      std::shared_ptr<const DataSchemaDescriptor>(
          new NatImuDataSchemaDescriptor()));
}

} // namespace core
} // namespace nat
