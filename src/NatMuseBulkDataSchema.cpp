#include <libnatkit-core.hpp>
#include <iostream>

#ifdef SERVER
#include <nlohmann/json.hpp>
#endif
#include <string.h>

namespace nat {
namespace core {

const std::string NatMuseBulkDataSchema::name = "NatMuseBulkDataSchema";

NatMuseBulkDataSchema::NatMuseBulkDataSchema()
    : size(0) {
    for (size_t i = 0; i < BULK_SIZE; ++i) {
        data[i] = NatMuseDataSchema{};
    }
}

NatMuseBulkDataSchema::NatMuseBulkDataSchema(const NatMuseDataSchema* samples, uint8_t count)
    : size(count) {
    assert(count <= BULK_SIZE);
    for (size_t i = 0; i < count; ++i) {
        data[i] = samples[i];
    }
}

bool NatMuseBulkDataSchema::isFull() const {
    return size >= BULK_SIZE;
}

void NatMuseBulkDataSchema::add(const NatMuseDataSchema& sample) {
    assert(size < BULK_SIZE);
    data[size++] = sample;
}

void NatMuseBulkDataSchema::reset() {
    size = 0;
}

uint8_t NatMuseBulkDataSchema::getSize() const {
    return size;
}

const NatMuseDataSchema* NatMuseBulkDataSchema::getData() const {
    return data;
}

#ifdef SERVER

Optional<std::shared_ptr<NatMuseBulkDataSchema>> NatMuseBulkDataSchema::tryCreateFromSchema(
    const Optional<const std::shared_ptr<Schema>>& messageMaybe) {
    if (!messageMaybe.has_value() || messageMaybe.value() == nullptr) {
        return {};
    }
    if (messageMaybe.value()->getName() == NatMuseBulkDataSchema::name) {
        return std::dynamic_pointer_cast<NatMuseBulkDataSchema>(messageMaybe.value());
    }
    return {};
}

#endif

// Binary format for a single NatMuseDataSchema sample:
// - time: 8 bytes (uint64_t)
// - eeg_sequence: 2 bytes (uint16_t)
// - motion_sequence: 2 bytes (uint16_t)
// - has_data: 1 byte (uint8_t)
// - tp9[12]: 48 bytes (12 floats)
// - af7[12]: 48 bytes
// - af8[12]: 48 bytes
// - tp10[12]: 48 bytes
// - accel[3][3]: 36 bytes (9 floats)
// - gyro[3][3]: 36 bytes
// - ppg0[6]: 24 bytes
// - ppg1[6]: 24 bytes
// - ppg2[6]: 24 bytes
// Total per sample: 8 + 2 + 2 + 1 + 48*4 + 36*2 + 24*3 = 349 bytes

static const size_t SINGLE_SAMPLE_BINARY_SIZE = 349;

std::unique_ptr<std::vector<uint8_t>>
NatMuseBulkDataSchema::encodeToBytes(const SerializationType& type) const {
    switch (type) {
    case SerializationType::Binary: {
        auto bytes = make_unique<std::vector<uint8_t>>(SINGLE_SAMPLE_BINARY_SIZE * BULK_SIZE, 0);
        char* ptr = reinterpret_cast<char*>(bytes->data());
        
        for (size_t i = 0; i < BULK_SIZE; ++i) {
            const NatMuseDataSchema& sample = data[i];
            
            // Time
            ptr += Binary::unsafeWriteAsBinaryToArray<uint64_t>(ptr, sample.time);
            
            // Sequences
            ptr += Binary::unsafeWriteAsBinaryToArray<uint16_t>(ptr, sample.eeg_sequence);
            ptr += Binary::unsafeWriteAsBinaryToArray<uint16_t>(ptr, sample.motion_sequence);
            
            // has_data
            ptr += Binary::unsafeWriteAsBinaryToArray<uint8_t>(ptr, sample.has_data);
            
            // EEG data (4 channels × 12 samples)
            memcpy(ptr, sample.tp9, sizeof(sample.tp9)); ptr += sizeof(sample.tp9);
            memcpy(ptr, sample.af7, sizeof(sample.af7)); ptr += sizeof(sample.af7);
            memcpy(ptr, sample.af8, sizeof(sample.af8)); ptr += sizeof(sample.af8);
            memcpy(ptr, sample.tp10, sizeof(sample.tp10)); ptr += sizeof(sample.tp10);
            
            // Motion data
            memcpy(ptr, sample.accel, sizeof(sample.accel)); ptr += sizeof(sample.accel);
            memcpy(ptr, sample.gyro, sizeof(sample.gyro)); ptr += sizeof(sample.gyro);
            
            // PPG data
            memcpy(ptr, sample.ppg0, sizeof(sample.ppg0)); ptr += sizeof(sample.ppg0);
            memcpy(ptr, sample.ppg1, sizeof(sample.ppg1)); ptr += sizeof(sample.ppg1);
            memcpy(ptr, sample.ppg2, sizeof(sample.ppg2)); ptr += sizeof(sample.ppg2);
        }
        
        size_t writtenBytes = reinterpret_cast<size_t>(ptr) - reinterpret_cast<size_t>(bytes->data());
        assert(writtenBytes == SINGLE_SAMPLE_BINARY_SIZE * BULK_SIZE);
        
        return bytes;
    }

    case SerializationType::Csv: {
        auto bytes = make_unique<std::vector<uint8_t>>();
        bytes->reserve(32768);  // Reserve reasonable size
        
        for (size_t i = 0; i < size; ++i) {
            auto sampleBytes = data[i].encodeToBytes(SerializationType::Csv);
            if (sampleBytes) {
                bytes->insert(bytes->end(), sampleBytes->begin(), sampleBytes->end());
                if (i < size - 1) {
                    bytes->push_back(static_cast<uint8_t>('\n'));
                }
            }
        }
        
        return bytes;
    }

    case SerializationType::Json:
        // JSON not supported for bulk (too large)
        return nullptr;
    }
    
    return nullptr;
}

bool NatMuseBulkDataSchema::isSerializationTypeSupported(const SerializationType type) const {
    switch (type) {
    case SerializationType::Binary:
        return true;
    case SerializationType::Csv:
        return true;
    case SerializationType::Json:
        return false;
    default:
        return false;
    }
}

std::string NatMuseBulkDataSchema::toString() const {
    return getName() + ": [" + std::to_string(size) + " samples]";
}

Optional<std::unique_ptr<NatMuseBulkDataSchema>> NatMuseBulkDataSchema::decodeBinary(
    const std::vector<uint8_t>& message) {
    
    const size_t expectedSize = SINGLE_SAMPLE_BINARY_SIZE * BULK_SIZE;
    if (message.size() != expectedSize) {
        std::cerr << "NatMuseBulkDataSchema::decodeBinary: size mismatch. Expected " 
                  << expectedSize << " bytes, got " << message.size() << " bytes.\n";
        return {};
    }
    
    auto schema = make_unique<NatMuseBulkDataSchema>();
    const char* ptr = reinterpret_cast<const char*>(message.data());
    
    for (size_t i = 0; i < BULK_SIZE; ++i) {
        NatMuseDataSchema sample{};
        
        // Time
        ptr += Binary::unsafeParseFromBinary<uint64_t>(const_cast<char*>(ptr), sample.time);
        
        // Sequences
        ptr += Binary::unsafeParseFromBinary<uint16_t>(const_cast<char*>(ptr), sample.eeg_sequence);
        ptr += Binary::unsafeParseFromBinary<uint16_t>(const_cast<char*>(ptr), sample.motion_sequence);
        
        // has_data
        ptr += Binary::unsafeParseFromBinary<uint8_t>(const_cast<char*>(ptr), sample.has_data);
        
        // EEG data
        memcpy(sample.tp9, ptr, sizeof(sample.tp9)); ptr += sizeof(sample.tp9);
        memcpy(sample.af7, ptr, sizeof(sample.af7)); ptr += sizeof(sample.af7);
        memcpy(sample.af8, ptr, sizeof(sample.af8)); ptr += sizeof(sample.af8);
        memcpy(sample.tp10, ptr, sizeof(sample.tp10)); ptr += sizeof(sample.tp10);
        
        // Motion data
        memcpy(sample.accel, ptr, sizeof(sample.accel)); ptr += sizeof(sample.accel);
        memcpy(sample.gyro, ptr, sizeof(sample.gyro)); ptr += sizeof(sample.gyro);
        
        // PPG data
        memcpy(sample.ppg0, ptr, sizeof(sample.ppg0)); ptr += sizeof(sample.ppg0);
        memcpy(sample.ppg1, ptr, sizeof(sample.ppg1)); ptr += sizeof(sample.ppg1);
        memcpy(sample.ppg2, ptr, sizeof(sample.ppg2)); ptr += sizeof(sample.ppg2);
        
        schema->add(sample);
    }
    
    return std::move(schema);
}

Optional<std::unique_ptr<NatMuseBulkDataSchema>> NatMuseBulkDataSchema::decodeCsv(
    const std::vector<uint8_t>& message) {
    
    auto schema = make_unique<NatMuseBulkDataSchema>();
    size_t byteIndex = 0;
    
    while (byteIndex < message.size() && schema->size < BULK_SIZE) {
        std::vector<uint8_t> lineBytes;
        
        // Read until newline or end
        while (byteIndex < message.size() && message[byteIndex] != '\n') {
            lineBytes.push_back(message[byteIndex]);
            ++byteIndex;
        }
        
        // Skip the newline
        if (byteIndex < message.size()) {
            ++byteIndex;
        }
        
        // Decode the line
        if (!lineBytes.empty()) {
            auto sampleMaybe = NatMuseDataSchema::decodeCsv(lineBytes);
            if (sampleMaybe.has_value()) {
                schema->add(*sampleMaybe.value());
            }
        }
    }
    
    return std::move(schema);
}

Optional<std::unique_ptr<NatMuseBulkDataSchema>> NatMuseBulkDataSchema::decodeAll(
    const std::vector<uint8_t>& message, const SerializationType& type) {
    switch (type) {
    case SerializationType::Binary:
        return decodeBinary(message);
    case SerializationType::Csv:
        return decodeCsv(message);
    default:
        return {};
    }
}

void NatMuseBulkDataSchema::decodeAndDispatch(
    const std::vector<uint8_t>& message,
    const SerializationType& type,
    const std::function<void(const std::shared_ptr<Schema>&)>& dispatchMethod) {
    auto decodedMaybe = decodeAll(message, type);
    if (decodedMaybe.has_value()) {
        std::shared_ptr<NatMuseBulkDataSchema> shared = std::move(decodedMaybe.value());
        dispatchMethod(shared);
    }
}

Optional<std::shared_ptr<Schema>> NatMuseBulkDataSchema::tryDecode(
    const std::vector<uint8_t>& message, const SerializationType& type) const {
    auto decoded = decodeAll(message, type);
    if (decoded.has_value()) {
        std::shared_ptr<Schema> shared = std::move(decoded.value());
        return shared;
    }
    return {};
}

void NatMuseBulkDataSchema::registerWithRegistry(Registry& registry) {
    const decoder_t decoder = [](const message_t& message, const SerializationType& type) {
        auto decodedMaybe = decodeAll(message, type);
        if (decodedMaybe.has_value()) {
            std::unique_ptr<Schema> converted = std::move(decodedMaybe.value());
            return Optional<std::unique_ptr<Schema>>{std::move(converted)};
        }
        return Optional<std::unique_ptr<Schema>>{};
    };
    registry.registerDecoder(name, SerializationType::Binary, decoder);
    registry.registerDecoder(name, SerializationType::Csv, decoder);
}

std::string NatMuseBulkDataSchema::getName() const { return name; }

size_t NatMuseBulkDataSchema::encodeToBytesInPlace(uint8_t* buffer, size_t bufferLen) const {
    if (buffer == nullptr || bufferLen < BINARY_BUFFER_SIZE) {
        return 0;
    }
    
    char* ptr = reinterpret_cast<char*>(buffer);
    
    for (size_t i = 0; i < BULK_SIZE; ++i) {
        const NatMuseDataSchema& sample = data[i];
        
        // Time
        ptr += Binary::unsafeWriteAsBinaryToArray<uint64_t>(ptr, sample.time);
        
        // Sequences
        ptr += Binary::unsafeWriteAsBinaryToArray<uint16_t>(ptr, sample.eeg_sequence);
        ptr += Binary::unsafeWriteAsBinaryToArray<uint16_t>(ptr, sample.motion_sequence);
        
        // has_data
        ptr += Binary::unsafeWriteAsBinaryToArray<uint8_t>(ptr, sample.has_data);
        
        // EEG data (4 channels × 12 samples)
        memcpy(ptr, sample.tp9, sizeof(sample.tp9)); ptr += sizeof(sample.tp9);
        memcpy(ptr, sample.af7, sizeof(sample.af7)); ptr += sizeof(sample.af7);
        memcpy(ptr, sample.af8, sizeof(sample.af8)); ptr += sizeof(sample.af8);
        memcpy(ptr, sample.tp10, sizeof(sample.tp10)); ptr += sizeof(sample.tp10);
        
        // Motion data
        memcpy(ptr, sample.accel, sizeof(sample.accel)); ptr += sizeof(sample.accel);
        memcpy(ptr, sample.gyro, sizeof(sample.gyro)); ptr += sizeof(sample.gyro);
        
        // PPG data
        memcpy(ptr, sample.ppg0, sizeof(sample.ppg0)); ptr += sizeof(sample.ppg0);
        memcpy(ptr, sample.ppg1, sizeof(sample.ppg1)); ptr += sizeof(sample.ppg1);
        memcpy(ptr, sample.ppg2, sizeof(sample.ppg2)); ptr += sizeof(sample.ppg2);
    }
    
    return BINARY_BUFFER_SIZE;
}

std::unique_ptr<std::vector<NatMuseDataSchema>> NatMuseBulkDataSchema::createMuseRecords() const {
    auto records = make_unique<std::vector<NatMuseDataSchema>>();
    for (size_t i = 0; i < size; ++i) {
        records->emplace_back(data[i]);
    }
    return std::move(records);
}

} // namespace core
} // namespace nat
