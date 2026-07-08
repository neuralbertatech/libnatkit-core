#include <libnatkit-core.hpp>

#ifdef SERVER
#include <nlohmann/json.hpp>
#else
#include <cJSON.h>
#endif

#include <cassert>
#include <cstdlib>
#include <cstring>

namespace nat {
namespace core {

const std::string MetaRecord::name = "MetaRecord";

namespace {

static Optional<std::shared_ptr<Schema>> convertUniqueMetaRecord(
    Optional<std::unique_ptr<MetaRecord>> &&unique) {
  if (!unique.has_value()) {
    return {};
  }
  std::shared_ptr<Schema> shared(std::move(unique.value()));
  return Optional<std::shared_ptr<Schema>>{shared};
}

static std::unordered_map<uint32_t, meta_record_decoder_t> &metaRecordDecoders() {
  static std::unordered_map<uint32_t, meta_record_decoder_t> decoders{};
  return decoders;
}

static bool readBinaryHeader(
    const std::vector<uint8_t> &message,
    uint32_t &recordTypeId,
    uint16_t &recordVersion,
    std::vector<uint8_t> &payload) {
  const size_t headerSize =
      sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t);
  if (message.size() < headerSize) {
    return false;
  }

  size_t cursor = 0;
  char *data = reinterpret_cast<char *>(const_cast<uint8_t *>(message.data()));
  cursor += Binary::unsafeParseFromBinary(data + cursor, recordTypeId);
  cursor += Binary::unsafeParseFromBinary(data + cursor, recordVersion);
  uint16_t flags = 0;
  cursor += Binary::unsafeParseFromBinary(data + cursor, flags);
  (void)flags;

  payload.assign(message.begin() + cursor, message.end());
  return true;
}

#ifdef SERVER
static Optional<std::unique_ptr<MetaRecord>> decodeJsonMessage(
    const std::vector<uint8_t> &message) {
  const std::string jsonStr(message.begin(), message.end());
  const auto json = nlohmann::json::parse(jsonStr);
  const uint32_t recordTypeId = json["record_type_id"];
  const uint16_t recordVersion = json["record_version"];
  const auto payloadJson = json["payload"];
  const auto payloadStr = payloadJson.dump();
  const std::vector<uint8_t> payload(payloadStr.begin(), payloadStr.end());
  const auto search = metaRecordDecoders().find(recordTypeId);
  if (search == metaRecordDecoders().end()) {
    return {};
  }
  return search->second(payload, SerializationType::Json, recordVersion);
}
#else
static Optional<std::unique_ptr<MetaRecord>> decodeJsonMessage(
    const std::vector<uint8_t> &message) {
  const std::string jsonStr(message.begin(), message.end());
  cJSON *json = cJSON_Parse(jsonStr.c_str());
  if (json == nullptr) {
    return {};
  }

  cJSON *recordTypeIdJson =
      cJSON_GetObjectItemCaseSensitive(json, "record_type_id");
  cJSON *recordVersionJson =
      cJSON_GetObjectItemCaseSensitive(json, "record_version");
  cJSON *payloadJson = cJSON_GetObjectItemCaseSensitive(json, "payload");
  if (recordTypeIdJson == nullptr || recordVersionJson == nullptr ||
      payloadJson == nullptr) {
    cJSON_Delete(json);
    return {};
  }

  const uint32_t recordTypeId =
      static_cast<uint32_t>(recordTypeIdJson->valuedouble);
  const uint16_t recordVersion =
      static_cast<uint16_t>(recordVersionJson->valuedouble);
  char *payloadChars = cJSON_PrintUnformatted(payloadJson);
  if (payloadChars == nullptr) {
    cJSON_Delete(json);
    return {};
  }

  const std::string payloadStr(payloadChars);
  free(payloadChars);
  cJSON_Delete(json);

  const auto search = metaRecordDecoders().find(recordTypeId);
  if (search == metaRecordDecoders().end()) {
    return {};
  }

  const std::vector<uint8_t> payload(payloadStr.begin(), payloadStr.end());
  return search->second(payload, SerializationType::Json, recordVersion);
}
#endif

static Optional<std::unique_ptr<MetaRecord>> decodeBinaryMessage(
    const std::vector<uint8_t> &message) {
  uint32_t recordTypeId = 0;
  uint16_t recordVersion = 0;
  std::vector<uint8_t> payload{};
  if (!readBinaryHeader(message, recordTypeId, recordVersion, payload)) {
    return {};
  }

  const auto search = metaRecordDecoders().find(recordTypeId);
  if (search == metaRecordDecoders().end()) {
    return {};
  }
  return search->second(payload, SerializationType::Binary, recordVersion);
}

} // namespace

Optional<std::unique_ptr<MetaRecord>> MetaRecord::decodeAll(
    const std::vector<uint8_t> &message,
    const SerializationType &type) {
  switch (type) {
  case SerializationType::Json:
    return decodeJsonMessage(message);
  case SerializationType::Binary:
    return decodeBinaryMessage(message);
  default:
    return {};
  }
}

Optional<std::shared_ptr<Schema>> MetaRecord::tryDecode(
    const std::vector<uint8_t> &message,
    const SerializationType &type) const {
  return sharedDecodeAll(message, type);
}

void MetaRecord::registerWithRegistry(Registry &registry) {
  registry.registerDecoder(name, SerializationType::Json, uniqueDecodeAll);
  registry.registerDecoder(name, SerializationType::Binary, uniqueDecodeAll);
}

void MetaRecord::registerMetaRecordType(
    uint32_t recordTypeId,
    const meta_record_decoder_t &decoder) {
  auto &decoders = metaRecordDecoders();
  const auto search = decoders.find(recordTypeId);
  if (search == decoders.end()) {
    decoders.emplace(recordTypeId, decoder);
  }
}

Optional<std::shared_ptr<Schema>> MetaRecord::sharedDecodeAll(
    const std::vector<uint8_t> &message,
    const SerializationType &type) {
  return convertUniqueMetaRecord(decodeAll(message, type));
}

Optional<std::unique_ptr<Schema>> MetaRecord::uniqueDecodeAll(
    const std::vector<uint8_t> &message,
    const SerializationType &type) {
  auto decoded = decodeAll(message, type);
  if (!decoded.has_value()) {
    return {};
  }
  std::unique_ptr<Schema> castedDecoded(std::move(decoded.value()));
  return Optional<std::unique_ptr<Schema>>{std::move(castedDecoded)};
}

} // namespace core
} // namespace nat
