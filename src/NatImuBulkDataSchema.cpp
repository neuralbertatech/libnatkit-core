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
        // ⚠️ VERSION 2 ADDS THE MAGNETOMETER. Everything recorded before 2026-08 is
        // version 1 and decodes through the v1 branch below; nothing needs migrating.
        const uint16_t NatImuBulkDataSchema::kFrameSchemaVersion = 2;
        const size_t NatImuBulkDataSchema::kFrameHeaderSize = 24;

        const uint8_t NatImuBulkDataSchema::kHasDataMaskV1 = 0x07;  // accel | gyro | rotation
        const uint8_t NatImuBulkDataSchema::kHasDataMaskV2 = 0x0F;  // ... | magnetometer

        const uint8_t NatImuBulkDataSchema::kAccuraciesMaskV1 = 0x3F;  // bits 5-0
        const uint8_t NatImuBulkDataSchema::kAccuraciesMaskV2 = 0xFF;  // ... + bits 7-6

        size_t NatImuBulkDataSchema::binaryFloatsPerSample(const uint16_t frameVersion) {
            // v1: accel(3) + gyro(3) + quat(4).  v2: ... + mag(3).
            return frameVersion >= 2 ? 13 : 10;
        }

        size_t NatImuBulkDataSchema::binarySampleSize(const uint16_t frameVersion) {
            // uint64 time + N float32 + uint8 accuracies + uint8 has_data.
            //   v1 -> 8 + 40 + 2 = 50 bytes,  v2 -> 8 + 52 + 2 = 62 bytes.
            return sizeof(uint64_t) + binaryFloatsPerSample(frameVersion) * sizeof(float) + 2;
        }
        static const int NatImuBulkDataSchemaDataArraySize = 100;

        NatImuBulkDataSchema::NatImuBulkDataSchema()
            : size(0), schemaVersion(kFrameSchemaVersion), sampleRateHz(0), seqNo(0), deviceTsUs(0) {
            for (size_t i = 0; i < NatImuBulkDataSchemaDataArraySize; ++i)
                data[i] = NatImuDataSchema{};
        }

        NatImuBulkDataSchema::NatImuBulkDataSchema(const NatImuDataSchema* data, uint8_t size)
            : size(size), schemaVersion(kFrameSchemaVersion), sampleRateHz(0), seqNo(0), deviceTsUs(0) {
            assert(size <= NatImuBulkDataSchemaDataArraySize);
            for (int i = 0; i < size; ++i)
                this->data[i] = data[i];
        }

        void NatImuBulkDataSchema::setFrameHeader(uint64_t seqNo, uint64_t deviceTsUs, uint32_t sampleRateHz) {
            this->seqNo = seqNo;
            this->deviceTsUs = deviceTsUs;
            this->sampleRateHz = sampleRateHz;
        }

        uint16_t NatImuBulkDataSchema::getSchemaVersion() const { return schemaVersion; }
        uint64_t NatImuBulkDataSchema::getSeqNo() const { return seqNo; }
        uint64_t NatImuBulkDataSchema::getDeviceTsUs() const { return deviceTsUs; }
        uint32_t NatImuBulkDataSchema::getSampleRateHz() const { return sampleRateHz; }
        uint8_t NatImuBulkDataSchema::getSampleCount() const { return size; }
        const NatImuDataSchema* NatImuBulkDataSchema::getSamples() const { return data; }

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
                // Written at the CURRENT version, which carries the magnetometer.
                const size_t floatsPerSample = binaryFloatsPerSample(kFrameSchemaVersion);
                const size_t singleReadingSizeInBytes = binarySampleSize(kFrameSchemaVersion);
                const uint16_t sampleCount = static_cast<uint16_t>(size);
                const size_t totalSize = kFrameHeaderSize + static_cast<size_t>(singleReadingSizeInBytes) * sampleCount;
                auto bytes = nat::core::make_unique<std::vector<uint8_t>>(totalSize, 0);
                char* dataPointer = (char*)bytes->data();

                // Frame header (24 bytes, little-endian).
                // ⚠️ kFrameSchemaVersion, NOT the member. The member holds the version
                // a frame was DECODED from, so echoing it here would re-emit a v1
                // header in front of a v2-sized body after any decode/encode round
                // trip -- a length mismatch at the far end with nothing to point at.
                dataPointer += Binary::unsafeWriteAsBinaryToArray<uint16_t>(dataPointer, kFrameSchemaVersion);
                dataPointer += Binary::unsafeWriteAsBinaryToArray<uint16_t>(dataPointer, sampleCount);
                dataPointer += Binary::unsafeWriteAsBinaryToArray<uint32_t>(dataPointer, sampleRateHz);
                dataPointer += Binary::unsafeWriteAsBinaryToArray<uint64_t>(dataPointer, seqNo);
                dataPointer += Binary::unsafeWriteAsBinaryToArray<uint64_t>(dataPointer, deviceTsUs);

                for (size_t i = 0; i < sampleCount; ++i) {
                    dataPointer += Binary::unsafeWriteAsBinaryToArray<uint64_t>(dataPointer, data[i].time);
                    for (size_t j = 0; j < floatsPerSample; ++j) {
                        // Use memcpy to preserve float bit pattern (not value conversion)
                        memcpy(dataPointer, &data[i].data[j], sizeof(float));
                        dataPointer += sizeof(float);
                    }
                    dataPointer += Binary::unsafeWriteAsBinaryToArray<uint8_t>(dataPointer, static_cast<uint8_t>(data[i].accuracies));
                    dataPointer += Binary::unsafeWriteAsBinaryToArray<uint8_t>(dataPointer, static_cast<uint8_t>(data[i].has_data));
                }
                size_t pointerDiff = (size_t)dataPointer - (size_t)bytes->data();
                assert(pointerDiff == totalSize);
                assert(bytes->size() == totalSize);
                (void)pointerDiff;
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
            // ⚠️ THE SAMPLE SIZE DEPENDS ON THE FRAME VERSION, so it cannot be read
            // until the header has been. v1 samples are 50 bytes (10 floats), v2 are
            // 62 (13, adding the magnetometer). This used to be one hard-coded 50 in
            // this function and another in the encoder.
            const size_t legacySize = static_cast<size_t>(binarySampleSize(1)) * NatImuBulkDataSchemaDataArraySize; // 5000

            auto schema = nat::core::make_unique<NatImuBulkDataSchema>();

            // Reads one fixed-size sample from `bytes`, advancing the pointer.
            // Floats the frame does not carry are left at zero and their has_data
            // bit is cleared by the caller's mask.
            const auto readSample = [&](char*& bytes, size_t floatsPerSample, uint8_t hasDataMask,
                                        uint8_t accuraciesMask) {
                NatImuDataSchema sensorReading{};
                bytes += Binary::unsafeParseFromBinary(bytes, sensorReading.time);
                for (size_t j = 0; j < floatsPerSample; ++j) {
                    // Use memcpy to read float bit pattern directly
                    memcpy(&sensorReading.data[j], bytes, sizeof(float));
                    bytes += sizeof(float);
                }
                bytes += Binary::unsafeParseFromBinary<uint8_t>(bytes, *(uint8_t*)&sensorReading.accuracies);
                bytes += Binary::unsafeParseFromBinary<uint8_t>(bytes, *(uint8_t*)&sensorReading.has_data);
                sensorReading.has_data &= hasDataMask;
                sensorReading.accuracies &= accuraciesMask;
                schema->add(sensorReading);
            };

            // Legacy headerless format: exactly 100 samples, no frame envelope, and
            // always v1-shaped.
            //
            // ⚠️ THE SENTINEL STILL HAS TO BE UNAMBIGUOUS AT EVERY VERSION, and that
            // is now two claims rather than one: 24 + n*50 == 5000 has no integer
            // solution, and neither does 24 + n*62 == 5000 (4976/62 = 80.26). Adding
            // a version 3 means re-checking this, so binarySampleSize() carries the
            // same warning.
            if (message.size() == legacySize) {
                char* bytes = (char*)message.data();
                for (int i = 0; i < NatImuBulkDataSchemaDataArraySize; ++i)
                    readSample(bytes, binaryFloatsPerSample(1), kHasDataMaskV1, kAccuraciesMaskV1);
                return std::move(schema);
            }

            if (message.size() < kFrameHeaderSize) {
                std::cerr << "NatImuBulkDataSchema::decodeBinary: message too small for header ("
                          << message.size() << " < " << kFrameHeaderSize << " bytes).\n";
                return {};
            }

            char* bytes = (char*)message.data();
            uint16_t decodedSchemaVersion = 0;
            uint16_t sampleCount = 0;
            uint32_t decodedSampleRateHz = 0;
            uint64_t decodedSeqNo = 0;
            uint64_t decodedDeviceTsUs = 0;
            bytes += Binary::unsafeParseFromBinary<uint16_t>(bytes, decodedSchemaVersion);
            bytes += Binary::unsafeParseFromBinary<uint16_t>(bytes, sampleCount);
            bytes += Binary::unsafeParseFromBinary<uint32_t>(bytes, decodedSampleRateHz);
            bytes += Binary::unsafeParseFromBinary<uint64_t>(bytes, decodedSeqNo);
            bytes += Binary::unsafeParseFromBinary<uint64_t>(bytes, decodedDeviceTsUs);

            if (sampleCount > NatImuBulkDataSchemaDataArraySize) {
                std::cerr << "NatImuBulkDataSchema::decodeBinary: sampleCount " << sampleCount
                          << " exceeds capacity " << NatImuBulkDataSchemaDataArraySize << ".\n";
                return {};
            }
            if (decodedSchemaVersion == 0 || decodedSchemaVersion > kFrameSchemaVersion) {
                std::cerr << "NatImuBulkDataSchema::decodeBinary: unsupported frame version "
                          << decodedSchemaVersion << " (this build understands 1.."
                          << kFrameSchemaVersion << "). A newer writer is on the wire.\n";
                return {};
            }
            const size_t floatsPerSample = binaryFloatsPerSample(decodedSchemaVersion);
            const size_t singleReadingSizeInBytes = binarySampleSize(decodedSchemaVersion);
            const uint8_t hasDataMask =
                decodedSchemaVersion >= 2 ? kHasDataMaskV2 : kHasDataMaskV1;
            const uint8_t accuraciesMask =
                decodedSchemaVersion >= 2 ? kAccuraciesMaskV2 : kAccuraciesMaskV1;

            const size_t expectedSize = kFrameHeaderSize + singleReadingSizeInBytes * sampleCount;
            if (message.size() != expectedSize) {
                std::cerr << "NatImuBulkDataSchema::decodeBinary: framed message size mismatch. Expected "
                          << expectedSize << " bytes for a version " << decodedSchemaVersion
                          << " frame of " << sampleCount << " samples, got " << message.size() << " bytes.\n";
                return {};
            }

            for (int i = 0; i < sampleCount; ++i)
                readSample(bytes, floatsPerSample, hasDataMask, accuraciesMask);
            schema->setFrameHeader(decodedSeqNo, decodedDeviceTsUs, decodedSampleRateHz);
            schema->schemaVersion = decodedSchemaVersion;
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
