#include <libnatkit-core.hpp>

#include <cJSON.h>

namespace nat {
namespace core {

const std::string NatImuDataSchema::name = "NatImuDataSchema";

NatImuDataSchema::NatImuDataSchema(uint64_t time, float* data, int size) : time(time) {
  assert(size <= 9);
  for (int i = 0; i < 9; ++i)
    if (i < size)
      this->data[i] = data[i];
    else
      this->data[i] = 0;
}

std::unique_ptr<std::vector<uint8_t>>
NatImuDataSchema::encodeToBytes(const SerializationType &type) const {
  switch (type) {
  case SerializationType::Json:
    cJSON *jsonObject = cJSON_CreateObject();
    cJSON_AddNumberToObject(jsonObject, "time", time);
    const auto jsonStr = std::string(cJSON_Print(jsonObject));
    return nat::core::make_unique<std::vector<uint8_t>>(std::begin(jsonStr), std::end(jsonStr));
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

Optional<std::unique_ptr<NatImuDataSchema>> NatImuDataSchema::decodeJson(const std::vector<uint8_t> &message) {
      std::string jsonStr(std::begin(message), std::end(message));
      cJSON *json = cJSON_Parse(jsonStr.c_str());
      cJSON *name = cJSON_GetObjectItemCaseSensitive(json, "time");
      float tmpData[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
      return nat::core::make_unique<NatImuDataSchema>(name->valuedouble, tmpData, 9);
    }

Optional<std::unique_ptr<NatImuDataSchema>> NatImuDataSchema::decodeAll(const std::vector<uint8_t> &message,
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
  const Optional<std::unique_ptr<NatImuDataSchema>> decodedMessageMaybe = decodeAll(message, type);
  if (decodedMessageMaybe.has_value()) {
    std::shared_ptr<NatImuDataSchema> sharedDecodedMessage = std::move(decodedMessageMaybe.value());
    dispatchMethod(sharedDecodedMessage);
  }
}

Optional<std::shared_ptr<Schema>> NatImuDataSchema::tryDecode(const std::vector<uint8_t> &message,
                                  const SerializationType &type) const {
  auto decoded = decodeAll(message, type);
  if (decoded.has_value()) {
    std::shared_ptr<Schema> sharedDecoded = std::move(decoded.value());
    return sharedDecoded;
  } else {
    return {};
  }
}

void NatImuDataSchema::registerWithRegistry(Registry &registry) {
  registry.registerDecoder(name, SerializationType::Json, [](const std::vector<uint8_t> &message, const SerializationType &type) {
        Optional<std::unique_ptr<NatImuDataSchema>> decodedMaybe = decodeAll(message, type);
        if (decodedMaybe.has_value()) {
          std::unique_ptr<Schema> convertedDecoded = std::move(decodedMaybe.value());
          return Optional<std::unique_ptr<Schema>>{std::move(convertedDecoded)};
        } else {
          return Optional<std::unique_ptr<Schema>>{};
        }
      });
}

std::string NatImuDataSchema::getName() const { return name; }

double NatImuDataSchema::getTime() const { return time; }

} // namespace core
} // namespace nat
