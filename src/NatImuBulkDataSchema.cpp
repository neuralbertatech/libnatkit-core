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

        const std::string NatImuBulkDataSchema::name = "NatImuBulkDataSchema";
        static const int NatImuBulkDataSchemaDataArraySize = 100;

        NatImuBulkDataSchema::NatImuBulkDataSchema()
            : size(0) {
            for (size_t i = 0; i < NatImuBulkDataSchemaDataArraySize; ++i)
                data[i] = NatImuDataSchema{};
        }

        NatImuBulkDataSchema::NatImuBulkDataSchema(const NatImuDataSchema* data, uint8_t size)
            : size(size) {
            assert(size <= NatImuBulkDataSchemaDataArraySize);
            for (int i = 0; i < size; ++i)
                this->data[i] = data[i];
        }

        bool NatImuBulkDataSchema::isFull() const {
            return size == NatImuBulkDataSchemaDataArraySize;
        }

        void NatImuBulkDataSchema::add(const NatImuDataSchema& datum) {
            assert(size < NatImuBulkDataSchemaDataArraySize);
            data[size++] = datum;
        }

        void NatImuBulkDataSchema::setData(const NatImuDataSchema* data, int32_t size) {
            assert(size <= NatImuBulkDataSchemaDataArraySize);
            for (int i = 0; i < size; ++i)
                this->data[i] = data[i];
            this->size = size;
        }

#ifdef SERVER

        Optional<std::shared_ptr<NatImuBulkDataSchema>> NatImuBulkDataSchema::tryCreateFromSchema(const Optional<const std::shared_ptr<Schema>>& messageMaybe) {
            if (!messageMaybe.has_value() || messageMaybe.value() == nullptr) {
                return {};
            }
            else {
                if (messageMaybe.value()->getName() == NatImuBulkDataSchema::name) {
                    return std::dynamic_pointer_cast<NatImuBulkDataSchema>(messageMaybe.value());
                }
                else {
                    return {};
                }
            }
        }

#endif

//#ifndef SERVER
//        cJSON* CreateJsonDataArray(const float* data) {
//            cJSON* jsonDataArray = cJSON_CreateArray();
//            if (jsonDataArray == NULL)
//                return NULL;
//            for (int i = 0; i < NatImuBulkDataSchemaDataArraySize; ++i) {
//                cJSON* dataNumber = cJSON_CreateNumber(data[i]);
//                if (dataNumber == NULL) {
//                    cJSON_Delete(jsonDataArray);
//                    return NULL;
//                }
//                cJSON_AddItemToArray(jsonDataArray, dataNumber);
//            }
//            return jsonDataArray;
//        }
//
//        std::vector<uint8_t>* stringToBytes(const char* string) {
//            std::vector<uint8_t>* bytes(new std::vector<uint8_t>());
//            int length = strlen(string);
//            bytes->reserve(length);
//            for (int i = 0; i < length; ++i)
//                bytes->emplace_back(string[i]);
//            return bytes;
//        }
//#endif

//        std::unique_ptr<std::vector<uint8_t>> CreateJsonDataObject(uint64_t time, int accuracy, const float* data) {
//#ifdef SERVER
//            nlohmann::json j;
//            j["time"] = time;
//            nlohmann::json jsonDataArray = nlohmann::json::array();
//            for (int i = 0; i < NatImuBulkDataSchemaDataArraySize; ++i)
//                jsonDataArray.push_back(data[i]);
//            j["data"] = jsonDataArray;
//            j["accuracy"] = accuracy;
//            const auto jsonStr = j.dump();
//            return nat::core::make_unique<std::vector<uint8_t>>(std::begin(jsonStr), std::end(jsonStr));
//#else
//            char* json = NULL;
//            cJSON* jsonObject = cJSON_CreateObject();
//            cJSON* jsonDataArray = NULL;
//            cJSON* jsonTime = NULL;
//            cJSON* jsonAccuracy = NULL;
//            if (jsonObject == NULL)
//                return nullptr;
//
//            jsonTime = cJSON_CreateNumber(time);
//            if (jsonTime == NULL) {
//                cJSON_Delete(jsonObject);
//                return nullptr;
//            }
//            cJSON_AddNumberToObject(jsonObject, "time", time);
//
//            jsonDataArray = CreateJsonDataArray(data);
//            if (jsonDataArray == NULL) {
//                cJSON_Delete(jsonTime);
//                cJSON_Delete(jsonObject);
//                return nullptr;
//            }
//
//            cJSON_AddItemToObject(jsonObject, "data", jsonDataArray);
//
//            jsonAccuracy = cJSON_CreateNumber(accuracy);
//            if (jsonTime == NULL) {
//                cJSON_Delete(jsonTime);
//                cJSON_Delete(jsonObject);
//                return nullptr;
//            }
//
//            cJSON_AddNumberToObject(jsonObject, "accuracy", accuracy);
//
//            json = cJSON_Print(jsonObject);
//            cJSON_Delete(jsonTime);
//            cJSON_Delete(jsonAccuracy);
//            cJSON_Delete(jsonObject);
//            if (json == NULL) {
//                free(json);
//                return nullptr;
//            }
//
//            cJSON_AddNumberToObject(jsonObject, "time", time);
//
//            std::unique_ptr<std::vector<uint8_t>> bytes(stringToBytes(json));
//            free(json);
//            return bytes;
//#endif
//        }


        std::unique_ptr<std::vector<uint8_t>>
            NatImuBulkDataSchema::encodeToBytes(const SerializationType& type) const {
            assert(isFull());
            switch (type) {
            //case SerializationType::Json:
            //    return CreateJsonDataObject(this->time, convertSensorAccuracyToInt(this->accuracy), this->data);

            case SerializationType::Csv: {
                auto bytes = nat::core::make_unique<std::vector<uint8_t>>();
                bytes->reserve(16384);
                for (size_t i = 0; i < NatImuBulkDataSchemaDataArraySize - 1; ++i) {
                    const auto singleImuBytes = data[i].encodeToBytes(type);
                    bytes->insert(bytes->end(), singleImuBytes->begin(), singleImuBytes->end());
                    bytes->push_back(static_cast<uint8_t>('\n'));
                }
                const auto singleImuBytes = data[NatImuBulkDataSchemaDataArraySize-1].encodeToBytes(type);
                bytes->insert(bytes->end(), singleImuBytes->begin(), singleImuBytes->end());
                return bytes;
            }

            case SerializationType::Binary: {
                const int singleReadingSizeInBytes = 50;
                auto bytes = nat::core::make_unique<std::vector<uint8_t>>(singleReadingSizeInBytes * NatImuBulkDataSchemaDataArraySize, 0);
                char* dataPointer = (char*)bytes->data();
                for (size_t i = 0; i < NatImuBulkDataSchemaDataArraySize; ++i) {
                    dataPointer += Binary::unsafeWriteAsBinaryToArray<uint64_t>(dataPointer, data[i].time);
                    for (size_t j = 0; j < NatImuDataSchema::NatImuDataSchemaDataArraySize; ++j) {
                        dataPointer += Binary::unsafeWriteAsBinaryToArray<uint32_t>(dataPointer, (uint32_t)data[i].data[j]);
                    }
                    dataPointer += Binary::unsafeWriteAsBinaryToArray<uint8_t>(dataPointer, static_cast<uint8_t>(data[i].accuracies));
                    dataPointer += Binary::unsafeWriteAsBinaryToArray<uint8_t>(dataPointer, static_cast<uint8_t>(data[i].has_data));
                }
                size_t pointerDiff = (size_t)dataPointer - (size_t)bytes->data();
                size_t bytesSize = bytes->size();
                assert(pointerDiff == (singleReadingSizeInBytes * NatImuBulkDataSchemaDataArraySize));
                assert(bytesSize == (singleReadingSizeInBytes * NatImuBulkDataSchemaDataArraySize));
                return bytes;
            }
            }

            assert(0);
        }

        bool NatImuBulkDataSchema::isSerializationTypeSupported(const SerializationType type) const {
            switch (type) {
            case SerializationType::Json:
                return false;
            case SerializationType::Csv:
                return true;
            case SerializationType::Binary:
                return true;
            default:
                assert(0);
            }
        }

        std::string NatImuBulkDataSchema::toString() const {
            std::string result = getName() + ": [";
            for (int i = 0; i < NatImuBulkDataSchemaDataArraySize; ++i) {
                result += this->data[i].toString();
                if (i < NatImuBulkDataSchemaDataArraySize - 1)
                    result.append(",");
                result.append("\n");
            }
            result.append("]");
            return result;
        }

//        Optional<std::unique_ptr<NatImuBulkDataSchema>> NatImuBulkDataSchema::decodeJson(const std::vector<uint8_t>& message) {
//            std::string jsonStr(std::begin(message), std::end(message));
//#ifdef SERVER
//            const auto json = nlohmann::json::parse(jsonStr);
//            const auto time = json.at("time").get<double>();
//            const auto accuracy = convertIntToSensorAccuracy(json.at("accuracy").get<int>());
//            const auto data = json.at("data").get<std::vector<float>>();
//            auto decodedSchema = nat::core::make_unique<NatImuBulkDataSchema>(time, accuracy, data.data(), NatImuBulkDataSchemaDataArraySize);
//            return std::move(decodedSchema);
//#else
//            cJSON* json = cJSON_Parse(jsonStr.c_str());
//            cJSON* name = cJSON_GetObjectItemCaseSensitive(json, "time");
//            cJSON* accuracy = cJSON_GetObjectItemCaseSensitive(json, "accuracy");
//            float tmpData[NatImuBulkDataSchemaDataArraySize] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
//            return nat::core::make_unique<NatImuBulkDataSchema>(name->valuedouble, convertIntToSensorAccuracy(accuracy->valueint), tmpData, NatImuBulkDataSchemaDataArraySize);
//#endif
//        }

        Optional<std::unique_ptr<NatImuBulkDataSchema>> NatImuBulkDataSchema::decodeBinary(const std::vector<uint8_t>& message) {
            const int singleReadingSizeInBytes = 50;
            const size_t expectedSize = singleReadingSizeInBytes * NatImuBulkDataSchemaDataArraySize;
            if (message.size() != expectedSize) {
                std::cerr << "NatImuBulkDataSchema::decodeBinary: message size mismatch. Expected " 
                          << expectedSize << " bytes, got " << message.size() << " bytes.\n";
                return {};
            }
            char* bytes = (char*)message.data();
            auto schema = nat::core::make_unique<NatImuBulkDataSchema>();

            for (int i = 0; i < NatImuBulkDataSchemaDataArraySize; ++i) {
                NatImuDataSchema sensorReading{};
                bytes += Binary::unsafeParseFromBinary(bytes, sensorReading.time);
                for (int j = 0; j < 13; ++j) {
                    bytes += Binary::unsafeParseFromBinary<uint32_t>(bytes, *(uint32_t*)&sensorReading.data[j]);
                }
                bytes += Binary::unsafeParseFromBinary<uint8_t>(bytes, *(uint8_t*)&sensorReading.accuracies);
                bytes += Binary::unsafeParseFromBinary<uint8_t>(bytes, *(uint8_t*)&sensorReading.has_data);
                schema->add(sensorReading);
            }
            return std::move(schema);
        }

        Optional<std::unique_ptr<NatImuBulkDataSchema>> NatImuBulkDataSchema::decodeCsv(const std::vector<uint8_t>& message) {
            size_t byteIndex = 0;
            auto schema = nat::core::make_unique<NatImuBulkDataSchema>();

            for (int i = 0; i < NatImuBulkDataSchemaDataArraySize; ++i) {
                std::vector<uint8_t> imuRecordBytes{};
                while (byteIndex < message.size() && message[byteIndex] != '\n') {
                    imuRecordBytes.push_back(message[byteIndex]);
                    ++byteIndex;
                }
                auto decodedImuRecord = NatImuDataSchema::decodeCsv(imuRecordBytes);
                assert(decodedImuRecord.has_value());
                schema->add(*decodedImuRecord.value());
            }
            return std::move(schema);
        }

        Optional<std::unique_ptr<NatImuBulkDataSchema>> NatImuBulkDataSchema::decodeAll(const std::vector<uint8_t>& message,
            const SerializationType& type) {
            switch (type) {
            //case SerializationType::Json:
            //    return decodeJson(message);
            case SerializationType::Csv:
                return decodeCsv(message);
            case SerializationType::Binary:
                return decodeBinary(message);
            default:
                assert(0);
            }
        }

        void
            NatImuBulkDataSchema::decodeAndDispatch(const std::vector<uint8_t>& message,
                const SerializationType& type,
                const std::function<void(const std::shared_ptr<Schema>&)>
                & dispatchMethod) {
            const Optional<std::unique_ptr<NatImuBulkDataSchema>> decodedMessageMaybe = decodeAll(message, type);
            if (decodedMessageMaybe.has_value()) {
                std::shared_ptr<NatImuBulkDataSchema> sharedDecodedMessage = std::move(decodedMessageMaybe.value());
                dispatchMethod(sharedDecodedMessage);
            }
        }

        Optional<std::shared_ptr<Schema>> NatImuBulkDataSchema::tryDecode(const std::vector<uint8_t>& message,
            const SerializationType& type) const {
            auto decoded = decodeAll(message, type);
            if (decoded.has_value()) {
                std::shared_ptr<Schema> sharedDecoded = std::move(decoded.value());
                return sharedDecoded;
            }
            else {
                return {};
            }
        }

        void NatImuBulkDataSchema::registerWithRegistry(Registry& registry) {
            const decoder_t decoder = [](const message_t& message, const SerializationType& type) {
                Optional<std::unique_ptr<NatImuBulkDataSchema>> decodedMaybe = decodeAll(message, type);
                if (decodedMaybe.has_value()) {
                    std::unique_ptr<Schema> convertedDecoded = std::move(decodedMaybe.value());
                    return Optional<std::unique_ptr<Schema>>{std::move(convertedDecoded)};
                }
                else {
                    return Optional<std::unique_ptr<Schema>>{};
                }
                };
            //registry.registerDecoder(name, SerializationType::Json, decoder);
            registry.registerDecoder(name, SerializationType::Csv, decoder);
            registry.registerDecoder(name, SerializationType::Binary, decoder);
        }

        std::string NatImuBulkDataSchema::getName() const { return name; }

        std::unique_ptr<std::vector<NatImuDataSchema>> NatImuBulkDataSchema::createImuRecords() const {
            auto records = nat::core::make_unique<std::vector<NatImuDataSchema>>();
            for (size_t i = 0; i < size; ++i)
                records->emplace_back(data[i]);
            return std::move(records);
        }

    } // namespace core
} // namespace nat
