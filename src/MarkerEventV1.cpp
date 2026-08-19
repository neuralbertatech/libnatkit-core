#include <libnatkit-core.hpp>

#ifdef SERVER
#include <nlohmann/json.hpp>
#else
#include <cJSON.h>
#endif

#include <cassert>
#include <cstring>
#include <sstream>

namespace nat {
namespace core {

const std::string MarkerEventV1::name = "MarkerEventV1";
const std::string MarkerEventV1::schemaVersion = "marker.event.v1";

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

static void appendBinaryUint64(
    std::vector<uint8_t> &buffer,
    uint64_t value) {
  const size_t originalSize = buffer.size();
  buffer.resize(originalSize + sizeof(uint64_t));
  char *data = reinterpret_cast<char *>(buffer.data());
  Binary::unsafeWriteAsBinaryToArray(data + originalSize, value);
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

#ifdef SERVER
static nlohmann::json parseAttributesJsonOrObject(const std::string &value) {
  if (value.empty()) {
    return nlohmann::json::object();
  }

  try {
    return nlohmann::json::parse(value);
  } catch (const std::exception &) {
    return nlohmann::json::object();
  }
}
#else
static cJSON *parseAttributesJsonOrObject(const std::string &value) {
  if (value.empty()) {
    return cJSON_CreateObject();
  }

  cJSON *parsed = cJSON_Parse(value.c_str());
  if (parsed == nullptr) {
    return cJSON_CreateObject();
  }
  return parsed;
}
#endif

} // namespace

std::unique_ptr<std::vector<uint8_t>>
MarkerEventV1::encodeToBytes(const SerializationType &type) const {
  switch (type) {
  case SerializationType::Json: {
#ifdef SERVER
    nlohmann::json root;
    root["schema_version"] = schemaVersion;
    root["session_id"] = sessionId;
    root["marker_type"] = markerType;
    root["marker_id"] = markerId;
    root["event"] = event;
    root["label"] = label;
    root["emitted_at_us"] = emittedAtUs;
    root["attributes"] = parseAttributesJsonOrObject(attributesJson);
    const auto jsonStr = root.dump();
    return nat::core::make_unique<std::vector<uint8_t>>(
        jsonStr.begin(), jsonStr.end());
#else
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "schema_version", schemaVersion.c_str());
    cJSON_AddStringToObject(root, "session_id", sessionId.c_str());
    cJSON_AddStringToObject(root, "marker_type", markerType.c_str());
    cJSON_AddStringToObject(root, "marker_id", markerId.c_str());
    cJSON_AddStringToObject(root, "event", event.c_str());
    cJSON_AddStringToObject(root, "label", label.c_str());
    cJSON_AddNumberToObject(root, "emitted_at_us", static_cast<double>(emittedAtUs));
    cJSON *attributes = parseAttributesJsonOrObject(attributesJson);
    cJSON_AddItemToObject(root, "attributes", attributes);
    const auto jsonStr = std::string(cJSON_PrintUnformatted(root));
    cJSON_Delete(root);
    return nat::core::make_unique<std::vector<uint8_t>>(
        jsonStr.begin(), jsonStr.end());
#endif
  }
  case SerializationType::Binary: {
    std::vector<uint8_t> payload{};
    appendBinaryString(payload, sessionId);
    appendBinaryString(payload, markerType);
    appendBinaryString(payload, markerId);
    appendBinaryString(payload, event);
    appendBinaryString(payload, label);
    appendBinaryUint64(payload, emittedAtUs);
    appendBinaryString(payload, attributesJson);
    return nat::core::make_unique<std::vector<uint8_t>>(
        payload.begin(), payload.end());
  }
  default:
    assert(0);
    return nat::core::make_unique<std::vector<uint8_t>>();
  }
}

bool MarkerEventV1::isSerializationTypeSupported(
    const SerializationType type) const {
  switch (type) {
  case SerializationType::Json:
  case SerializationType::Binary:
    return true;
  default:
    return false;
  }
}

std::string MarkerEventV1::getName() const { return name; }

std::string MarkerEventV1::toString() const {
  std::ostringstream builder;
  builder << "MarkerEventV1{session_id=\"" << sessionId
          << "\", marker_type=\"" << markerType
          << "\", marker_id=\"" << markerId
          << "\", event=\"" << event
          << "\", label=\"" << label
          << "\", emitted_at_us=" << emittedAtUs << "}";
  return builder.str();
}

const std::string &MarkerEventV1::getSessionId() const { return sessionId; }

const std::string &MarkerEventV1::getMarkerType() const { return markerType; }

const std::string &MarkerEventV1::getMarkerId() const { return markerId; }

const std::string &MarkerEventV1::getEvent() const { return event; }

const std::string &MarkerEventV1::getLabel() const { return label; }

uint64_t MarkerEventV1::getEmittedAtUs() const { return emittedAtUs; }

const std::string &MarkerEventV1::getAttributesJson() const {
  return attributesJson;
}

Optional<std::unique_ptr<MarkerEventV1>>
MarkerEventV1::decodeJson(const std::vector<uint8_t> &message) {
  const auto rawJson =
      std::string(reinterpret_cast<const char *>(message.data()), message.size());

#ifdef SERVER
  try {
    const auto root = nlohmann::json::parse(rawJson);
    const std::string attributes =
        root.contains("attributes") ? root["attributes"].dump() : "{}";
    return nat::core::make_unique<MarkerEventV1>(
        root.value("session_id", std::string{}),
        root.value("marker_type", std::string{}),
        root.value("marker_id", std::string{}),
        root.value("event", std::string{}),
        root.value("label", std::string{}),
        root.value("emitted_at_us", static_cast<uint64_t>(0)),
        attributes);
  } catch (const std::exception &) {
    return {};
  }
#else
  cJSON *root = cJSON_Parse(rawJson.c_str());
  if (root == nullptr) {
    return {};
  }

  cJSON *sessionIdJson = cJSON_GetObjectItemCaseSensitive(root, "session_id");
  cJSON *markerTypeJson = cJSON_GetObjectItemCaseSensitive(root, "marker_type");
  cJSON *markerIdJson = cJSON_GetObjectItemCaseSensitive(root, "marker_id");
  cJSON *eventJson = cJSON_GetObjectItemCaseSensitive(root, "event");
  cJSON *labelJson = cJSON_GetObjectItemCaseSensitive(root, "label");
  cJSON *emittedAtUsJson =
      cJSON_GetObjectItemCaseSensitive(root, "emitted_at_us");
  cJSON *attributesJsonNode =
      cJSON_GetObjectItemCaseSensitive(root, "attributes");

  const auto attributes =
      attributesJsonNode == nullptr
          ? std::string("{}")
          : std::string(cJSON_PrintUnformatted(attributesJsonNode));
  const auto emittedAtUs = emittedAtUsJson != nullptr && cJSON_IsNumber(emittedAtUsJson)
                               ? static_cast<uint64_t>(emittedAtUsJson->valuedouble)
                               : static_cast<uint64_t>(0);
  auto decoded = nat::core::make_unique<MarkerEventV1>(
      sessionIdJson != nullptr && cJSON_IsString(sessionIdJson) && sessionIdJson->valuestring != nullptr
          ? std::string(sessionIdJson->valuestring)
          : std::string{},
      markerTypeJson != nullptr && cJSON_IsString(markerTypeJson) && markerTypeJson->valuestring != nullptr
          ? std::string(markerTypeJson->valuestring)
          : std::string{},
      markerIdJson != nullptr && cJSON_IsString(markerIdJson) && markerIdJson->valuestring != nullptr
          ? std::string(markerIdJson->valuestring)
          : std::string{},
      eventJson != nullptr && cJSON_IsString(eventJson) && eventJson->valuestring != nullptr
          ? std::string(eventJson->valuestring)
          : std::string{},
      labelJson != nullptr && cJSON_IsString(labelJson) && labelJson->valuestring != nullptr
          ? std::string(labelJson->valuestring)
          : std::string{},
      emittedAtUs,
      attributes);
  cJSON_Delete(root);
  return decoded;
#endif
}

Optional<std::unique_ptr<MarkerEventV1>>
MarkerEventV1::decodeBinary(const std::vector<uint8_t> &message) {
  size_t cursor = 0;
  std::string sessionId{};
  std::string markerType{};
  std::string markerId{};
  std::string event{};
  std::string label{};
  uint64_t emittedAtUs = 0;
  std::string attributesJson{};

  if (!parseBinaryString(message, cursor, sessionId) ||
      !parseBinaryString(message, cursor, markerType) ||
      !parseBinaryString(message, cursor, markerId) ||
      !parseBinaryString(message, cursor, event) ||
      !parseBinaryString(message, cursor, label) ||
      !parseBinaryUint64(message, cursor, emittedAtUs) ||
      !parseBinaryString(message, cursor, attributesJson)) {
    return {};
  }

  return nat::core::make_unique<MarkerEventV1>(
      sessionId, markerType, markerId, event, label, emittedAtUs,
      attributesJson);
}

Optional<std::unique_ptr<MarkerEventV1>>
MarkerEventV1::decodeAll(const std::vector<uint8_t> &message,
                         const SerializationType &type) {
  switch (type) {
  case SerializationType::Json:
    return decodeJson(message);
  case SerializationType::Binary:
    return decodeBinary(message);
  default:
    return {};
  }
}

Optional<std::shared_ptr<Schema>>
MarkerEventV1::tryDecode(const std::vector<uint8_t> &message,
                         const SerializationType &type) const {
  return sharedDecodeAll(message, type);
}

void MarkerEventV1::registerWithRegistry(Registry &registry) {
  registry.registerDecoder(name, SerializationType::Json, uniqueDecodeAll);
  registry.registerDecoder(name, SerializationType::Binary, uniqueDecodeAll);
}

Optional<std::shared_ptr<Schema>>
MarkerEventV1::sharedDecodeAll(const std::vector<uint8_t> &message,
                               const SerializationType &type) {
  auto decodedMaybe = decodeAll(message, type);
  if (!decodedMaybe.has_value()) {
    return {};
  }

  std::shared_ptr<Schema> shared(std::move(decodedMaybe.value()));
  return shared;
}

Optional<std::unique_ptr<Schema>>
MarkerEventV1::uniqueDecodeAll(const std::vector<uint8_t> &message,
                               const SerializationType &type) {
  auto decodedMaybe = decodeAll(message, type);
  if (!decodedMaybe.has_value()) {
    return {};
  }

  std::unique_ptr<Schema> schema(std::move(decodedMaybe.value()));
  return Optional<std::unique_ptr<Schema>>{std::move(schema)};
}

} // namespace core
} // namespace nat

namespace {

// Two-call byte-buffer output helper (see libnatkit-core-abi.h). Writes the
// exact required size through inout_size and, when out is non-NULL and large
// enough, copies the bytes. No NUL terminator -- this is a byte buffer.
int writeMarkerBytesOutput(const std::vector<uint8_t> &value, uint8_t *out,
                           size_t *inout_size) {
  if (inout_size == nullptr) {
    return NAT_ERR_NULL_ARGUMENT;
  }
  const size_t required = value.size();
  const size_t provided = *inout_size;
  *inout_size = required;
  if (out == nullptr) {
    return NAT_OK;
  }
  if (provided < required) {
    return NAT_ERR_BUFFER_TOO_SMALL;
  }
  if (!value.empty()) {
    std::memcpy(out, value.data(), value.size());
  }
  return NAT_OK;
}

// Shared body: decode the input JSON into a MarkerEventV1 and re-emit the
// canonical JSON wire form. encode and decode coincide for the JSON-wire marker
// (see header) but are exposed as distinct symbols for API symmetry.
int markerEventCanonicalizeJson(const uint8_t *input, size_t input_size,
                                uint8_t *out, size_t *inout_size) {
  if (input == nullptr || inout_size == nullptr) {
    return NAT_ERR_NULL_ARGUMENT;
  }
  try {
    const std::vector<uint8_t> message(input, input + input_size);
    auto recordMaybe = nat::core::MarkerEventV1::decodeJson(message);
    if (!recordMaybe.has_value()) {
      return NAT_ERR_DECODE_FAILED;
    }
    auto encoded =
        recordMaybe.value()->encodeToBytes(nat::core::SerializationType::Json);
    if (!encoded) {
      return NAT_ERR_ENCODE_FAILED;
    }
    return writeMarkerBytesOutput(*encoded, out, inout_size);
  } catch (...) {
    return NAT_ERR_INTERNAL;
  }
}

} // namespace

extern "C" int nat_core_v1_marker_event_encode_json(
    const uint8_t *payload_json, size_t payload_json_size, uint8_t *out_message,
    size_t *inout_message_size) {
  return markerEventCanonicalizeJson(payload_json, payload_json_size,
                                     out_message, inout_message_size);
}

extern "C" int nat_core_v1_marker_event_decode_json(
    const uint8_t *message, size_t message_size, uint8_t *out_payload_json,
    size_t *inout_payload_json_size) {
  return markerEventCanonicalizeJson(message, message_size, out_payload_json,
                                     inout_payload_json_size);
}
