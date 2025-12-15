#include <libnatkit-core.hpp>
#include <iostream>

#ifdef SERVER
#include <nlohmann/json.hpp>
#else
#include <cJSON.h>
#endif
#include <string.h>

namespace nat {
namespace core {

const std::string NatImuDataSchema::name = "NatImuDataSchema";
const uint32_t NatImuDataSchema::NatImuDataSchemaDataArraySize = 10;

NatImuDataSchema::NatImuDataSchema()
    : time(0), accuracies(0) {
    for (int i = 0; i < NatImuDataSchemaDataArraySize; ++i)
        this->data[i] = 0;
}

NatImuDataSchema::NatImuDataSchema(const NatImuDataSchema &other)
    : time(other.time), accuracies(other.accuracies) {
    for (int i = 0; i < NatImuDataSchemaDataArraySize; ++i)
        this->data[i] = other.data[i];
}

NatImuDataSchema::NatImuDataSchema(uint64_t time, NatImuDataSchema::SensorAccuracy acceleration_accuracy, NatImuDataSchema::SensorAccuracy gyroscope_accuracy, NatImuDataSchema::SensorAccuracy rotation_accuracy, bool acceleration_has_data, bool gryoscope_has_data, bool rotation_has_data, const float* data, int size) 
    : time(time), accuracies(((static_cast<int>(acceleration_accuracy) & 3) << 4) | ((static_cast<int>(gyroscope_accuracy) & 3) << 2) | (static_cast<int>(rotation_accuracy) & 3)), has_data((static_cast<int>(acceleration_has_data) << 2) | (static_cast<int>(gryoscope_has_data) << 1) | static_cast<int>(rotation_has_data)) {
  assert(size <= NatImuDataSchemaDataArraySize);
  for (int i = 0; i < NatImuDataSchemaDataArraySize; ++i)
      if (i < size)
          this->data[i] = data[i];
      else
        this->data[i] = 0;
}

NatImuDataSchema::NatImuDataSchema(uint64_t time, uint8_t accuracies, uint8_t has_data, const float* data, int size) 
    : time(time), accuracies(accuracies), has_data(has_data) {
  assert(size <= NatImuDataSchemaDataArraySize);
  for (int i = 0; i < NatImuDataSchemaDataArraySize; ++i)
      if (i < size)
          this->data[i] = data[i];
      else
        this->data[i] = 0;
}

#ifdef SERVER

Optional<std::shared_ptr<NatImuDataSchema>> NatImuDataSchema::tryCreateFromSchema(const Optional<const std::shared_ptr<Schema>>& messageMaybe) {
    if (!messageMaybe.has_value() || messageMaybe.value() == nullptr) {
        return {};
    }
    else {
        if (messageMaybe.value()->getName() == NatImuDataSchema::name) {
            return std::dynamic_pointer_cast<NatImuDataSchema>(messageMaybe.value());
        }
        else {
            return {};
        }
    }
}

#endif

NatImuDataSchema::SensorAccuracy NatImuDataSchema::convertIntToSensorAccuracy(int val) {
    switch (val) {
    case 0:
    case 1:
    case 2:
    case 3:
        return static_cast<NatImuDataSchema::SensorAccuracy>(val);
    default:
        return NatImuDataSchema::SensorAccuracy::Unreliable;
    }
}

int NatImuDataSchema::convertSensorAccuracyToInt(NatImuDataSchema::SensorAccuracy accuracy) {
    return static_cast<int>(accuracy);
}

std::string NatImuDataSchema::toString(NatImuDataSchema::SensorAccuracy accuracy) {
    switch (accuracy) {
    case NatImuDataSchema::SensorAccuracy::Unreliable:
        return "Unreliable";

    case NatImuDataSchema::SensorAccuracy::LowAccuracy:
        return "Low Accuracy";

    case NatImuDataSchema::SensorAccuracy::MediumAccuracy:
        return "Medium Accuracy";

    case NatImuDataSchema::SensorAccuracy::HighAccuracy:
        return "High Accuracy";

    default:
        return "Error: Not a valid value!";
    }
}

#ifndef SERVER
cJSON* CreateJsonDataArray(const float *data) {
  cJSON *jsonDataArray = cJSON_CreateArray();
  if (jsonDataArray == NULL)
    return NULL;
  for (int i = 0; i < NatImuDataSchema::NatImuDataSchemaDataArraySize; ++i) {
    cJSON *dataNumber = cJSON_CreateNumber(data[i]);
    if (dataNumber == NULL) {
      cJSON_Delete(jsonDataArray);
      return NULL;
    }
    cJSON_AddItemToArray(jsonDataArray, dataNumber);
  }
  return jsonDataArray;
}

std::vector<uint8_t>* stringToBytes(const char *string) {
  std::vector<uint8_t> *bytes(new std::vector<uint8_t>());
  int length = strlen(string);
  bytes->reserve(length);
  for (int i = 0; i < length; ++i)
    bytes->emplace_back(string[i]);
  return bytes;
}
#endif

std::unique_ptr<std::vector<uint8_t>> CreateJsonDataObject(uint64_t time, uint8_t accuracies, uint8_t has_data, const float* data) {
#ifdef SERVER
    nlohmann::json j;
    j["time"] = time;
    nlohmann::json jsonDataArray = nlohmann::json::array();
    for (int i = 0; i < NatImuDataSchema::NatImuDataSchemaDataArraySize; ++i)
        jsonDataArray.push_back(data[i]);
    j["data"] = jsonDataArray;
    j["accuracies"] = accuracies;
    j["has_data"] = has_data;
    const auto jsonStr = j.dump();
    return nat::core::make_unique<std::vector<uint8_t>>(std::begin(jsonStr), std::end(jsonStr));
#else
    char *json = NULL;
  cJSON *jsonObject = cJSON_CreateObject();
  cJSON *jsonDataArray = NULL;
  cJSON *jsonTime = NULL;
    if (jsonObject == NULL)
      return nullptr;
    
    jsonTime = cJSON_CreateNumber(time);
    if (jsonTime == NULL) {
      cJSON_Delete(jsonObject);
      return nullptr;
    }
    cJSON_AddNumberToObject(jsonObject, "time", time);

    jsonDataArray = CreateJsonDataArray(data);
    if (jsonDataArray == NULL) {
      cJSON_Delete(jsonTime);
      cJSON_Delete(jsonObject);
      return nullptr;
    }

    cJSON_AddNumberToObject(jsonObject, "accuracies", accuracies);
    cJSON_AddNumberToObject(jsonObject, "has_data", has_data);
    cJSON_AddItemToObject(jsonObject, "data", jsonDataArray);

    json = cJSON_Print(jsonObject);
    cJSON_Delete(jsonDataArray);
    cJSON_Delete(jsonTime);
    cJSON_Delete(jsonObject);
    if (json == NULL) {
      free(json);
      return nullptr;
    }

    std::unique_ptr<std::vector<uint8_t>> bytes(stringToBytes(json));
    free(json);
    return bytes;
#endif
}


std::unique_ptr<std::vector<uint8_t>>
NatImuDataSchema::encodeToBytes(const SerializationType &type) const {
  switch (type) {
  case SerializationType::Json:
    return CreateJsonDataObject(this->time, this->accuracies, this->has_data, this->data);

  case SerializationType::Csv:
      std::string csvString = std::to_string(time) + ","
          + std::to_string(accuracies) + ","
          + std::to_string(has_data) + ","
          + std::to_string(data[0]) + ","
          + std::to_string(data[1]) + ","
          + std::to_string(data[2]) + ","
          + std::to_string(data[3]) + ","
          + std::to_string(data[4]) + ","
          + std::to_string(data[5]) + ","
          + std::to_string(data[6]) + ","
          + std::to_string(data[7]) + ","
          + std::to_string(data[7]) + ","
          + std::to_string(data[8]) + ","
          + std::to_string(data[9]);
      return nat::core::make_unique<std::vector<uint8_t>>(std::begin(csvString), std::end(csvString));
  }
    assert(0);
}

bool NatImuDataSchema::isSerializationTypeSupported(const SerializationType type) const {
  switch (type) {
  case SerializationType::Json:
    return true;
  case SerializationType::Csv:
    return true;
  default:
    assert(0);
  }
}

std::string NatImuDataSchema::toString() const {
    std::string result = getName() + ": {\"time\": " + std::to_string(time) + ", \"data\": [";
    for (int i = 0; i < NatImuDataSchemaDataArraySize; ++i) {
        result += std::to_string(this->data[i]);
        if (i < NatImuDataSchemaDataArraySize - 1)
            result += ", ";
    }
  return result + "]}";
}

Optional<std::unique_ptr<NatImuDataSchema>> NatImuDataSchema::decodeJson(const std::vector<uint8_t> &message) {
      std::string jsonStr(std::begin(message), std::end(message));
#ifdef SERVER
      const auto json = nlohmann::json::parse(jsonStr);
      const auto time = json.at("time").get<double>();
      const auto accuracies = json.at("accuracies").get<int>();
      const auto has_data = json.at("has_data").get<int>();
      const auto data = json.at("data").get<std::vector<float>>();
      auto decodedSchema = nat::core::make_unique<NatImuDataSchema>(time, static_cast<uint8_t>(accuracies), static_cast<uint8_t>(has_data), data.data(), NatImuDataSchema::NatImuDataSchemaDataArraySize);
      return std::move(decodedSchema);
#else
      cJSON *json = cJSON_Parse(jsonStr.c_str());
      cJSON *name = cJSON_GetObjectItemCaseSensitive(json, "time");
      cJSON *accuracies = cJSON_GetObjectItemCaseSensitive(json, "accuracies");
      cJSON *has_data = cJSON_GetObjectItemCaseSensitive(json, "has_data");
      float tmpData[NatImuDataSchemaDataArraySize] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
      return nat::core::make_unique<NatImuDataSchema>(name->valuedouble, static_cast<uint8_t>(accuracies->valueint), static_cast<uint8_t>(has_data->valueint), tmpData, NatImuDataSchema::NatImuDataSchemaDataArraySize);
#endif
    }

Optional<std::unique_ptr<NatImuDataSchema>> NatImuDataSchema::decodeCsv(const std::vector<uint8_t>& message) {
    std::string messageStr(std::begin(message), std::end(message));
    std::string currentValue = "";
    size_t characterIndex = 0;
    uint64_t time = 0;
    float data[NatImuDataSchemaDataArraySize];
    uint8_t accuracies = 0;
    uint8_t has_data = 0;

    for (int i = 0; i < NatImuDataSchemaDataArraySize + 2; ++i) {
        while (characterIndex < messageStr.size() && messageStr[characterIndex] != ',') {
            currentValue += messageStr[characterIndex];
            ++characterIndex;
        }
        switch (i) {
        case 0:
            time = std::stoul(currentValue);
            break;

        case 1:
            accuracies = std::stoi(currentValue);
            break;

        case 2:
            has_data = std::stoi(currentValue);
            break;

        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
            data[i - 3] = std::stof(currentValue);
            break;

        default:
            assert(0);
        }
        currentValue = "";
    }
    return nat::core::make_unique<NatImuDataSchema>(time, accuracies, has_data, data, NatImuDataSchemaDataArraySize);
}

Optional<std::unique_ptr<NatImuDataSchema>> NatImuDataSchema::decodeAll(const std::vector<uint8_t> &message,
                                  const SerializationType &type) {
  switch (type) {
  case SerializationType::Json:
    return decodeJson(message);
  case SerializationType::Csv:
   return decodeCsv(message);
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
    const decoder_t decoder = [](const message_t& message, const SerializationType& type) {
        Optional<std::unique_ptr<NatImuDataSchema>> decodedMaybe = decodeAll(message, type);
        if (decodedMaybe.has_value()) {
            std::unique_ptr<Schema> convertedDecoded = std::move(decodedMaybe.value());
            return Optional<std::unique_ptr<Schema>>{std::move(convertedDecoded)};
        }
        else {
            return Optional<std::unique_ptr<Schema>>{};
        }
        };
  registry.registerDecoder(name, SerializationType::Json, decoder);
  registry.registerDecoder(name, SerializationType::Csv, decoder);
}

std::string NatImuDataSchema::getName() const { return name; }

double NatImuDataSchema::getTime() const { return time; }

NatImuDataSchema::SensorAccuracy NatImuDataSchema::getAccelerationAccuracy() const {
  return convertIntToSensorAccuracy((this->accuracies >> 4) & 3);
}

NatImuDataSchema::SensorAccuracy NatImuDataSchema::getGyroscopeAccuracy() const{
  return convertIntToSensorAccuracy((this->accuracies >> 2) & 3);
}

NatImuDataSchema::SensorAccuracy NatImuDataSchema::getRotationAccuracy() const{
  return convertIntToSensorAccuracy(this->accuracies & 3);
}

bool NatImuDataSchema::wasDataSetForAcceleration() const {
  return static_cast<bool>((this->has_data >> 2) & 1);
}

bool NatImuDataSchema::wasDataSetForGryoscope() const {
  return static_cast<bool>((this->has_data >> 1) & 1);
}

bool NatImuDataSchema::wasDataSetForRotation() const {
  return static_cast<bool>(this->has_data & 1);
}

const float* NatImuDataSchema::getData() const {
  return this->data;
}

} // namespace core
} // namespace nat
