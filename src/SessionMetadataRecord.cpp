#include <libnatkit-core.hpp>

#ifdef SERVER
#include <nlohmann/json.hpp>
#else
#include <cJSON.h>
#endif

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <sstream>

namespace nat {
namespace core {

const std::string SessionMetadataRecord::name = "SessionMetadataRecord";
const uint32_t SessionMetadataRecord::recordTypeId = 1;
const uint16_t SessionMetadataRecord::recordVersion = 1;

static void ensureSessionMetadataRecordRegisteredForMetaRecord() {
  static bool registered = false;
  if (!registered) {
    nat::core::MetaRecord::registerMetaRecordType(
        nat::core::SessionMetadataRecord::recordTypeId,
        nat::core::SessionMetadataRecord::decodePayloadAll);
    registered = true;
  }
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

static void appendBinaryStringVector(
    std::vector<uint8_t> &buffer,
    const std::vector<std::string> &values) {
  const uint32_t count = static_cast<uint32_t>(values.size());
  const size_t originalSize = buffer.size();
  buffer.resize(originalSize + sizeof(uint32_t));
  char *data = reinterpret_cast<char *>(buffer.data());
  Binary::unsafeWriteAsBinaryToArray(data + originalSize, count);
  for (size_t i = 0; i < values.size(); ++i) {
    appendBinaryString(buffer, values[i]);
  }
}

static bool parseBinaryStringVector(
    const std::vector<uint8_t> &buffer,
    size_t &cursor,
    std::vector<std::string> &values) {
  if (cursor + sizeof(uint32_t) > buffer.size()) {
    return false;
  }

  char *data = reinterpret_cast<char *>(const_cast<uint8_t *>(buffer.data()));
  uint32_t count = 0;
  cursor += Binary::unsafeParseFromBinary(data + cursor, count);
  values.clear();
  values.reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    std::string next{};
    if (!parseBinaryString(buffer, cursor, next)) {
      return false;
    }
    values.push_back(next);
  }
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

#ifdef SERVER
static std::vector<std::string> parseStringVectorJson(const nlohmann::json &json) {
  std::vector<std::string> values{};
  if (!json.is_array()) {
    return values;
  }
  for (size_t i = 0; i < json.size(); ++i) {
    values.push_back(json[i].get<std::string>());
  }
  return values;
}
#else
static std::vector<std::string> parseStringVectorJson(cJSON *json) {
  std::vector<std::string> values{};
  if (json == nullptr || !cJSON_IsArray(json)) {
    return values;
  }
  const int count = cJSON_GetArraySize(json);
  for (int i = 0; i < count; ++i) {
    cJSON *item = cJSON_GetArrayItem(json, i);
    if (cJSON_IsString(item) && item->valuestring != nullptr) {
      values.push_back(std::string(item->valuestring));
    }
  }
  return values;
}
#endif

} // namespace

std::unique_ptr<std::vector<uint8_t>>
SessionMetadataRecord::encodeToBytes(const SerializationType &type) const {
  switch (type) {
  case SerializationType::Json: {
#ifdef SERVER
    nlohmann::json payload;
    payload["session_id"] = sessionId;
    payload["purpose"] = purpose;
    payload["participant_id"] = participantId;
    payload["protocol_id"] = protocolId;
    payload["device_ids"] = deviceIds;
    payload["tags"] = tags;
    payload["notes"] = notes;
    payload["created_at_us"] = createdAtUs;
    payload["updated_at_us"] = updatedAtUs;

    nlohmann::json root;
    root["record_type_id"] = getRecordTypeId();
    root["record_version"] = getRecordVersion();
    root["payload"] = payload;
    const auto jsonStr = root.dump();
    return nat::core::make_unique<std::vector<uint8_t>>(
        jsonStr.begin(), jsonStr.end());
#else
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "session_id", sessionId.c_str());
    cJSON_AddStringToObject(payload, "purpose", purpose.c_str());
    cJSON_AddStringToObject(payload, "participant_id", participantId.c_str());
    cJSON_AddStringToObject(payload, "protocol_id", protocolId.c_str());
    cJSON *deviceIdsJson = cJSON_AddArrayToObject(payload, "device_ids");
    for (size_t i = 0; i < deviceIds.size(); ++i) {
      cJSON_AddItemToArray(deviceIdsJson, cJSON_CreateString(deviceIds[i].c_str()));
    }
    cJSON *tagsJson = cJSON_AddArrayToObject(payload, "tags");
    for (size_t i = 0; i < tags.size(); ++i) {
      cJSON_AddItemToArray(tagsJson, cJSON_CreateString(tags[i].c_str()));
    }
    cJSON_AddStringToObject(payload, "notes", notes.c_str());
    cJSON_AddNumberToObject(payload, "created_at_us", static_cast<double>(createdAtUs));
    cJSON_AddNumberToObject(payload, "updated_at_us", static_cast<double>(updatedAtUs));

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
    appendBinaryString(payload, sessionId);
    appendBinaryString(payload, purpose);
    appendBinaryString(payload, participantId);
    appendBinaryString(payload, protocolId);
    appendBinaryStringVector(payload, deviceIds);
    appendBinaryStringVector(payload, tags);
    appendBinaryString(payload, notes);
    appendBinaryUint64(payload, createdAtUs);
    appendBinaryUint64(payload, updatedAtUs);
    std::vector<uint8_t> message =
        wrapBinaryMetaRecord(getRecordTypeId(), getRecordVersion(), payload);
    return nat::core::make_unique<std::vector<uint8_t>>(
        message.begin(), message.end());
  }
  default:
    assert(0);
  }
}

bool SessionMetadataRecord::isSerializationTypeSupported(
    const SerializationType type) const {
  switch (type) {
  case SerializationType::Json:
  case SerializationType::Binary:
    return true;
  default:
    return false;
  }
}

std::string SessionMetadataRecord::getName() const { return name; }

std::string SessionMetadataRecord::toString() const {
  std::ostringstream output;
  output << name << ": {session_id: " << sessionId
         << ", purpose: " << purpose
         << ", participant_id: " << participantId
         << ", protocol_id: " << protocolId
         << ", device_ids: " << deviceIds.size()
         << ", tags: " << tags.size()
         << ", created_at_us: " << createdAtUs
         << ", updated_at_us: " << updatedAtUs
         << "}";
  return output.str();
}

uint32_t SessionMetadataRecord::getRecordTypeId() const {
  return recordTypeId;
}

uint16_t SessionMetadataRecord::getRecordVersion() const {
  return recordVersion;
}

const std::string &SessionMetadataRecord::getSessionId() const {
  return sessionId;
}

const std::string &SessionMetadataRecord::getPurpose() const {
  return purpose;
}

const std::string &SessionMetadataRecord::getParticipantId() const {
  return participantId;
}

const std::string &SessionMetadataRecord::getProtocolId() const {
  return protocolId;
}

const std::vector<std::string> &SessionMetadataRecord::getDeviceIds() const {
  return deviceIds;
}

const std::vector<std::string> &SessionMetadataRecord::getTags() const {
  return tags;
}

const std::string &SessionMetadataRecord::getNotes() const {
  return notes;
}

uint64_t SessionMetadataRecord::getCreatedAtUs() const {
  return createdAtUs;
}

uint64_t SessionMetadataRecord::getUpdatedAtUs() const {
  return updatedAtUs;
}

Optional<std::unique_ptr<SessionMetadataRecord>>
SessionMetadataRecord::decodePayloadJson(
    const std::vector<uint8_t> &message,
    uint16_t payloadRecordVersion) {
  if (payloadRecordVersion != recordVersion) {
    return {};
  }

  const std::string jsonStr(message.begin(), message.end());
#ifdef SERVER
  const auto json = nlohmann::json::parse(jsonStr);
  const std::string sessionId = json["session_id"];
  const std::string purpose =
      json.find("purpose") != json.end() ? json["purpose"].get<std::string>() : "";
  const std::string participantId =
      json.find("participant_id") != json.end()
          ? json["participant_id"].get<std::string>()
          : "";
  const std::string protocolId =
      json.find("protocol_id") != json.end()
          ? json["protocol_id"].get<std::string>()
          : "";
  const std::vector<std::string> deviceIds =
      json.find("device_ids") != json.end()
          ? parseStringVectorJson(json["device_ids"])
          : std::vector<std::string>{};
  const std::vector<std::string> tags =
      json.find("tags") != json.end()
          ? parseStringVectorJson(json["tags"])
          : std::vector<std::string>{};
  const std::string notes =
      json.find("notes") != json.end() ? json["notes"].get<std::string>() : "";
  const uint64_t createdAtUs =
      json.find("created_at_us") != json.end()
          ? json["created_at_us"].get<uint64_t>()
          : 0;
  const uint64_t updatedAtUs =
      json.find("updated_at_us") != json.end()
          ? json["updated_at_us"].get<uint64_t>()
          : 0;
#else
  cJSON *json = cJSON_Parse(jsonStr.c_str());
  if (json == nullptr) {
    return {};
  }

  cJSON *sessionIdJson = cJSON_GetObjectItemCaseSensitive(json, "session_id");
  if (!cJSON_IsString(sessionIdJson) || sessionIdJson->valuestring == nullptr) {
    cJSON_Delete(json);
    return {};
  }

  cJSON *purposeJson = cJSON_GetObjectItemCaseSensitive(json, "purpose");
  cJSON *participantIdJson =
      cJSON_GetObjectItemCaseSensitive(json, "participant_id");
  cJSON *protocolIdJson =
      cJSON_GetObjectItemCaseSensitive(json, "protocol_id");
  cJSON *deviceIdsJson = cJSON_GetObjectItemCaseSensitive(json, "device_ids");
  cJSON *tagsJson = cJSON_GetObjectItemCaseSensitive(json, "tags");
  cJSON *notesJson = cJSON_GetObjectItemCaseSensitive(json, "notes");
  cJSON *createdAtUsJson =
      cJSON_GetObjectItemCaseSensitive(json, "created_at_us");
  cJSON *updatedAtUsJson =
      cJSON_GetObjectItemCaseSensitive(json, "updated_at_us");

  const std::string sessionId(sessionIdJson->valuestring);
  const std::string purpose =
      cJSON_IsString(purposeJson) && purposeJson->valuestring != nullptr
          ? std::string(purposeJson->valuestring)
          : "";
  const std::string participantId =
      cJSON_IsString(participantIdJson) &&
              participantIdJson->valuestring != nullptr
          ? std::string(participantIdJson->valuestring)
          : "";
  const std::string protocolId =
      cJSON_IsString(protocolIdJson) && protocolIdJson->valuestring != nullptr
          ? std::string(protocolIdJson->valuestring)
          : "";
  const std::vector<std::string> deviceIds = parseStringVectorJson(deviceIdsJson);
  const std::vector<std::string> tags = parseStringVectorJson(tagsJson);
  const std::string notes =
      cJSON_IsString(notesJson) && notesJson->valuestring != nullptr
          ? std::string(notesJson->valuestring)
          : "";
  const uint64_t createdAtUs =
      cJSON_IsNumber(createdAtUsJson)
          ? static_cast<uint64_t>(createdAtUsJson->valuedouble)
          : 0;
  const uint64_t updatedAtUs =
      cJSON_IsNumber(updatedAtUsJson)
          ? static_cast<uint64_t>(updatedAtUsJson->valuedouble)
          : 0;
  cJSON_Delete(json);
#endif

  return nat::core::make_unique<SessionMetadataRecord>(
      sessionId,
      purpose,
      participantId,
      protocolId,
      deviceIds,
      tags,
      notes,
      createdAtUs,
      updatedAtUs);
}

Optional<std::unique_ptr<SessionMetadataRecord>>
SessionMetadataRecord::decodePayloadBinary(
    const std::vector<uint8_t> &message,
    uint16_t payloadRecordVersion) {
  if (payloadRecordVersion != recordVersion) {
    return {};
  }

  size_t cursor = 0;
  std::string sessionId{};
  std::string purpose{};
  std::string participantId{};
  std::string protocolId{};
  std::vector<std::string> deviceIds{};
  std::vector<std::string> tags{};
  std::string notes{};
  uint64_t createdAtUs = 0;
  uint64_t updatedAtUs = 0;

  if (!parseBinaryString(message, cursor, sessionId) ||
      !parseBinaryString(message, cursor, purpose) ||
      !parseBinaryString(message, cursor, participantId) ||
      !parseBinaryString(message, cursor, protocolId) ||
      !parseBinaryStringVector(message, cursor, deviceIds) ||
      !parseBinaryStringVector(message, cursor, tags) ||
      !parseBinaryString(message, cursor, notes) ||
      !parseBinaryUint64(message, cursor, createdAtUs) ||
      !parseBinaryUint64(message, cursor, updatedAtUs)) {
    return {};
  }

  return nat::core::make_unique<SessionMetadataRecord>(
      sessionId,
      purpose,
      participantId,
      protocolId,
      deviceIds,
      tags,
      notes,
      createdAtUs,
      updatedAtUs);
}

Optional<std::unique_ptr<MetaRecord>> SessionMetadataRecord::decodePayloadAll(
    const std::vector<uint8_t> &message,
    const SerializationType &type,
    uint16_t payloadRecordVersion) {
  switch (type) {
  case SerializationType::Json: {
    auto decoded = decodePayloadJson(message, payloadRecordVersion);
    if (!decoded.has_value()) {
      return {};
    }
    std::unique_ptr<MetaRecord> castedDecoded(std::move(decoded.value()));
    return Optional<std::unique_ptr<MetaRecord>>{std::move(castedDecoded)};
  }
  case SerializationType::Binary: {
    auto decoded = decodePayloadBinary(message, payloadRecordVersion);
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

void SessionMetadataRecord::registerWithRegistry(Registry &registry) {
  (void)registry;
  ensureSessionMetadataRecordRegisteredForMetaRecord();
}

} // namespace core
} // namespace nat

extern "C" {

int nat_session_metadata_record_encode_json(const uint8_t *payload_json,
                                            size_t payload_json_size,
                                            uint8_t **out_message,
                                            size_t *out_message_size) {
  if (payload_json == nullptr || out_message == nullptr ||
      out_message_size == nullptr) {
    return 1;
  }

  const std::vector<uint8_t> payload(payload_json, payload_json + payload_json_size);
  nat::core::ensureSessionMetadataRecordRegisteredForMetaRecord();
  auto recordMaybe = nat::core::SessionMetadataRecord::decodePayloadJson(
      payload,
      nat::core::SessionMetadataRecord::recordVersion);
  if (!recordMaybe.has_value()) {
    return 2;
  }

  auto encoded = recordMaybe.value()->encodeToBytes(nat::core::SerializationType::Json);
  if (!encoded) {
    return 3;
  }

  uint8_t *buffer = reinterpret_cast<uint8_t *>(
      std::malloc(encoded->size() + 1));
  if (buffer == nullptr) {
    return 4;
  }

  if (!encoded->empty()) {
    std::memcpy(buffer, encoded->data(), encoded->size());
  }
  buffer[encoded->size()] = 0;
  *out_message = buffer;
  *out_message_size = encoded->size();
  return 0;
}

int nat_session_metadata_record_decode_json(const uint8_t *message,
                                            size_t message_size,
                                            uint8_t **out_payload_json,
                                            size_t *out_payload_json_size) {
  if (message == nullptr || out_payload_json == nullptr ||
      out_payload_json_size == nullptr) {
    return 1;
  }

  const std::vector<uint8_t> encoded(message, message + message_size);
  nat::core::ensureSessionMetadataRecordRegisteredForMetaRecord();
  auto recordMaybe = nat::core::MetaRecord::decodeAll(
      encoded,
      nat::core::SerializationType::Json);
  if (!recordMaybe.has_value()) {
    return 2;
  }

  nat::core::SessionMetadataRecord *record =
      dynamic_cast<nat::core::SessionMetadataRecord *>(recordMaybe.value().get());
  if (record == nullptr) {
    return 3;
  }

#ifdef SERVER
  nlohmann::json payload;
  payload["session_id"] = record->getSessionId();
  payload["purpose"] = record->getPurpose();
  payload["participant_id"] = record->getParticipantId();
  payload["protocol_id"] = record->getProtocolId();
  payload["device_ids"] = record->getDeviceIds();
  payload["tags"] = record->getTags();
  payload["notes"] = record->getNotes();
  payload["created_at_us"] = record->getCreatedAtUs();
  payload["updated_at_us"] = record->getUpdatedAtUs();
  const auto payloadStr = payload.dump();
  const size_t payloadSize = payloadStr.size();
  uint8_t *buffer = reinterpret_cast<uint8_t *>(std::malloc(payloadSize + 1));
  if (buffer == nullptr) {
    return 4;
  }
  if (payloadSize > 0) {
    std::memcpy(buffer, payloadStr.data(), payloadSize);
  }
  buffer[payloadSize] = 0;
  *out_payload_json = buffer;
  *out_payload_json_size = payloadSize;
  return 0;
#else
  cJSON *payload = cJSON_CreateObject();
  cJSON_AddStringToObject(payload, "session_id", record->getSessionId().c_str());
  cJSON_AddStringToObject(payload, "purpose", record->getPurpose().c_str());
  cJSON_AddStringToObject(payload, "participant_id", record->getParticipantId().c_str());
  cJSON_AddStringToObject(payload, "protocol_id", record->getProtocolId().c_str());
  cJSON *deviceIdsJson = cJSON_AddArrayToObject(payload, "device_ids");
  const auto &deviceIds = record->getDeviceIds();
  for (size_t i = 0; i < deviceIds.size(); ++i) {
    cJSON_AddItemToArray(deviceIdsJson, cJSON_CreateString(deviceIds[i].c_str()));
  }
  cJSON *tagsJson = cJSON_AddArrayToObject(payload, "tags");
  const auto &tags = record->getTags();
  for (size_t i = 0; i < tags.size(); ++i) {
    cJSON_AddItemToArray(tagsJson, cJSON_CreateString(tags[i].c_str()));
  }
  cJSON_AddStringToObject(payload, "notes", record->getNotes().c_str());
  cJSON_AddNumberToObject(payload, "created_at_us", static_cast<double>(record->getCreatedAtUs()));
  cJSON_AddNumberToObject(payload, "updated_at_us", static_cast<double>(record->getUpdatedAtUs()));
  char *payloadChars = cJSON_PrintUnformatted(payload);
  cJSON_Delete(payload);
  if (payloadChars == nullptr) {
    return 4;
  }
  const size_t payloadSize = std::strlen(payloadChars);
  uint8_t *buffer = reinterpret_cast<uint8_t *>(std::malloc(payloadSize + 1));
  if (buffer == nullptr) {
    std::free(payloadChars);
    return 4;
  }
  if (payloadSize > 0) {
    std::memcpy(buffer, payloadChars, payloadSize);
  }
  buffer[payloadSize] = 0;
  std::free(payloadChars);
  *out_payload_json = buffer;
  *out_payload_json_size = payloadSize;
  return 0;
#endif
}

void nat_free_bytes(uint8_t *buffer) {
  if (buffer != nullptr) {
    std::free(buffer);
  }
}

}
