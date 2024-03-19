#include <libnatkit-core.hpp>

#include <cJSON.h>

namespace nat::core {

std::unique_ptr<std::vector<uint8_t>>
NatImuDataSchema::encodeToBytes(const SerializationType &type) const {
  switch (type) {
  case SerializationType::Json:
    cJSON *jsonObject = cJSON_CreateObject();
    cJSON_AddNumberToObject(jsonObject, "time", time);
    const auto jsonStr = std::string(cJSON_Print(jsonObject));
    return std::make_unique<std::vector<uint8_t>>(std::begin(jsonStr), std::end(jsonStr));
  }
    assert(0);
}

bool NatImuDataSchema::isSerializationTypeSupported(const SerializationType type) const {
  switch (type) {
  case SerializationType::Json:
    return true;
  default:
    assert(0);
  }
}

std::string NatImuDataSchema::toString() const {
  return getName() + ": {\"time\": " + std::to_string(time) + "}";
}

std::optional<std::unique_ptr<NatImuDataSchema>> NatImuDataSchema::decodeJson(const std::vector<uint8_t> &message) {
      std::string jsonStr(std::begin(message), std::end(message));
      cJSON *json = cJSON_Parse(jsonStr.c_str());
      cJSON *name = cJSON_GetObjectItemCaseSensitive(json, "time");
      float tmpData[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
      return std::make_unique<NatImuDataSchema>(name->valuedouble, tmpData, 9);
    }

std::optional<std::unique_ptr<NatImuDataSchema>> NatImuDataSchema::decodeAll(const std::vector<uint8_t> &message,
                                  const SerializationType &type) {
  switch (type) {
  case SerializationType::Json:
    return decodeJson(message);
  default:
    assert(0);
  }
}

void
NatImuDataSchema::decodeAndDispatch(const std::vector<uint8_t> &message,
                  const SerializationType &type,
                  const std::function<void(const std::shared_ptr<Schema> &)>
                      &dispatchMethod) {
  const std::optional<std::shared_ptr<Schema>> decodedMessageMaybe = decodeAll(message, type);
  if (decodedMessageMaybe.has_value()) {
    dispatchMethod(decodedMessageMaybe.value());
  }
}

std::optional<std::shared_ptr<Schema>> NatImuDataSchema::tryDecode(const std::vector<uint8_t> &message,
                                  const SerializationType &type) const {
  return decodeAll(message, type);
}

void NatImuDataSchema::registerWithRegistry(Registry &registry) {
  registry.registerDecoder(name, SerializationType::Json, decodeAll);
}

std::string NatImuDataSchema::getName() const { return name; }

double NatImuDataSchema::getTime() const { return time; }

} // namespace nat::core
