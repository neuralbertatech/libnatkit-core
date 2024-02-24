#include <libnatkit-core.hpp>

#ifdef SERVER
#include <nlohmann/json.hpp>
#else
#include <cJSON.h>
#endif

namespace nat::core {

std::unique_ptr<std::vector<uint8_t>>
BasicMetaInfoSchema::encodeToBytes(const SerializationType &type) const {
    switch (type) {
    case SerializationType::Json:
#ifdef SERVER
      nlohmann::json j;
      j["name"] = streamName;
      const auto jsonStr = j.dump();
      return std::make_unique<std::vector<uint8_t>>(std::begin(jsonStr), std::end(jsonStr));
#else
      cJSON *jsonObject = cJSON_CreateObject();
      cJSON_AddStringToObject(jsonObject, "name", streamName.c_str());
      const auto jsonStr = std::string(cJSON_Print(jsonObject));
      return std::make_unique<std::vector<uint8_t>>(std::begin(jsonStr), std::end(jsonStr));
#endif
    }
      assert(0);
  }

  bool BasicMetaInfoSchema::isSerializationTypeSupported(const SerializationType type) const {
    switch (type) {
    case SerializationType::Json:
      return true;
    default:
      assert(0);
    }
  }

  std::string BasicMetaInfoSchema::toString() const {
    return getName() + ": {\"name\": " + streamName + "}";
  }

  std::optional<std::unique_ptr<BasicMetaInfoSchema>> BasicMetaInfoSchema::decodeJson(const std::vector<uint8_t> &message) {
        std::string jsonStr(std::begin(message), std::end(message));
#ifdef SERVER
        const auto json = nlohmann::json::parse(jsonStr);
        return std::make_unique<BasicMetaInfoSchema>(json["name"]);
#else
        cJSON *json = cJSON_Parse(jsonStr.c_str());
        cJSON *name = cJSON_GetObjectItemCaseSensitive(json, "name");
        return std::make_unique<BasicMetaInfoSchema>(std::string(name->valuestring));
#endif
      }

  std::optional<std::unique_ptr<BasicMetaInfoSchema>> BasicMetaInfoSchema::decodeAll(const std::vector<uint8_t> &message,
                                    const SerializationType &type) {
    switch (type) {
    case SerializationType::Json:
      return decodeJson(message);
    default:
      assert(0);
    }
  }

  void
  BasicMetaInfoSchema::decodeAndDispatch(const std::vector<uint8_t> &message,
                    const SerializationType &type,
                    const std::function<void(const std::shared_ptr<Schema> &)>
                        &dispatchMethod) {
    const std::optional<std::shared_ptr<Schema>> decodedMessageMaybe = decodeAll(message, type);
    if (decodedMessageMaybe.has_value()) {
      dispatchMethod(decodedMessageMaybe.value());
    }
  }

  std::optional<std::shared_ptr<Schema>> BasicMetaInfoSchema::tryDecode(const std::vector<uint8_t> &message,
                                    const SerializationType &type) const {
    return decodeAll(message, type);
  }

  void BasicMetaInfoSchema::registerWithRegistry(Registry &registry) {
    registry.registerDecoder(name, SerializationType::Json, decodeAll);
  }

  std::string BasicMetaInfoSchema::getName() const { return name; }

  std::string BasicMetaInfoSchema::getStreamName() const { return streamName; }

} // namespace nat::core
