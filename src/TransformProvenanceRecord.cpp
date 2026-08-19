#include <libnatkit-core.hpp>

#ifdef SERVER
#include <nlohmann/json.hpp>
#else
#include <cJSON.h>
#endif

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <sstream>

namespace nat {
namespace core {

const std::string TransformProvenanceRecord::name = "TransformProvenanceRecord";
const uint32_t TransformProvenanceRecord::recordTypeId = 2;
const uint16_t TransformProvenanceRecord::recordVersion = 1;

static void ensureTransformProvenanceRecordRegisteredForMetaRecord() {
  static std::once_flag registeredFlag;
  std::call_once(registeredFlag, [] {
    nat::core::MetaRecord::registerMetaRecordType(
        nat::core::TransformProvenanceRecord::recordTypeId,
        nat::core::TransformProvenanceRecord::decodePayloadAll);
  });
}

namespace {

static void appendBinaryString(
    std::vector<uint8_t> &buffer,
    const std::string &value) {
  const uint32_t length = static_cast<uint32_t>(value.size());
  const size_t originalSize = buffer.size();
  buffer.resize(originalSize + sizeof(uint32_t) + length);
  char *data = reinterpret_cast<char *>(buffer.data());
  Binary::unsafeWriteAsBinaryToArray(data + originalSize, length);
  if (length > 0) {
    memcpy(data + originalSize + sizeof(uint32_t), value.data(), length);
  }
}

static bool parseBinaryString(
    const std::vector<uint8_t> &buffer,
    size_t &cursor,
    std::string &value) {
  if (cursor + sizeof(uint32_t) > buffer.size()) {
    return false;
  }

  char *data = reinterpret_cast<char *>(const_cast<uint8_t *>(buffer.data()));
  uint32_t length = 0;
  cursor += Binary::unsafeParseFromBinary(data + cursor, length);
  if (cursor + length > buffer.size()) {
    return false;
  }

  value.assign(
      reinterpret_cast<const char *>(buffer.data() + cursor),
      reinterpret_cast<const char *>(buffer.data() + cursor + length));
  cursor += length;
  return true;
}

static bool parseBinaryUint64(
    const std::vector<uint8_t> &buffer,
    size_t &cursor,
    uint64_t &value) {
  if (cursor + sizeof(uint64_t) > buffer.size()) {
    return false;
  }
  char *data = reinterpret_cast<char *>(const_cast<uint8_t *>(buffer.data()));
  cursor += Binary::unsafeParseFromBinary(data + cursor, value);
  return true;
}

static void appendBinaryUint64(
    std::vector<uint8_t> &buffer,
    uint64_t value) {
  const size_t originalSize = buffer.size();
  buffer.resize(originalSize + sizeof(uint64_t));
  char *data = reinterpret_cast<char *>(buffer.data());
  Binary::unsafeWriteAsBinaryToArray(data + originalSize, value);
}

static std::vector<uint8_t> wrapBinaryMetaRecord(
    uint32_t recordTypeId,
    uint16_t recordVersion,
    const std::vector<uint8_t> &payload) {
  std::vector<uint8_t> message{};
  message.resize(sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t));
  char *data = reinterpret_cast<char *>(message.data());
  size_t cursor = 0;
  cursor += Binary::unsafeWriteAsBinaryToArray(data + cursor, recordTypeId);
  cursor += Binary::unsafeWriteAsBinaryToArray(data + cursor, recordVersion);
  const uint16_t flags = 0;
  cursor += Binary::unsafeWriteAsBinaryToArray(data + cursor, flags);
  (void)cursor;
  message.insert(message.end(), payload.begin(), payload.end());
  return message;
}

} // namespace

TransformProvenanceRecord::TransformProvenanceRecord(
    const std::string &outputIdentifier,
    uint64_t outputStreamId,
    const std::string &outputSchemaName,
    const std::string &outputTopic,
    uint64_t sourceStreamId,
    const std::string &sourceSchemaName,
    const std::string &sourceTopic,
    const std::string &transformKind,
    const std::string &inputMappingId,
    const std::string &configJson,
    uint64_t createdAtUs)
    : outputIdentifier(outputIdentifier),
      outputStreamId(outputStreamId),
      outputSchemaName(outputSchemaName),
      outputTopic(outputTopic),
      sourceStreamId(sourceStreamId),
      sourceSchemaName(sourceSchemaName),
      sourceTopic(sourceTopic),
      transformKind(transformKind),
      inputMappingId(inputMappingId),
      configJson(configJson),
      createdAtUs(createdAtUs) {}

std::unique_ptr<std::vector<uint8_t>>
TransformProvenanceRecord::encodeToBytes(const SerializationType &type) const {
  switch (type) {
  case SerializationType::Json: {
#ifdef SERVER
    nlohmann::json payload;
    payload["output_identifier"] = outputIdentifier;
    payload["output_stream_id"] = outputStreamId;
    payload["output_schema_name"] = outputSchemaName;
    payload["output_topic"] = outputTopic;
    payload["source_stream_id"] = sourceStreamId;
    payload["source_schema_name"] = sourceSchemaName;
    payload["source_topic"] = sourceTopic;
    payload["transform_kind"] = transformKind;
    payload["input_mapping_id"] = inputMappingId;
    payload["config_json"] = configJson;
    payload["created_at_us"] = createdAtUs;

    nlohmann::json root;
    root["record_type_id"] = getRecordTypeId();
    root["record_version"] = getRecordVersion();
    root["payload"] = payload;
    const auto jsonStr = root.dump();
    return nat::core::make_unique<std::vector<uint8_t>>(
        jsonStr.begin(), jsonStr.end());
#else
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "output_identifier", outputIdentifier.c_str());
    cJSON_AddNumberToObject(payload, "output_stream_id", static_cast<double>(outputStreamId));
    cJSON_AddStringToObject(payload, "output_schema_name", outputSchemaName.c_str());
    cJSON_AddStringToObject(payload, "output_topic", outputTopic.c_str());
    cJSON_AddNumberToObject(payload, "source_stream_id", static_cast<double>(sourceStreamId));
    cJSON_AddStringToObject(payload, "source_schema_name", sourceSchemaName.c_str());
    cJSON_AddStringToObject(payload, "source_topic", sourceTopic.c_str());
    cJSON_AddStringToObject(payload, "transform_kind", transformKind.c_str());
    cJSON_AddStringToObject(payload, "input_mapping_id", inputMappingId.c_str());
    cJSON_AddStringToObject(payload, "config_json", configJson.c_str());
    cJSON_AddNumberToObject(payload, "created_at_us", static_cast<double>(createdAtUs));

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "record_type_id", static_cast<double>(getRecordTypeId()));
    cJSON_AddNumberToObject(root, "record_version", static_cast<double>(getRecordVersion()));
    cJSON_AddItemToObject(root, "payload", payload);
    const auto jsonStr = std::string(cJSON_PrintUnformatted(root));
    cJSON_Delete(root);
    return nat::core::make_unique<std::vector<uint8_t>>(
        jsonStr.begin(), jsonStr.end());
#endif
  }
  case SerializationType::Binary: {
    std::vector<uint8_t> payload{};
    appendBinaryString(payload, outputIdentifier);
    appendBinaryUint64(payload, outputStreamId);
    appendBinaryString(payload, outputSchemaName);
    appendBinaryString(payload, outputTopic);
    appendBinaryUint64(payload, sourceStreamId);
    appendBinaryString(payload, sourceSchemaName);
    appendBinaryString(payload, sourceTopic);
    appendBinaryString(payload, transformKind);
    appendBinaryString(payload, inputMappingId);
    appendBinaryString(payload, configJson);
    appendBinaryUint64(payload, createdAtUs);
    std::vector<uint8_t> message =
        wrapBinaryMetaRecord(getRecordTypeId(), getRecordVersion(), payload);
    return nat::core::make_unique<std::vector<uint8_t>>(
        message.begin(), message.end());
  }
  default:
    assert(0);
  }
}

bool TransformProvenanceRecord::isSerializationTypeSupported(
    const SerializationType type) const {
  switch (type) {
  case SerializationType::Json:
  case SerializationType::Binary:
    return true;
  default:
    return false;
  }
}

std::string TransformProvenanceRecord::getName() const { return name; }

std::string TransformProvenanceRecord::toString() const {
  std::ostringstream output;
  output << name
         << ": {output_identifier: " << outputIdentifier
         << ", output_stream_id: " << outputStreamId
         << ", source_stream_id: " << sourceStreamId
         << ", transform_kind: " << transformKind
         << ", input_mapping_id: " << inputMappingId
         << ", created_at_us: " << createdAtUs
         << "}";
  return output.str();
}

uint32_t TransformProvenanceRecord::getRecordTypeId() const {
  return recordTypeId;
}

uint16_t TransformProvenanceRecord::getRecordVersion() const {
  return recordVersion;
}

const std::string &TransformProvenanceRecord::getOutputIdentifier() const {
  return outputIdentifier;
}

uint64_t TransformProvenanceRecord::getOutputStreamId() const {
  return outputStreamId;
}

const std::string &TransformProvenanceRecord::getOutputSchemaName() const {
  return outputSchemaName;
}

const std::string &TransformProvenanceRecord::getOutputTopic() const {
  return outputTopic;
}

uint64_t TransformProvenanceRecord::getSourceStreamId() const {
  return sourceStreamId;
}

const std::string &TransformProvenanceRecord::getSourceSchemaName() const {
  return sourceSchemaName;
}

const std::string &TransformProvenanceRecord::getSourceTopic() const {
  return sourceTopic;
}

const std::string &TransformProvenanceRecord::getTransformKind() const {
  return transformKind;
}

const std::string &TransformProvenanceRecord::getInputMappingId() const {
  return inputMappingId;
}

const std::string &TransformProvenanceRecord::getConfigJson() const {
  return configJson;
}

uint64_t TransformProvenanceRecord::getCreatedAtUs() const {
  return createdAtUs;
}

Optional<std::unique_ptr<TransformProvenanceRecord>>
TransformProvenanceRecord::decodePayloadJson(
    const std::vector<uint8_t> &message,
    uint16_t version) {
  if (version != recordVersion) {
    return {};
  }

  const std::string payloadStr(message.begin(), message.end());
#ifdef SERVER
  try {
    const auto payload = nlohmann::json::parse(payloadStr);
    return nat::core::make_unique<TransformProvenanceRecord>(
        payload.value("output_identifier", std::string{}),
        payload.value("output_stream_id", static_cast<uint64_t>(0)),
        payload.value("output_schema_name", std::string{}),
        payload.value("output_topic", std::string{}),
        payload.value("source_stream_id", static_cast<uint64_t>(0)),
        payload.value("source_schema_name", std::string{}),
        payload.value("source_topic", std::string{}),
        payload.value("transform_kind", std::string{}),
        payload.value("input_mapping_id", std::string{}),
        payload.value("config_json", std::string{}),
        payload.value("created_at_us", static_cast<uint64_t>(0)));
  } catch (const std::exception &) {
    return {};
  }
#else
  cJSON *payload = cJSON_Parse(payloadStr.c_str());
  if (payload == nullptr) {
    return {};
  }

  const auto readString = [payload](const char *field) -> std::string {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(payload, field);
    if (cJSON_IsString(item) && item->valuestring != nullptr) {
      return std::string(item->valuestring);
    }
    return std::string{};
  };

  const auto readUint64 = [payload](const char *field) -> uint64_t {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(payload, field);
    if (cJSON_IsNumber(item)) {
      return static_cast<uint64_t>(item->valuedouble);
    }
    return 0;
  };

  auto record = nat::core::make_unique<TransformProvenanceRecord>(
      readString("output_identifier"),
      readUint64("output_stream_id"),
      readString("output_schema_name"),
      readString("output_topic"),
      readUint64("source_stream_id"),
      readString("source_schema_name"),
      readString("source_topic"),
      readString("transform_kind"),
      readString("input_mapping_id"),
      readString("config_json"),
      readUint64("created_at_us"));
  cJSON_Delete(payload);
  return record;
#endif
}

Optional<std::unique_ptr<TransformProvenanceRecord>>
TransformProvenanceRecord::decodePayloadBinary(
    const std::vector<uint8_t> &message,
    uint16_t version) {
  if (version != recordVersion) {
    return {};
  }

  size_t cursor = 0;
  std::string outputIdentifier{};
  uint64_t outputStreamId = 0;
  std::string outputSchemaName{};
  std::string outputTopic{};
  uint64_t sourceStreamId = 0;
  std::string sourceSchemaName{};
  std::string sourceTopic{};
  std::string transformKind{};
  std::string inputMappingId{};
  std::string configJson{};
  uint64_t createdAtUs = 0;

  if (!parseBinaryString(message, cursor, outputIdentifier) ||
      !parseBinaryUint64(message, cursor, outputStreamId) ||
      !parseBinaryString(message, cursor, outputSchemaName) ||
      !parseBinaryString(message, cursor, outputTopic) ||
      !parseBinaryUint64(message, cursor, sourceStreamId) ||
      !parseBinaryString(message, cursor, sourceSchemaName) ||
      !parseBinaryString(message, cursor, sourceTopic) ||
      !parseBinaryString(message, cursor, transformKind) ||
      !parseBinaryString(message, cursor, inputMappingId) ||
      !parseBinaryString(message, cursor, configJson) ||
      !parseBinaryUint64(message, cursor, createdAtUs)) {
    return {};
  }

  return nat::core::make_unique<TransformProvenanceRecord>(
      outputIdentifier,
      outputStreamId,
      outputSchemaName,
      outputTopic,
      sourceStreamId,
      sourceSchemaName,
      sourceTopic,
      transformKind,
      inputMappingId,
      configJson,
      createdAtUs);
}

Optional<std::unique_ptr<MetaRecord>> TransformProvenanceRecord::decodePayloadAll(
    const std::vector<uint8_t> &message,
    const SerializationType &type,
    uint16_t version) {
  switch (type) {
  case SerializationType::Json: {
    auto decoded = decodePayloadJson(message, version);
    if (!decoded.has_value()) {
      return {};
    }
    std::unique_ptr<MetaRecord> castedDecoded(std::move(decoded.value()));
    return Optional<std::unique_ptr<MetaRecord>>{std::move(castedDecoded)};
  }
  case SerializationType::Binary: {
    auto decoded = decodePayloadBinary(message, version);
    if (!decoded.has_value()) {
      return {};
    }
    std::unique_ptr<MetaRecord> castedDecoded(std::move(decoded.value()));
    return Optional<std::unique_ptr<MetaRecord>>{std::move(castedDecoded)};
  }
  default:
    return {};
  }
}

void TransformProvenanceRecord::registerWithRegistry(Registry &registry) {
  (void)registry;
  ensureTransformProvenanceRecordRegisteredForMetaRecord();
}

} // namespace core
} // namespace nat
