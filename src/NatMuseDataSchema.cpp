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

const std::string NatMuseDataSchema::name = "NatMuseDataSchema";

NatMuseDataSchema::NatMuseDataSchema()
    : time(0), eeg_sequence(0), motion_sequence(0), has_data(0) {
    memset(tp9, 0, sizeof(tp9));
    memset(af7, 0, sizeof(af7));
    memset(af8, 0, sizeof(af8));
    memset(tp10, 0, sizeof(tp10));
    memset(accel, 0, sizeof(accel));
    memset(gyro, 0, sizeof(gyro));
    memset(ppg0, 0, sizeof(ppg0));
    memset(ppg1, 0, sizeof(ppg1));
    memset(ppg2, 0, sizeof(ppg2));
}

NatMuseDataSchema::NatMuseDataSchema(const NatMuseDataSchema& other)
    : time(other.time), eeg_sequence(other.eeg_sequence), 
      motion_sequence(other.motion_sequence), has_data(other.has_data) {
    memcpy(tp9, other.tp9, sizeof(tp9));
    memcpy(af7, other.af7, sizeof(af7));
    memcpy(af8, other.af8, sizeof(af8));
    memcpy(tp10, other.tp10, sizeof(tp10));
    memcpy(accel, other.accel, sizeof(accel));
    memcpy(gyro, other.gyro, sizeof(gyro));
    memcpy(ppg0, other.ppg0, sizeof(ppg0));
    memcpy(ppg1, other.ppg1, sizeof(ppg1));
    memcpy(ppg2, other.ppg2, sizeof(ppg2));
}

NatMuseDataSchema::NatMuseDataSchema(
    uint64_t time,
    uint16_t eeg_sequence,
    uint16_t motion_sequence,
    const float tp9_data[EEG_SAMPLES_PER_PACKET],
    const float af7_data[EEG_SAMPLES_PER_PACKET],
    const float af8_data[EEG_SAMPLES_PER_PACKET],
    const float tp10_data[EEG_SAMPLES_PER_PACKET],
    const float accel_data[MOTION_SAMPLES][3],
    const float gyro_data[MOTION_SAMPLES][3],
    const float ppg0_data[PPG_SAMPLES],
    const float ppg1_data[PPG_SAMPLES],
    const float ppg2_data[PPG_SAMPLES],
    uint8_t has_data)
    : time(time), eeg_sequence(eeg_sequence), motion_sequence(motion_sequence), has_data(has_data) {
    
    if (tp9_data) memcpy(tp9, tp9_data, sizeof(tp9)); else memset(tp9, 0, sizeof(tp9));
    if (af7_data) memcpy(af7, af7_data, sizeof(af7)); else memset(af7, 0, sizeof(af7));
    if (af8_data) memcpy(af8, af8_data, sizeof(af8)); else memset(af8, 0, sizeof(af8));
    if (tp10_data) memcpy(tp10, tp10_data, sizeof(tp10)); else memset(tp10, 0, sizeof(tp10));
    if (accel_data) memcpy(accel, accel_data, sizeof(accel)); else memset(accel, 0, sizeof(accel));
    if (gyro_data) memcpy(gyro, gyro_data, sizeof(gyro)); else memset(gyro, 0, sizeof(gyro));
    if (ppg0_data) memcpy(ppg0, ppg0_data, sizeof(ppg0)); else memset(ppg0, 0, sizeof(ppg0));
    if (ppg1_data) memcpy(ppg1, ppg1_data, sizeof(ppg1)); else memset(ppg1, 0, sizeof(ppg1));
    if (ppg2_data) memcpy(ppg2, ppg2_data, sizeof(ppg2)); else memset(ppg2, 0, sizeof(ppg2));
}

#ifdef SERVER

Optional<std::shared_ptr<NatMuseDataSchema>> NatMuseDataSchema::tryCreateFromSchema(
    const Optional<const std::shared_ptr<Schema>>& messageMaybe) {
    if (!messageMaybe.has_value() || messageMaybe.value() == nullptr) {
        return {};
    }
    if (messageMaybe.value()->getName() == NatMuseDataSchema::name) {
        return std::dynamic_pointer_cast<NatMuseDataSchema>(messageMaybe.value());
    }
    return {};
}

#endif

void NatMuseDataSchema::setEegData(uint16_t sequence, 
                                    const float tp9_data[EEG_SAMPLES_PER_PACKET],
                                    const float af7_data[EEG_SAMPLES_PER_PACKET],
                                    const float af8_data[EEG_SAMPLES_PER_PACKET],
                                    const float tp10_data[EEG_SAMPLES_PER_PACKET]) {
    eeg_sequence = sequence;
    memcpy(tp9, tp9_data, sizeof(tp9));
    memcpy(af7, af7_data, sizeof(af7));
    memcpy(af8, af8_data, sizeof(af8));
    memcpy(tp10, tp10_data, sizeof(tp10));
    has_data |= HAS_EEG;
}

void NatMuseDataSchema::setAccelData(uint16_t sequence, const float data[MOTION_SAMPLES][3]) {
    motion_sequence = sequence;
    memcpy(accel, data, sizeof(accel));
    has_data |= HAS_ACCEL;
}

void NatMuseDataSchema::setGyroData(uint16_t sequence, const float data[MOTION_SAMPLES][3]) {
    motion_sequence = sequence;
    memcpy(gyro, data, sizeof(gyro));
    has_data |= HAS_GYRO;
}

void NatMuseDataSchema::setPpgData(const float ppg0_data[PPG_SAMPLES],
                                    const float ppg1_data[PPG_SAMPLES],
                                    const float ppg2_data[PPG_SAMPLES]) {
    memcpy(ppg0, ppg0_data, sizeof(ppg0));
    memcpy(ppg1, ppg1_data, sizeof(ppg1));
    memcpy(ppg2, ppg2_data, sizeof(ppg2));
    has_data |= HAS_PPG;
}

void NatMuseDataSchema::setTime(uint64_t t) {
    time = t;
}

#ifdef SERVER
static nlohmann::json floatArrayToJson(const float* arr, size_t size) {
    nlohmann::json jsonArray = nlohmann::json::array();
    for (size_t i = 0; i < size; ++i) {
        jsonArray.push_back(arr[i]);
    }
    return jsonArray;
}

static nlohmann::json motionSamplesToJson(const float data[NatMuseDataSchema::MOTION_SAMPLES][3]) {
    nlohmann::json jsonArray = nlohmann::json::array();
    for (size_t i = 0; i < NatMuseDataSchema::MOTION_SAMPLES; ++i) {
        nlohmann::json sample;
        sample["x"] = data[i][0];
        sample["y"] = data[i][1];
        sample["z"] = data[i][2];
        jsonArray.push_back(sample);
    }
    return jsonArray;
}
#endif

std::unique_ptr<std::vector<uint8_t>>
NatMuseDataSchema::encodeToBytes(const SerializationType& type) const {
    switch (type) {
    case SerializationType::Json: {
#ifdef SERVER
        nlohmann::json j;
        j["time"] = time;
        j["eeg_sequence"] = eeg_sequence;
        j["motion_sequence"] = motion_sequence;
        
        j["eeg"]["tp9"] = floatArrayToJson(tp9, EEG_SAMPLES_PER_PACKET);
        j["eeg"]["af7"] = floatArrayToJson(af7, EEG_SAMPLES_PER_PACKET);
        j["eeg"]["af8"] = floatArrayToJson(af8, EEG_SAMPLES_PER_PACKET);
        j["eeg"]["tp10"] = floatArrayToJson(tp10, EEG_SAMPLES_PER_PACKET);
        
        j["accel"] = motionSamplesToJson(accel);
        j["gyro"] = motionSamplesToJson(gyro);
        
        j["ppg"]["ppg0"] = floatArrayToJson(ppg0, PPG_SAMPLES);
        j["ppg"]["ppg1"] = floatArrayToJson(ppg1, PPG_SAMPLES);
        j["ppg"]["ppg2"] = floatArrayToJson(ppg2, PPG_SAMPLES);
        
        j["has_data"]["eeg"] = hasEegData();
        j["has_data"]["accel"] = hasAccelData();
        j["has_data"]["gyro"] = hasGyroData();
        j["has_data"]["ppg"] = hasPpgData();
        
        const auto jsonStr = j.dump();
        return make_unique<std::vector<uint8_t>>(std::begin(jsonStr), std::end(jsonStr));
#else
        return nullptr;
#endif
    }

    case SerializationType::Csv: {
        std::string csv;
        csv += std::to_string(time) + ",";
        csv += std::to_string(eeg_sequence) + ",";
        csv += std::to_string(motion_sequence) + ",";
        csv += std::to_string(has_data) + ",";
        
        // EEG data (48 floats)
        for (int i = 0; i < EEG_SAMPLES_PER_PACKET; ++i) csv += std::to_string(tp9[i]) + ",";
        for (int i = 0; i < EEG_SAMPLES_PER_PACKET; ++i) csv += std::to_string(af7[i]) + ",";
        for (int i = 0; i < EEG_SAMPLES_PER_PACKET; ++i) csv += std::to_string(af8[i]) + ",";
        for (int i = 0; i < EEG_SAMPLES_PER_PACKET; ++i) csv += std::to_string(tp10[i]) + ",";
        
        // Motion data (18 floats)
        for (int i = 0; i < MOTION_SAMPLES; ++i) {
            csv += std::to_string(accel[i][0]) + ",";
            csv += std::to_string(accel[i][1]) + ",";
            csv += std::to_string(accel[i][2]) + ",";
        }
        for (int i = 0; i < MOTION_SAMPLES; ++i) {
            csv += std::to_string(gyro[i][0]) + ",";
            csv += std::to_string(gyro[i][1]) + ",";
            csv += std::to_string(gyro[i][2]) + ",";
        }
        
        // PPG data (18 floats)
        for (int i = 0; i < PPG_SAMPLES; ++i) csv += std::to_string(ppg0[i]) + ",";
        for (int i = 0; i < PPG_SAMPLES; ++i) csv += std::to_string(ppg1[i]) + ",";
        for (int i = 0; i < PPG_SAMPLES - 1; ++i) csv += std::to_string(ppg2[i]) + ",";
        csv += std::to_string(ppg2[PPG_SAMPLES - 1]);  // Last value without comma
        
        return nat::core::make_unique<std::vector<uint8_t>>(csv.begin(), csv.end());
    }

    case SerializationType::Binary:
        // Binary encoding not supported for single samples (only bulk)
        return nullptr;
    }
    
    return nullptr;
}

bool NatMuseDataSchema::isSerializationTypeSupported(const SerializationType type) const {
    switch (type) {
    case SerializationType::Json:
        return true;
    case SerializationType::Csv:
        return true;
    case SerializationType::Binary:
        return false;  // Only bulk supports binary
    default:
        return false;
    }
}

std::string NatMuseDataSchema::toString() const {
    return getName() + ": {time: " + std::to_string(time) + 
           ", eeg_seq: " + std::to_string(eeg_sequence) +
           ", motion_seq: " + std::to_string(motion_sequence) +
           ", has_data: " + std::to_string(has_data) + "}";
}

#ifdef SERVER
Optional<std::unique_ptr<NatMuseDataSchema>> NatMuseDataSchema::decodeJson(
    const std::vector<uint8_t>& message) {
    try {
        std::string jsonStr(std::begin(message), std::end(message));
        const auto json = nlohmann::json::parse(jsonStr);
        
        auto schema = make_unique<NatMuseDataSchema>();
        schema->time = json.at("time").get<uint64_t>();
        schema->eeg_sequence = json.at("eeg_sequence").get<uint16_t>();
        schema->motion_sequence = json.at("motion_sequence").get<uint16_t>();
        
        // EEG
        auto tp9_arr = json.at("eeg").at("tp9").get<std::vector<float>>();
        auto af7_arr = json.at("eeg").at("af7").get<std::vector<float>>();
        auto af8_arr = json.at("eeg").at("af8").get<std::vector<float>>();
        auto tp10_arr = json.at("eeg").at("tp10").get<std::vector<float>>();
        for (int i = 0; i < EEG_SAMPLES_PER_PACKET && i < (int)tp9_arr.size(); ++i) schema->tp9[i] = tp9_arr[i];
        for (int i = 0; i < EEG_SAMPLES_PER_PACKET && i < (int)af7_arr.size(); ++i) schema->af7[i] = af7_arr[i];
        for (int i = 0; i < EEG_SAMPLES_PER_PACKET && i < (int)af8_arr.size(); ++i) schema->af8[i] = af8_arr[i];
        for (int i = 0; i < EEG_SAMPLES_PER_PACKET && i < (int)tp10_arr.size(); ++i) schema->tp10[i] = tp10_arr[i];
        
        // Motion
        auto accel_arr = json.at("accel");
        auto gyro_arr = json.at("gyro");
        for (int i = 0; i < MOTION_SAMPLES && i < (int)accel_arr.size(); ++i) {
            schema->accel[i][0] = accel_arr[i]["x"].get<float>();
            schema->accel[i][1] = accel_arr[i]["y"].get<float>();
            schema->accel[i][2] = accel_arr[i]["z"].get<float>();
        }
        for (int i = 0; i < MOTION_SAMPLES && i < (int)gyro_arr.size(); ++i) {
            schema->gyro[i][0] = gyro_arr[i]["x"].get<float>();
            schema->gyro[i][1] = gyro_arr[i]["y"].get<float>();
            schema->gyro[i][2] = gyro_arr[i]["z"].get<float>();
        }
        
        // PPG
        auto ppg0_arr = json.at("ppg").at("ppg0").get<std::vector<float>>();
        auto ppg1_arr = json.at("ppg").at("ppg1").get<std::vector<float>>();
        auto ppg2_arr = json.at("ppg").at("ppg2").get<std::vector<float>>();
        for (int i = 0; i < PPG_SAMPLES && i < (int)ppg0_arr.size(); ++i) schema->ppg0[i] = ppg0_arr[i];
        for (int i = 0; i < PPG_SAMPLES && i < (int)ppg1_arr.size(); ++i) schema->ppg1[i] = ppg1_arr[i];
        for (int i = 0; i < PPG_SAMPLES && i < (int)ppg2_arr.size(); ++i) schema->ppg2[i] = ppg2_arr[i];
        
        // has_data
        schema->has_data = 0;
        if (json.at("has_data").at("eeg").get<bool>()) schema->has_data |= HAS_EEG;
        if (json.at("has_data").at("accel").get<bool>()) schema->has_data |= HAS_ACCEL;
        if (json.at("has_data").at("gyro").get<bool>()) schema->has_data |= HAS_GYRO;
        if (json.at("has_data").at("ppg").get<bool>()) schema->has_data |= HAS_PPG;
        
        return std::move(schema);
    } catch (const std::exception& e) {
        std::cerr << "NatMuseDataSchema::decodeJson error: " << e.what() << std::endl;
        return {};
    }
}
#endif

Optional<std::unique_ptr<NatMuseDataSchema>> NatMuseDataSchema::decodeCsv(
    const std::vector<uint8_t>& message) {
    // CSV decoding - parse comma-separated values
    std::string messageStr(std::begin(message), std::end(message));
    auto values = Strings::split(messageStr, ',');
    
    if (values.size() < 4 + 48 + 18 + 18) {  // time + seqs + has_data + eeg + motion + ppg
        return {};
    }
    
    auto schema = make_unique<NatMuseDataSchema>();
    size_t idx = 0;
    
    schema->time = std::stoull(values[idx++]);
    schema->eeg_sequence = static_cast<uint16_t>(std::stoul(values[idx++]));
    schema->motion_sequence = static_cast<uint16_t>(std::stoul(values[idx++]));
    schema->has_data = static_cast<uint8_t>(std::stoul(values[idx++]));
    
    // EEG
    for (int i = 0; i < EEG_SAMPLES_PER_PACKET; ++i) schema->tp9[i] = std::stof(values[idx++]);
    for (int i = 0; i < EEG_SAMPLES_PER_PACKET; ++i) schema->af7[i] = std::stof(values[idx++]);
    for (int i = 0; i < EEG_SAMPLES_PER_PACKET; ++i) schema->af8[i] = std::stof(values[idx++]);
    for (int i = 0; i < EEG_SAMPLES_PER_PACKET; ++i) schema->tp10[i] = std::stof(values[idx++]);
    
    // Motion
    for (int i = 0; i < MOTION_SAMPLES; ++i) {
        schema->accel[i][0] = std::stof(values[idx++]);
        schema->accel[i][1] = std::stof(values[idx++]);
        schema->accel[i][2] = std::stof(values[idx++]);
    }
    for (int i = 0; i < MOTION_SAMPLES; ++i) {
        schema->gyro[i][0] = std::stof(values[idx++]);
        schema->gyro[i][1] = std::stof(values[idx++]);
        schema->gyro[i][2] = std::stof(values[idx++]);
    }
    
    // PPG
    for (int i = 0; i < PPG_SAMPLES; ++i) schema->ppg0[i] = std::stof(values[idx++]);
    for (int i = 0; i < PPG_SAMPLES; ++i) schema->ppg1[i] = std::stof(values[idx++]);
    for (int i = 0; i < PPG_SAMPLES; ++i) schema->ppg2[i] = std::stof(values[idx++]);
    
    return std::move(schema);
}

Optional<std::unique_ptr<NatMuseDataSchema>> NatMuseDataSchema::decodeAll(
    const std::vector<uint8_t>& message, const SerializationType& type) {
    switch (type) {
    case SerializationType::Json:
#ifdef SERVER
        return decodeJson(message);
#else
        return {};
#endif
    case SerializationType::Csv:
        return decodeCsv(message);
    default:
        return {};
    }
}

void NatMuseDataSchema::decodeAndDispatch(
    const std::vector<uint8_t>& message,
    const SerializationType& type,
    const std::function<void(const std::shared_ptr<Schema>&)>& dispatchMethod) {
    auto decodedMaybe = decodeAll(message, type);
    if (decodedMaybe.has_value()) {
        std::shared_ptr<NatMuseDataSchema> shared = std::move(decodedMaybe.value());
        dispatchMethod(shared);
    }
}

Optional<std::shared_ptr<Schema>> NatMuseDataSchema::tryDecode(
    const std::vector<uint8_t>& message, const SerializationType& type) const {
    auto decoded = decodeAll(message, type);
    if (decoded.has_value()) {
        std::shared_ptr<Schema> shared = std::move(decoded.value());
        return shared;
    }
    return {};
}

void NatMuseDataSchema::registerWithRegistry(Registry& registry) {
    const decoder_t decoder = [](const message_t& message, const SerializationType& type) {
        auto decodedMaybe = decodeAll(message, type);
        if (decodedMaybe.has_value()) {
            std::unique_ptr<Schema> converted = std::move(decodedMaybe.value());
            return Optional<std::unique_ptr<Schema>>{std::move(converted)};
        }
        return Optional<std::unique_ptr<Schema>>{};
    };
    registry.registerDecoder(name, SerializationType::Json, decoder);
    registry.registerDecoder(name, SerializationType::Csv, decoder);
}

std::string NatMuseDataSchema::getName() const { return name; }

uint64_t NatMuseDataSchema::getTime() const { return time; }
uint16_t NatMuseDataSchema::getEegSequence() const { return eeg_sequence; }
uint16_t NatMuseDataSchema::getMotionSequence() const { return motion_sequence; }

const float* NatMuseDataSchema::getTp9() const { return tp9; }
const float* NatMuseDataSchema::getAf7() const { return af7; }
const float* NatMuseDataSchema::getAf8() const { return af8; }
const float* NatMuseDataSchema::getTp10() const { return tp10; }

const float (*NatMuseDataSchema::getAccel() const)[3] { return accel; }
const float (*NatMuseDataSchema::getGyro() const)[3] { return gyro; }

const float* NatMuseDataSchema::getPpg0() const { return ppg0; }
const float* NatMuseDataSchema::getPpg1() const { return ppg1; }
const float* NatMuseDataSchema::getPpg2() const { return ppg2; }

bool NatMuseDataSchema::hasEegData() const { return (has_data & HAS_EEG) != 0; }
bool NatMuseDataSchema::hasAccelData() const { return (has_data & HAS_ACCEL) != 0; }
bool NatMuseDataSchema::hasGyroData() const { return (has_data & HAS_GYRO) != 0; }
bool NatMuseDataSchema::hasPpgData() const { return (has_data & HAS_PPG) != 0; }

} // namespace core
} // namespace nat
