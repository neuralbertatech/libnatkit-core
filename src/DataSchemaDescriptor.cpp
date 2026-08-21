#include <libnatkit-core.hpp>

#ifdef SERVER
#include <nlohmann/json.hpp>
#endif

#include <cctype>
#include <sstream>

namespace nat {
namespace core {

namespace {

uint32_t hashStringToUint32(const std::string &value) {
  uint32_t hash = 2166136261u;
  for (unsigned char ch : value) {
    hash ^= static_cast<uint32_t>(ch);
    hash *= 16777619u;
  }
  return hash;
}

#ifdef SERVER
nlohmann::json fieldValueTypeToJson(const FieldValueType type) {
  switch (type) {
  case FieldValueType::Bool:
    return "bool";
  case FieldValueType::Int16:
    return "int16";
  case FieldValueType::Uint32:
    return "uint32";
  case FieldValueType::Uint64:
    return "uint64";
  case FieldValueType::Float32:
    return "float32";
  case FieldValueType::Float64:
    return "float64";
  case FieldValueType::String:
    return "string";
  case FieldValueType::Enum:
    return "enum";
  case FieldValueType::Object:
    return "object";
  case FieldValueType::Array:
    return "array";
  default:
    return "unknown";
  }
}

nlohmann::json schemaFieldDescriptorToJson(const SchemaFieldDescriptor &field) {
  nlohmann::json json;
  json["id"] = field.getFieldId();
  json["label"] = field.getLabel();
  json["type"] = fieldValueTypeToJson(field.getValueType());
  if (!field.getDescription().empty()) {
    json["description"] = field.getDescription();
  }
  if (!field.getUnit().empty()) {
    json["unit"] = field.getUnit();
  }
  json["optional"] = field.isOptional();
  if (!field.getEnumValues().empty()) {
    json["enum_values"] = field.getEnumValues();
  }
  if (!field.getChildFields().empty()) {
    json["fields"] = nlohmann::json::object();
    for (const auto &child : field.getChildFields()) {
      json["fields"][child.getFieldId()] = schemaFieldDescriptorToJson(child);
    }
  }
  if (field.getArrayItemField()) {
    json["items"] = schemaFieldDescriptorToJson(*field.getArrayItemField());
  }
  return json;
}
#endif

} // namespace

SchemaFieldDescriptor::SchemaFieldDescriptor()
    : optional(false), valueType(FieldValueType::Object) {}

SchemaFieldDescriptor::SchemaFieldDescriptor(
    const std::string &fieldId,
    const std::string &label,
    FieldValueType valueType,
    const std::string &description,
    const std::string &unit,
    bool optional,
    const std::vector<std::string> &enumValues,
    const std::vector<SchemaFieldDescriptor> &childFields,
    const std::shared_ptr<SchemaFieldDescriptor> &arrayItemField)
    : fieldId(fieldId),
      label(label),
      description(description),
      unit(unit),
      optional(optional),
      valueType(valueType),
      enumValues(enumValues),
      childFields(childFields),
      arrayItemField(arrayItemField) {}

const std::string &SchemaFieldDescriptor::getFieldId() const { return fieldId; }
const std::string &SchemaFieldDescriptor::getLabel() const { return label; }
const std::string &SchemaFieldDescriptor::getDescription() const {
  return description;
}
const std::string &SchemaFieldDescriptor::getUnit() const { return unit; }
bool SchemaFieldDescriptor::isOptional() const { return optional; }
FieldValueType SchemaFieldDescriptor::getValueType() const { return valueType; }
const std::vector<std::string> &SchemaFieldDescriptor::getEnumValues() const {
  return enumValues;
}
const std::vector<SchemaFieldDescriptor> &
SchemaFieldDescriptor::getChildFields() const {
  return childFields;
}
const std::shared_ptr<SchemaFieldDescriptor> &
SchemaFieldDescriptor::getArrayItemField() const {
  return arrayItemField;
}

const SchemaFieldDescriptor *SchemaFieldDescriptor::findChildField(
    const std::string &childFieldId) const {
  for (const auto &child : childFields) {
    if (child.getFieldId() == childFieldId) {
      return &child;
    }
  }
  return nullptr;
}

SchemaPath::Segment::Segment() : isArrayIndex(false), arrayIndex(0) {}
SchemaPath::Segment::Segment(const std::string &fieldId)
    : isArrayIndex(false), fieldId(fieldId), arrayIndex(0) {}
SchemaPath::Segment::Segment(uint32_t arrayIndex)
    : isArrayIndex(true), arrayIndex(arrayIndex) {}

SchemaPath::SchemaPath() = default;
SchemaPath::SchemaPath(const std::vector<Segment> &segments) : segments(segments) {}

const std::vector<SchemaPath::Segment> &SchemaPath::getSegments() const {
  return segments;
}

Optional<SchemaPath> SchemaPath::parse(const std::string &path) {
  if (path.empty()) {
    return {};
  }

  std::vector<Segment> parsedSegments{};
  std::string current{};
  for (size_t i = 0; i <= path.size(); ++i) {
    if (i == path.size() || path[i] == '.') {
      if (current.empty()) {
        return {};
      }
      const bool isNumeric = std::all_of(
          current.begin(), current.end(),
          [](char ch) { return std::isdigit(static_cast<unsigned char>(ch)) != 0; });
      if (isNumeric) {
        parsedSegments.emplace_back(
            static_cast<uint32_t>(std::stoul(current)));
      } else {
        parsedSegments.emplace_back(current);
      }
      current.clear();
      continue;
    }
    current.push_back(path[i]);
  }

  return SchemaPath(parsedSegments);
}

FieldValueRef::FieldValueRef()
    : valueType(FieldValueType::Object), valuePtr(nullptr), elementCount(0) {}

FieldValueRef::FieldValueRef(
    FieldValueType valueType, const void *valuePtr, size_t elementCount)
    : valueType(valueType), valuePtr(valuePtr), elementCount(elementCount) {}

FieldValueRef FieldValueRef::fromBool(const bool &value) {
  FieldValueRef ref;
  ref.valueType = FieldValueType::Bool;
  ref.scalarStorage.boolValue = value;
  ref.valuePtr = &ref.scalarStorage.boolValue;
  return ref;
}
FieldValueRef FieldValueRef::fromInt16(const int16_t &value) {
  FieldValueRef ref;
  ref.valueType = FieldValueType::Int16;
  ref.scalarStorage.int16Value = value;
  ref.valuePtr = &ref.scalarStorage.int16Value;
  return ref;
}
FieldValueRef FieldValueRef::fromUint32(const uint32_t &value) {
  FieldValueRef ref;
  ref.valueType = FieldValueType::Uint32;
  ref.scalarStorage.uint32Value = value;
  ref.valuePtr = &ref.scalarStorage.uint32Value;
  return ref;
}
FieldValueRef FieldValueRef::fromUint64(const uint64_t &value) {
  FieldValueRef ref;
  ref.valueType = FieldValueType::Uint64;
  ref.scalarStorage.uint64Value = value;
  ref.valuePtr = &ref.scalarStorage.uint64Value;
  return ref;
}
FieldValueRef FieldValueRef::fromFloat32(const float &value) {
  FieldValueRef ref;
  ref.valueType = FieldValueType::Float32;
  ref.scalarStorage.float32Value = value;
  ref.valuePtr = &ref.scalarStorage.float32Value;
  return ref;
}
FieldValueRef FieldValueRef::fromFloat64(const double &value) {
  FieldValueRef ref;
  ref.valueType = FieldValueType::Float64;
  ref.scalarStorage.float64Value = value;
  ref.valuePtr = &ref.scalarStorage.float64Value;
  return ref;
}
FieldValueRef FieldValueRef::fromString(const std::string &value) {
  return FieldValueRef(FieldValueType::String, &value);
}
FieldValueRef FieldValueRef::fromArray(size_t elementCount) {
  return FieldValueRef(FieldValueType::Array, nullptr, elementCount);
}
FieldValueRef FieldValueRef::fromObject() {
  return FieldValueRef(FieldValueType::Object, nullptr, 0);
}

FieldValueType FieldValueRef::getValueType() const { return valueType; }
size_t FieldValueRef::getElementCount() const { return elementCount; }

Optional<bool> FieldValueRef::getBool() const {
  if (valueType != FieldValueType::Bool) {
    return {};
  }
  return scalarStorage.boolValue;
}
Optional<int16_t> FieldValueRef::getInt16() const {
  if (valueType != FieldValueType::Int16) {
    return {};
  }
  return scalarStorage.int16Value;
}
Optional<uint32_t> FieldValueRef::getUint32() const {
  if (valueType != FieldValueType::Uint32) {
    return {};
  }
  return scalarStorage.uint32Value;
}
Optional<uint64_t> FieldValueRef::getUint64() const {
  if (valueType != FieldValueType::Uint64) {
    return {};
  }
  return scalarStorage.uint64Value;
}
Optional<float> FieldValueRef::getFloat32() const {
  if (valueType != FieldValueType::Float32) {
    return {};
  }
  return scalarStorage.float32Value;
}
Optional<double> FieldValueRef::getFloat64() const {
  if (valueType != FieldValueType::Float64) {
    return {};
  }
  return scalarStorage.float64Value;
}
Optional<std::string> FieldValueRef::getString() const {
  if (valueType != FieldValueType::String || valuePtr == nullptr) {
    return {};
  }
  return *static_cast<const std::string *>(valuePtr);
}

std::unique_ptr<std::vector<uint8_t>>
DataSchemaDescriptor::encodeToBytes(const SerializationType &type) const {
  switch (type) {
  case SerializationType::Json: {
#ifdef SERVER
    nlohmann::json json;
    json["schema_name"] = getTargetSchemaName();
    json["descriptor_version"] = getDescriptorVersion();
    json["root"] = schemaFieldDescriptorToJson(getRootField());
    const auto jsonString = json.dump();
    return nat::core::make_unique<std::vector<uint8_t>>(
        jsonString.begin(), jsonString.end());
#else
    return nat::core::make_unique<std::vector<uint8_t>>();
#endif
  }
  default:
    return nat::core::make_unique<std::vector<uint8_t>>();
  }
}

bool DataSchemaDescriptor::isSerializationTypeSupported(
    const SerializationType type) const {
  return type == SerializationType::Json;
}

std::string DataSchemaDescriptor::getName() const {
  return getTargetSchemaName() + "Descriptor";
}

std::string DataSchemaDescriptor::toString() const {
  auto encoded = encodeToBytes(SerializationType::Json);
  if (!encoded) {
    return getName();
  }
  return std::string(encoded->begin(), encoded->end());
}

uint32_t DataSchemaDescriptor::getRecordTypeId() const {
  return hashStringToUint32(getName());
}

uint16_t DataSchemaDescriptor::getRecordVersion() const {
  return getDescriptorVersion();
}

Optional<std::string> DataSchemaDescriptor::getString(
    const Schema &record,
    const std::string &path) const {
  const auto fieldValue = tryGetFieldValue(record, path);
  if (!fieldValue.has_value()) {
    return {};
  }
  return fieldValue.value().getString();
}

Optional<int16_t> DataSchemaDescriptor::getInt16(
    const Schema &record,
    const std::string &path) const {
  const auto fieldValue = tryGetFieldValue(record, path);
  if (!fieldValue.has_value()) {
    return {};
  }
  return fieldValue.value().getInt16();
}

Optional<uint32_t> DataSchemaDescriptor::getUint32(
    const Schema &record,
    const std::string &path) const {
  const auto fieldValue = tryGetFieldValue(record, path);
  if (!fieldValue.has_value()) {
    return {};
  }
  return fieldValue.value().getUint32();
}

Optional<uint64_t> DataSchemaDescriptor::getUint64(
    const Schema &record,
    const std::string &path) const {
  const auto fieldValue = tryGetFieldValue(record, path);
  if (!fieldValue.has_value()) {
    return {};
  }
  return fieldValue.value().getUint64();
}

void DataSchemaDescriptorRegistry::registerDescriptor(
    const std::shared_ptr<const DataSchemaDescriptor> &descriptor) {
  if (descriptor) {
    descriptorsBySchemaName[descriptor->getTargetSchemaName()] = descriptor;
  }
}

Optional<std::shared_ptr<const DataSchemaDescriptor>>
DataSchemaDescriptorRegistry::findBySchemaName(
    const std::string &schemaName) const {
  const auto found = descriptorsBySchemaName.find(schemaName);
  if (found == descriptorsBySchemaName.end()) {
    return {};
  }
  return found->second;
}

DataSchemaDescriptorRegistry &DataSchemaDescriptorRegistry::getDefault() {
  static DataSchemaDescriptorRegistry registry{};
  static bool initialized = false;
  if (!initialized) {
    ExgPillEmgDataSchemaV1Descriptor::registerWithRegistry(registry);
    ExgPillEmgTransformDataSchemaV1Descriptor::registerWithRegistry(registry);
    NatSignalFrameDataSchemaV1Descriptor::registerWithRegistry(registry);
    NatImuDataSchemaDescriptor::registerWithRegistry(registry);
    NatImuBulkDataSchemaDescriptor::registerWithRegistry(registry);
    NatKitNodeStatusV1Descriptor::registerWithRegistry(registry);
    NatKitPrimaryStatusV1Descriptor::registerWithRegistry(registry);
    NatMuseDataSchemaDescriptor::registerWithRegistry(registry);
    initialized = true;
  }
  return registry;
}

} // namespace core
} // namespace nat
