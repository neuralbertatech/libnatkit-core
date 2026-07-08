#include <libnatkit-core.hpp>

namespace nat {
namespace core {

const std::string NatMuseDataSchemaDescriptor::name = "NatMuseDataSchemaDescriptor";
const uint16_t NatMuseDataSchemaDescriptor::descriptorVersion = 1;

namespace {

SchemaFieldDescriptor buildMuseVectorItemField(
    const std::string &fieldId,
    const std::string &label,
    const std::string &description) {
  return SchemaFieldDescriptor(
      fieldId,
      label,
      FieldValueType::Object,
      description,
      std::string{},
      false,
      {},
      {
          SchemaFieldDescriptor("x", "X", FieldValueType::Float32),
          SchemaFieldDescriptor("y", "Y", FieldValueType::Float32),
          SchemaFieldDescriptor("z", "Z", FieldValueType::Float32),
      });
}

SchemaFieldDescriptor buildMuseFloatArrayField(
    const std::string &fieldId,
    const std::string &label,
    const std::string &description) {
  return SchemaFieldDescriptor(
      fieldId,
      label,
      FieldValueType::Array,
      description,
      std::string{},
      false,
      {},
      {},
      std::make_shared<SchemaFieldDescriptor>(
          "sample", "Sample", FieldValueType::Float32, "One float sample"));
}

SchemaFieldDescriptor buildNatMuseRootField() {
  const auto motionSampleField = std::make_shared<SchemaFieldDescriptor>(
      buildMuseVectorItemField("sample", "Sample", "One motion sample"));

  return SchemaFieldDescriptor(
      "root",
      "Muse Sample",
      FieldValueType::Object,
      "Descriptor for natKit Muse samples",
      std::string{},
      false,
      {},
      {
          SchemaFieldDescriptor(
              "time", "Timestamp", FieldValueType::Uint64, "Sample timestamp", "us"),
          SchemaFieldDescriptor(
              "eeg_sequence",
              "EEG Sequence",
              FieldValueType::Uint32,
              "EEG packet sequence number"),
          SchemaFieldDescriptor(
              "motion_sequence",
              "Motion Sequence",
              FieldValueType::Uint32,
              "Motion packet sequence number"),
          SchemaFieldDescriptor(
              "eeg",
              "EEG",
              FieldValueType::Object,
              "EEG channels",
              std::string{},
              false,
              {},
              {
                  buildMuseFloatArrayField("tp9", "TP9", "Left ear EEG"),
                  buildMuseFloatArrayField("af7", "AF7", "Left forehead EEG"),
                  buildMuseFloatArrayField("af8", "AF8", "Right forehead EEG"),
                  buildMuseFloatArrayField("tp10", "TP10", "Right ear EEG"),
              }),
          SchemaFieldDescriptor(
              "accel",
              "Accelerometer",
              FieldValueType::Array,
              "Accelerometer motion samples",
              std::string{},
              false,
              {},
              {},
              motionSampleField),
          SchemaFieldDescriptor(
              "gyro",
              "Gyroscope",
              FieldValueType::Array,
              "Gyroscope motion samples",
              std::string{},
              false,
              {},
              {},
              motionSampleField),
          SchemaFieldDescriptor(
              "ppg",
              "PPG",
              FieldValueType::Object,
              "PPG channels",
              std::string{},
              false,
              {},
              {
                  buildMuseFloatArrayField("ppg0", "PPG0", "PPG channel 0"),
                  buildMuseFloatArrayField("ppg1", "PPG1", "PPG channel 1"),
                  buildMuseFloatArrayField("ppg2", "PPG2", "PPG channel 2"),
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
                  SchemaFieldDescriptor("eeg", "EEG", FieldValueType::Bool),
                  SchemaFieldDescriptor(
                      "accel", "Accelerometer", FieldValueType::Bool),
                  SchemaFieldDescriptor("gyro", "Gyroscope", FieldValueType::Bool),
                  SchemaFieldDescriptor("ppg", "PPG", FieldValueType::Bool),
              })});
}

Optional<FieldValueRef> getNamedFloatArrayValue(
    const std::string &arrayName,
    const NatMuseDataSchema &record,
    const SchemaPath::Segment &segment) {
  if (!segment.isArrayIndex) {
    return {};
  }

  const size_t index = segment.arrayIndex;
  const float *values = nullptr;
  size_t size = 0;
  if (arrayName == "tp9") {
    values = record.getTp9();
    size = NatMuseDataSchema::EEG_SAMPLES_PER_PACKET;
  } else if (arrayName == "af7") {
    values = record.getAf7();
    size = NatMuseDataSchema::EEG_SAMPLES_PER_PACKET;
  } else if (arrayName == "af8") {
    values = record.getAf8();
    size = NatMuseDataSchema::EEG_SAMPLES_PER_PACKET;
  } else if (arrayName == "tp10") {
    values = record.getTp10();
    size = NatMuseDataSchema::EEG_SAMPLES_PER_PACKET;
  } else if (arrayName == "ppg0") {
    values = record.getPpg0();
    size = NatMuseDataSchema::PPG_SAMPLES;
  } else if (arrayName == "ppg1") {
    values = record.getPpg1();
    size = NatMuseDataSchema::PPG_SAMPLES;
  } else if (arrayName == "ppg2") {
    values = record.getPpg2();
    size = NatMuseDataSchema::PPG_SAMPLES;
  } else {
    return {};
  }

  if (index >= size) {
    return {};
  }
  return FieldValueRef::fromFloat32(values[index]);
}

Optional<FieldValueRef> getMotionFieldValue(
    const std::string &groupName,
    const NatMuseDataSchema &record,
    const std::vector<SchemaPath::Segment> &segments) {
  if (segments.size() == 1) {
    return FieldValueRef::fromArray(NatMuseDataSchema::MOTION_SAMPLES);
  }
  if (!segments[1].isArrayIndex || segments[1].arrayIndex >= NatMuseDataSchema::MOTION_SAMPLES) {
    return {};
  }

  const float (*motion)[3] =
      groupName == "accel" ? record.getAccel() : record.getGyro();
  if (segments.size() == 2) {
    return FieldValueRef::fromObject();
  }
  if (segments.size() != 3 || segments[2].isArrayIndex) {
    return {};
  }

  size_t axisIndex = 0;
  if (segments[2].fieldId == "x") {
    axisIndex = 0;
  } else if (segments[2].fieldId == "y") {
    axisIndex = 1;
  } else if (segments[2].fieldId == "z") {
    axisIndex = 2;
  } else {
    return {};
  }
  return FieldValueRef::fromFloat32(motion[segments[1].arrayIndex][axisIndex]);
}

} // namespace

std::string NatMuseDataSchemaDescriptor::getTargetSchemaName() const {
  return NatMuseDataSchema::name;
}

uint16_t NatMuseDataSchemaDescriptor::getDescriptorVersion() const {
  return descriptorVersion;
}

const SchemaFieldDescriptor &NatMuseDataSchemaDescriptor::getRootField() const {
  static const SchemaFieldDescriptor rootField = buildNatMuseRootField();
  return rootField;
}

Optional<FieldValueRef> NatMuseDataSchemaDescriptor::tryGetFieldValue(
    const Schema &record,
    const std::string &path) const {
  const auto schemaPathMaybe = SchemaPath::parse(path);
  if (!schemaPathMaybe.has_value()) {
    return {};
  }

  const NatMuseDataSchema *museRecord =
      dynamic_cast<const NatMuseDataSchema *>(&record);
  if (museRecord == nullptr) {
    return {};
  }

  const auto &segments = schemaPathMaybe.value().getSegments();
  if (segments.empty() || segments[0].isArrayIndex) {
    return {};
  }

  if (segments.size() == 1) {
    if (segments[0].fieldId == "time") {
      return FieldValueRef::fromUint64(museRecord->getTime());
    }
    if (segments[0].fieldId == "eeg_sequence") {
      return FieldValueRef::fromUint32(museRecord->getEegSequence());
    }
    if (segments[0].fieldId == "motion_sequence") {
      return FieldValueRef::fromUint32(museRecord->getMotionSequence());
    }
    if (segments[0].fieldId == "eeg" || segments[0].fieldId == "ppg" ||
        segments[0].fieldId == "has_data") {
      return FieldValueRef::fromObject();
    }
    if (segments[0].fieldId == "accel" || segments[0].fieldId == "gyro") {
      return FieldValueRef::fromArray(NatMuseDataSchema::MOTION_SAMPLES);
    }
    return {};
  }

  if (segments[0].fieldId == "eeg") {
    if (segments.size() == 2) {
      if (segments[1].isArrayIndex) {
        return {};
      }
      return FieldValueRef::fromArray(NatMuseDataSchema::EEG_SAMPLES_PER_PACKET);
    }
    if (segments.size() == 3 && !segments[1].isArrayIndex) {
      return getNamedFloatArrayValue(segments[1].fieldId, *museRecord, segments[2]);
    }
    return {};
  }

  if (segments[0].fieldId == "ppg") {
    if (segments.size() == 2) {
      if (segments[1].isArrayIndex) {
        return {};
      }
      return FieldValueRef::fromArray(NatMuseDataSchema::PPG_SAMPLES);
    }
    if (segments.size() == 3 && !segments[1].isArrayIndex) {
      return getNamedFloatArrayValue(segments[1].fieldId, *museRecord, segments[2]);
    }
    return {};
  }

  if (segments[0].fieldId == "accel" || segments[0].fieldId == "gyro") {
    return getMotionFieldValue(segments[0].fieldId, *museRecord, segments);
  }

  if (segments[0].fieldId == "has_data" && segments.size() == 2 &&
      !segments[1].isArrayIndex) {
    if (segments[1].fieldId == "eeg") {
      return FieldValueRef::fromBool(museRecord->hasEegData());
    }
    if (segments[1].fieldId == "accel") {
      return FieldValueRef::fromBool(museRecord->hasAccelData());
    }
    if (segments[1].fieldId == "gyro") {
      return FieldValueRef::fromBool(museRecord->hasGyroData());
    }
    if (segments[1].fieldId == "ppg") {
      return FieldValueRef::fromBool(museRecord->hasPpgData());
    }
  }

  return {};
}

void NatMuseDataSchemaDescriptor::registerWithRegistry(
    DataSchemaDescriptorRegistry &registry) {
  registry.registerDescriptor(
      std::shared_ptr<const DataSchemaDescriptor>(
          new NatMuseDataSchemaDescriptor()));
}

} // namespace core
} // namespace nat
