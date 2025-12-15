#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <type_traits>


namespace nat {
namespace core {

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename T>
class Optional {
  std::unique_ptr<T> valMaybe = nullptr;

public:
  Optional() : valMaybe(nullptr) {}
  Optional(T val) : valMaybe(new T{std::move(val)}) {}
  Optional(const Optional<T>& other) {
    if (other.has_value()) {
      valMaybe = make_unique<T>(*other.valMaybe);
    } else {
      valMaybe = nullptr;
    }
  }
  Optional(Optional<T>&& other) = default;

  ~Optional() = default;

  Optional<T>& operator=(const Optional<T>& other) {
    if (this != &other) {
      valMaybe.reset();
      if (other.has_value()) {
        valMaybe = make_unique<T>(*other.valMaybe);
      } else {
        valMaybe = nullptr;
      }
    }
    return *this;
  }

  Optional<T>& operator=(Optional<T>&& other) = default;

  bool has_value() const { return valMaybe != nullptr; }
  T& value() const { return *valMaybe; }
  void set(T val) { 
    if (has_value())
      valMaybe.reset();
    valMaybe = make_unique<T>(std::move(val));
  }
};

namespace Strings {

inline std::vector<std::string> split(const std::string& string, char delimiter) {
    size_t prevIndex = 0;
    size_t nextIndex = 0;
    std::vector<std::string> strings;
    while(true) {
        nextIndex = string.find(delimiter, prevIndex);
        if (nextIndex == std::string::npos) {
        break;
    }
        strings.emplace_back(string.substr(prevIndex, nextIndex-prevIndex));
        prevIndex = nextIndex + 1;
    }
    strings.emplace_back(string.substr(prevIndex, string.size()-prevIndex));

    return strings;
}

inline std::string toLowercase(const std::string& string) {
    std::string lowercaseString{string};
    std::transform(lowercaseString.begin(), lowercaseString.end(), lowercaseString.begin(), [](const char character) { return std::tolower(character); });
    return lowercaseString;
}

}

namespace Vectors {

template <typename T>
inline std::vector<std::unique_ptr<T>> wrapContainedValueWithUnique(const std::vector<T>& vec) {
  std::vector<std::unique_ptr<T>> wrappedVec{};
  for (const auto& val : vec) {
	  wrappedVec.emplace_back(nat::core::make_unique<T>(val));
  }

  return wrappedVec;
}

}

namespace Binary {

template <typename T>
inline size_t unsafeWriteAsBinaryToArray(char* array, T value) {
  //static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
  memcpy(array, &value, sizeof(T));
  return sizeof(T);
}

template <typename T>
inline size_t unsafeParseFromBinary(char* array, T& value) {
  //static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
  memcpy(&value, array, sizeof(T));
  return sizeof(T);
}

} // namespace Binary

using message_t = std::vector<uint8_t>;

inline std::string toString(const message_t& msg) {
    return std::string(msg.begin(), msg.end());
}

class StreamInfo {
  std::string streamName;

  public:
  StreamInfo(const std::string& streamName) : streamName(streamName) {}
};



// Enums ///////////////////////////////////////////////////////////////////////
enum class SerializationType {
  Json,
  Csv,
  Binary
};

static const std::unordered_map<SerializationType, std::string>
    serializationTypeToStringMapping = {
        {SerializationType::Json, "Json"},
        {SerializationType::Csv, "CSV"},
        {SerializationType::Binary, "Binary"},
};

static const std::unordered_map<std::string, SerializationType>
    stringToSerializationTypeMapping = []() {
      std::unordered_map<std::string, SerializationType> newMap{};
      std::transform(
          serializationTypeToStringMapping.begin(), serializationTypeToStringMapping.end(),
          std::inserter(newMap, newMap.end()),
          [](const std::pair<SerializationType, std::string> &pair) -> std::pair<std::string, SerializationType> {
            return {Strings::toLowercase(pair.second), pair.first};
          });
      return newMap;
    }();

static const std::unordered_map<std::string, SerializationType>
    lowercaseStringToSerializationTypeMapping = []() {
      std::unordered_map<std::string, SerializationType> lowercaseMap{};
      for (const auto& pair : stringToSerializationTypeMapping) {
        const auto key = pair.first;
        const auto val = pair.second;
        lowercaseMap[Strings::toLowercase(key)] = val;
      }
      return lowercaseMap;
    }();

SerializationType getSerializationType(const std::string& encoderName);

std::string toString(const SerializationType& streamType);

Optional<SerializationType>
serializationTypeFromString(const std::string &streamTypeString);


enum class StreamType {
  // Core types
  DATA,
  META,

  // Execution Extension
  EXECUTION_COMMAND,

  // Hardware Extension
  HARDWARE_STATUS,
  HARDWARE_CONFIGURATION,

  // Logging Extension
  LOGGING_LOG,
  LOGGING_HEARTBEAT,
};

static const std::unordered_map<StreamType, std::string>
    streamTypeToStringMapping = {
        {StreamType::DATA, "Data"},
        {StreamType::META, "Meta"},
        {StreamType::EXECUTION_COMMAND, "Command"},
        {StreamType::HARDWARE_STATUS, "Status"},
        {StreamType::HARDWARE_CONFIGURATION, "Configuration"},
        {StreamType::LOGGING_LOG, "Log"},
        {StreamType::LOGGING_HEARTBEAT, "Heartbeat"},
};

static const std::unordered_map<std::string, StreamType>
    stringToStreamTypeMapping = []() {
      std::unordered_map<std::string, StreamType> newMap{};
      std::transform(
          streamTypeToStringMapping.begin(), streamTypeToStringMapping.end(),
          std::inserter(newMap, newMap.end()),
          [](const std::pair<StreamType, std::string> &pair) -> std::pair<std::string, StreamType> {
            return {Strings::toLowercase(pair.second), pair.first};
          });
      return newMap;
    }();

static const std::unordered_map<std::string, StreamType>
    lowercaseStringToStreamTypeMapping = []() {
      std::unordered_map<std::string, StreamType> lowercaseMap{};
      for (const auto& pair : stringToStreamTypeMapping) {
        const auto key = pair.first;
        const auto val = pair.second;
        lowercaseMap[Strings::toLowercase(key)] = val;
      }
      return lowercaseMap;
    }();

std::string toString(const StreamType &streamType);

Optional<StreamType>
streamTypeFromString(const std::string &streamTypeString);

////////////////////////////////////////////////////////////////////////////////

class Stream {
  const std::string name;
  const StreamType type;
  const uint64_t id;
  const std::string encoderName;
  const SerializationType serializationType;
  const std::string schemaName;

public:
  Stream(const std::string &name, const StreamType &type, uint64_t id,
         const std::string &encoderName, const std::string &schemaName)
      : name(name), type(type), id(id), encoderName(encoderName),
        serializationType(nat::core::getSerializationType(encoderName)),
        schemaName(schemaName) {}

  static Optional<Stream>
  createFromKafkaBrokerName(const std::string &brokerName);

  std::string getName() const { return name; }

  StreamType getType() const { return type; }

  uint64_t getId() const { return id; }

  std::string getEncoderName() const { return encoderName; }

  SerializationType getSerializationType() const { return serializationType; }

  std::string getSchemaName() const { return schemaName; }

  std::string toTopicString() const {
    return toString(type) + "-" + std::to_string(id) + "-" + encoderName + "-" + schemaName;
  }
};



class StreamMessage {
  const std::vector<uint8_t> message;
  const Stream stream;

public:
  StreamMessage(const std::vector<uint8_t>& message, const Stream& stream) : message(message), stream(stream) {}

  std::vector<uint8_t> getMessage() const { return message; }

  SerializationType getSerializationType() const { return stream.getSerializationType(); }     

  std::string getSchemaName() const { return stream.getSchemaName(); }
};


class Schema {
  public:
    virtual bool isSerializationTypeSupported(const SerializationType) const = 0;

    virtual std::unique_ptr<message_t> encodeToBytes(const SerializationType& type) const = 0;

    virtual std::string getName() const = 0;

    virtual std::string toString() const = 0;
};

using decoder_t = std::function<Optional<std::unique_ptr<Schema>>(const std::vector<uint8_t>& message, const SerializationType& type)>;

class Decoder {
  public:
    virtual ~Decoder() {}

    virtual bool isSerializationTypeSupported(const SerializationType) const = 0;

    virtual Optional<std::shared_ptr<Schema>> tryDecode(const std::vector<uint8_t> &message, const SerializationType &type) const = 0;
};

class Encoder {
  public:
    virtual ~Encoder() {}

    virtual bool isSerializationTypeSupported(const SerializationType) = 0;

    virtual std::vector<uint8_t> encode(const StreamMessage& message) = 0;
};




class Registry;

class BasicMetaInfoSchema : public Schema, public Decoder {
  std::string streamName;
public:
  static const std::string name;

  BasicMetaInfoSchema(const std::string &streamName) : streamName(streamName) {}

  virtual std::unique_ptr<std::vector<uint8_t>>
  encodeToBytes(const SerializationType &type) const override;

  virtual bool isSerializationTypeSupported(const SerializationType type) const override;

  virtual std::string toString() const override;

  static Optional<std::unique_ptr<BasicMetaInfoSchema>> decodeJson(const std::vector<uint8_t> &message);

  static Optional<std::unique_ptr<BasicMetaInfoSchema>> decodeAll(const std::vector<uint8_t> &message,
                                    const SerializationType &type);

  static void
  decodeAndDispatch(const std::vector<uint8_t> &message,
                    const SerializationType &type,
                    const std::function<void(const std::shared_ptr<Schema> &)>
                        &dispatchMethod);

  virtual Optional<std::shared_ptr<Schema>> tryDecode(const std::vector<uint8_t> &message,
                                    const SerializationType &type) const override;

  static void registerWithRegistry(Registry &registry);

  virtual std::string getName() const override;

  std::string getStreamName() const;

private:
  static Optional<std::shared_ptr<Schema>> sharedDecodeAll(const std::vector<uint8_t> &message,
                                    const SerializationType &type);
  static Optional<std::unique_ptr<Schema>> uniqueDecodeAll(const std::vector<uint8_t> &message,
                                    const SerializationType &type);

};

class NatImuBulkDataSchema;

class NatImuDataSchema: public Schema, public Decoder {
public:
    enum class SensorAccuracy {
        Unreliable = 0,
        LowAccuracy = 1,
        MediumAccuracy = 2,
        HighAccuracy = 3
    };

private:
  uint64_t time;
  float data[10];
  uint8_t accuracies; // 0bXX XX XX XX
                      //   ^   ^  ^  ^
                      //   |   |  |  L rotation_accuracy
                      //   |   |  L gryoscope_accuracy
                      //   |   L acceleration_accuracy
                      //   L unused
  uint8_t has_data;   // 0bXXXXX X X X
                      //       ^ ^ ^ ^
                      //       | | | L rotation_has_data
                      //       | | L gyroscope_has_data
                      //       | L acceleration_has_data
                      //       L unused

public:
  static const std::string name;
  static const uint32_t NatImuDataSchemaDataArraySize;

  NatImuDataSchema();

  NatImuDataSchema(const NatImuDataSchema& other);

  NatImuDataSchema(uint64_t time, NatImuDataSchema::SensorAccuracy acceleration_accuracy, NatImuDataSchema::SensorAccuracy gyroscope_accuracy, NatImuDataSchema::SensorAccuracy rotation_accuracy, bool acceleration_has_data, bool gryoscope_has_data, bool rotation_has_data, const float* data, int size);

  NatImuDataSchema(uint64_t time, uint8_t accuracies, uint8_t has_data, const float* data, int size);

#ifdef SERVER
  static Optional<std::shared_ptr<NatImuDataSchema>> tryCreateFromSchema(const Optional<const std::shared_ptr<Schema>>& messageMaybe);
#endif

  static SensorAccuracy convertIntToSensorAccuracy(int val);

  static int convertSensorAccuracyToInt(SensorAccuracy accuracy);

  static std::string toString(SensorAccuracy accuracy);

  virtual std::unique_ptr<std::vector<uint8_t>>
  encodeToBytes(const SerializationType &type) const override;

  virtual bool isSerializationTypeSupported(const SerializationType type) const override;

  virtual std::string toString() const override;

  static Optional<std::unique_ptr<NatImuDataSchema>> decodeJson(const std::vector<uint8_t> &message);

  static Optional<std::unique_ptr<NatImuDataSchema>> decodeCsv(const std::vector<uint8_t> &message);

  static Optional<std::unique_ptr<NatImuDataSchema>> decodeAll(const std::vector<uint8_t> &message,
                                    const SerializationType &type);

  static void
  decodeAndDispatch(const std::vector<uint8_t> &message,
                    const SerializationType &type,
                    const std::function<void(const std::shared_ptr<Schema> &)>
                        &dispatchMethod);

  virtual Optional<std::shared_ptr<Schema>> tryDecode(const std::vector<uint8_t> &message,
                                    const SerializationType &type) const override;

  static void registerWithRegistry(Registry &registry);

  virtual std::string getName() const override;

  double getTime() const;

  SensorAccuracy getAccelerationAccuracy() const;

  SensorAccuracy getGyroscopeAccuracy() const;

  SensorAccuracy getRotationAccuracy() const;

  bool wasDataSetForAcceleration() const;

  bool wasDataSetForGryoscope() const;

  bool wasDataSetForRotation() const;

  const float* getData() const;

private:
    friend class NatImuBulkDataSchema;

};

class NatImuBulkDataSchema : public Schema, public Decoder {
    NatImuDataSchema data[100];
    uint8_t size;

public:
    static const std::string name;

    NatImuBulkDataSchema();

    NatImuBulkDataSchema(const NatImuDataSchema* data, uint8_t size);

    bool isFull() const;

    void add(const NatImuDataSchema& datum);

    void setData(const NatImuDataSchema* data, int32_t size);

#ifdef SERVER
    static Optional<std::shared_ptr<NatImuBulkDataSchema>> tryCreateFromSchema(const Optional<const std::shared_ptr<Schema>>& messageMaybe);
#endif

    virtual std::unique_ptr<std::vector<uint8_t>>
        encodeToBytes(const SerializationType& type) const override;

    virtual bool isSerializationTypeSupported(const SerializationType type) const override;

    virtual std::string toString() const override;

    //static Optional<std::unique_ptr<NatImuBulkDataSchema>> decodeJson(const std::vector<uint8_t>& message);

    static Optional<std::unique_ptr<NatImuBulkDataSchema>> decodeCsv(const std::vector<uint8_t>& message);

    static Optional<std::unique_ptr<NatImuBulkDataSchema>> decodeBinary(const std::vector<uint8_t>& message);

    static Optional<std::unique_ptr<NatImuBulkDataSchema>> decodeAll(const std::vector<uint8_t>& message,
        const SerializationType& type);

    static void
        decodeAndDispatch(const std::vector<uint8_t>& message,
            const SerializationType& type,
            const std::function<void(const std::shared_ptr<Schema>&)>
            & dispatchMethod);

    virtual Optional<std::shared_ptr<Schema>> tryDecode(const std::vector<uint8_t>& message,
        const SerializationType& type) const override;

    static void registerWithRegistry(Registry& registry);

    virtual std::string getName() const override;

    std::unique_ptr<std::vector<NatImuDataSchema>> createImuRecords() const;

};

// Forward declaration
class NatMuseBulkDataSchema;

class NatMuseDataSchema : public Schema, public Decoder {
public:
    // Constants for Muse data sizes
    static const int EEG_SAMPLES_PER_PACKET = 12;   // 12 samples per EEG packet
    static const int MOTION_SAMPLES = 3;            // 3 samples per motion packet
    static const int PPG_SAMPLES = 6;               // 6 samples per PPG packet
    
    // has_data bitfield values
    static const uint8_t HAS_EEG = 0x01;
    static const uint8_t HAS_ACCEL = 0x02;
    static const uint8_t HAS_GYRO = 0x04;
    static const uint8_t HAS_PPG = 0x08;

//private:
    uint64_t time;              // NTP-synced timestamp (microseconds)
    uint16_t eeg_sequence;      // EEG packet sequence number
    uint16_t motion_sequence;   // Motion packet sequence number
    
    // EEG: 4 channels × 12 samples = 48 floats (microvolts)
    float tp9[EEG_SAMPLES_PER_PACKET];    // Left ear
    float af7[EEG_SAMPLES_PER_PACKET];    // Left forehead
    float af8[EEG_SAMPLES_PER_PACKET];    // Right forehead
    float tp10[EEG_SAMPLES_PER_PACKET];   // Right ear
    
    // Motion: 3 samples each
    float accel[MOTION_SAMPLES][3];       // Accelerometer (x,y,z) × 3 samples
    float gyro[MOTION_SAMPLES][3];        // Gyroscope (x,y,z) × 3 samples
    
    // PPG: 3 channels × 6 samples (Muse 2/S only)
    float ppg0[PPG_SAMPLES];
    float ppg1[PPG_SAMPLES];
    float ppg2[PPG_SAMPLES];
    
    uint8_t has_data;           // Bitfield: eeg(1), accel(2), gyro(4), ppg(8)

public:
    static const std::string name;

    NatMuseDataSchema();
    NatMuseDataSchema(const NatMuseDataSchema& other);
    NatMuseDataSchema(
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
        uint8_t has_data);

#ifdef SERVER
    static Optional<std::shared_ptr<NatMuseDataSchema>> tryCreateFromSchema(
        const Optional<const std::shared_ptr<Schema>>& messageMaybe);
#endif

    // Setters for building samples incrementally
    void setEegData(uint16_t sequence, 
                    const float tp9_data[EEG_SAMPLES_PER_PACKET],
                    const float af7_data[EEG_SAMPLES_PER_PACKET],
                    const float af8_data[EEG_SAMPLES_PER_PACKET],
                    const float tp10_data[EEG_SAMPLES_PER_PACKET]);
    void setAccelData(uint16_t sequence, const float data[MOTION_SAMPLES][3]);
    void setGyroData(uint16_t sequence, const float data[MOTION_SAMPLES][3]);
    void setPpgData(const float ppg0_data[PPG_SAMPLES],
                    const float ppg1_data[PPG_SAMPLES],
                    const float ppg2_data[PPG_SAMPLES]);
    void setTime(uint64_t t);

    // Schema interface
    virtual std::unique_ptr<std::vector<uint8_t>>
        encodeToBytes(const SerializationType& type) const override;
    virtual bool isSerializationTypeSupported(const SerializationType type) const override;
    virtual std::string toString() const override;
    virtual std::string getName() const override;

    // Decoder interface
    virtual Optional<std::shared_ptr<Schema>> tryDecode(
        const std::vector<uint8_t>& message, const SerializationType& type) const override;

    // Static decode methods
#ifdef SERVER
    static Optional<std::unique_ptr<NatMuseDataSchema>> decodeJson(const std::vector<uint8_t>& message);
#endif
    static Optional<std::unique_ptr<NatMuseDataSchema>> decodeCsv(const std::vector<uint8_t>& message);
    static Optional<std::unique_ptr<NatMuseDataSchema>> decodeAll(
        const std::vector<uint8_t>& message, const SerializationType& type);
    static void decodeAndDispatch(
        const std::vector<uint8_t>& message,
        const SerializationType& type,
        const std::function<void(const std::shared_ptr<Schema>&)>& dispatchMethod);

    static void registerWithRegistry(Registry& registry);

    // Getters
    uint64_t getTime() const;
    uint16_t getEegSequence() const;
    uint16_t getMotionSequence() const;
    
    const float* getTp9() const;
    const float* getAf7() const;
    const float* getAf8() const;
    const float* getTp10() const;
    
    const float (*getAccel() const)[3];
    const float (*getGyro() const)[3];
    
    const float* getPpg0() const;
    const float* getPpg1() const;
    const float* getPpg2() const;
    
    bool hasEegData() const;
    bool hasAccelData() const;
    bool hasGyroData() const;
    bool hasPpgData() const;

private:
    friend class NatMuseBulkDataSchema;
};

class NatMuseBulkDataSchema : public Schema, public Decoder {
public:
    static const size_t BULK_SIZE = 100;
    static const size_t SINGLE_SAMPLE_BINARY_SIZE = 349;
    static const size_t BINARY_BUFFER_SIZE = SINGLE_SAMPLE_BINARY_SIZE * BULK_SIZE; // 34900 bytes

private:
    NatMuseDataSchema data[BULK_SIZE];
    uint8_t size;

public:
    static const std::string name;

    NatMuseBulkDataSchema();
    NatMuseBulkDataSchema(const NatMuseDataSchema* samples, uint8_t count);

    bool isFull() const;
    void add(const NatMuseDataSchema& sample);
    void reset();
    uint8_t getSize() const;
    const NatMuseDataSchema* getData() const;

#ifdef SERVER
    static Optional<std::shared_ptr<NatMuseBulkDataSchema>> tryCreateFromSchema(
        const Optional<const std::shared_ptr<Schema>>& messageMaybe);
#endif

    // Schema interface
    virtual std::unique_ptr<std::vector<uint8_t>>
        encodeToBytes(const SerializationType& type) const override;
    virtual bool isSerializationTypeSupported(const SerializationType type) const override;
    virtual std::string toString() const override;
    virtual std::string getName() const override;

    // ESP32-friendly in-place encoding (avoids heap allocation)
    // Returns number of bytes written, or 0 on error
    // bufferLen must be >= BINARY_BUFFER_SIZE (34900 bytes)
    size_t encodeToBytesInPlace(uint8_t* buffer, size_t bufferLen) const;

    // Decoder interface
    virtual Optional<std::shared_ptr<Schema>> tryDecode(
        const std::vector<uint8_t>& message, const SerializationType& type) const override;

    // Static decode methods
    static Optional<std::unique_ptr<NatMuseBulkDataSchema>> decodeBinary(const std::vector<uint8_t>& message);
    static Optional<std::unique_ptr<NatMuseBulkDataSchema>> decodeCsv(const std::vector<uint8_t>& message);
    static Optional<std::unique_ptr<NatMuseBulkDataSchema>> decodeAll(
        const std::vector<uint8_t>& message, const SerializationType& type);
    static void decodeAndDispatch(
        const std::vector<uint8_t>& message,
        const SerializationType& type,
        const std::function<void(const std::shared_ptr<Schema>&)>& dispatchMethod);

    static void registerWithRegistry(Registry& registry);

    std::unique_ptr<std::vector<NatMuseDataSchema>> createMuseRecords() const;
};

struct BasicTopicInformation {
  const StreamType type;
  const SerializationType serializationType;
  const uint64_t id;
  const std::string schemaName;

  BasicTopicInformation() = delete;
  BasicTopicInformation(const BasicTopicInformation &other) = default;
  bool operator<(const BasicTopicInformation& other) const {
    if (type < other.type) return true;
    if (type > other.type) return false;
    if (serializationType < other.serializationType) return true;
    if (serializationType > other.serializationType) return false;
    if (id < other.id) return true;
    if (id > other.id) return false;
    if (schemaName < other.schemaName) return true;
    return false;
  }

  bool operator==(const BasicTopicInformation& other) const {
    return type == other.type && serializationType == other.serializationType && id == other.id && schemaName == other.schemaName;
  }

  static Optional<std::unique_ptr<BasicTopicInformation>>
  create(const std::string &kafkaTopicString);

  std::string toString() const;

  std::string toTopicString() const;

private:
  BasicTopicInformation(const StreamType &type,
                        const SerializationType &serializationType,
                        const uint64_t id, const std::string &schemaName)
      : type(type), serializationType(serializationType), id(id),
        schemaName(schemaName) {}
};

class JsonDecoder : public Decoder {
  public:
    virtual bool isSerializationTypeSupported(const SerializationType type) const override;

    // TODO: Change this return type
  virtual Optional<std::shared_ptr<Schema>> tryDecode(const std::vector<uint8_t> &message, const SerializationType &type) const override;
};

class JsonEncoder : public Encoder {
  public:
    JsonEncoder() = default;

    virtual bool isSerializationTypeSupported(const SerializationType type) override;

    virtual std::vector<uint8_t> encode(const StreamMessage& message) override;
 
};

class MessagingQueue {
public:
  virtual ~MessagingQueue() = default;

  virtual void enqueueMessageToSend(std::unique_ptr<message_t> &&message) = 0;

  virtual void enqueueMessageToReceive(const std::shared_ptr<message_t> message) = 0;

  virtual Optional<std::shared_ptr<message_t>> tryGetNextMessage() = 0;

  virtual void clearAllMessages() = 0;
};

class PlainTextMessage {
    const std::string plainTextMessage;

  public:
    PlainTextMessage(const std::string& message) : plainTextMessage(message) {}

    std::string getPlainTextMessage() const { return plainTextMessage; }
};

class RawStream {
    const uint64_t id;
    std::vector<std::unique_ptr<BasicTopicInformation>> topics;

  public:
	RawStream(const uint64_t id, std::vector<std::unique_ptr<BasicTopicInformation>>&& topics) : id(id), topics(std::move(topics)) {}
	
	RawStream(const uint64_t id, const std::vector<BasicTopicInformation>& topics) : id(id), topics(Vectors::wrapContainedValueWithUnique(topics)) {}
  public:
	RawStream() = delete;

	static Optional<std::unique_ptr<RawStream>> create(const std::vector<BasicTopicInformation>& topics);

	static Optional<std::unique_ptr<RawStream>> create(std::vector<std::unique_ptr<BasicTopicInformation>>&& topics);

	std::string toPrettyString() const;

	std::string toString() const;

	bool addTopic(const BasicTopicInformation& topic);

    std::vector<std::unique_ptr<BasicTopicInformation>> getTopicsByType(StreamType type) const;

	uint64_t getId() const;
};

class Registry {
    std::unordered_map<std::string, std::shared_ptr<Encoder>> encoders;
    std::unordered_map<std::string, decoder_t> decoders;
    std::unordered_map<std::string, std::vector<std::function<void(const std::shared_ptr<Schema>&)>>> schemaHandlers;

    static std::string createKey(const std::string& schemaName, const SerializationType& type);

    static std::string createKey(const StreamMessage& message);

    static std::string createKey(const BasicTopicInformation& topicInfo);

	public:
    Registry() = default;

    static std::unique_ptr<Registry> createDefaultInitalizeRegistry();

    void registerEncoder(const std::string& schemaName, const SerializationType& type, const std::shared_ptr<Encoder>& encoder);

    void registerDecoder(const std::string& schemaName, const SerializationType& type, const decoder_t& decoder);

    void registerSchemaHandler(const std::string& schemaName, const SerializationType& type, const std::function<void(const std::shared_ptr<Schema>&)>& dispatchFunction);
    
    Optional<std::unique_ptr<Schema>> tryDecode(const std::vector<uint8_t>& message, const BasicTopicInformation& topicInfo) const;

    void dispatchOnDecode(const std::vector<uint8_t>& message, const BasicTopicInformation& topicInfo);
};


class TopicTranslator {
  const std::shared_ptr<BasicTopicInformation> topicInfo;
  const std::shared_ptr<Registry> registry;

public:
  TopicTranslator(const std::shared_ptr<BasicTopicInformation> &topicInfo,
                  const std::shared_ptr<Registry> &registry)
      : topicInfo(topicInfo), registry(registry) {}

  Optional<std::unique_ptr<Schema>>
  tryDecodeMessage(const message_t &message);

  Optional<std::unique_ptr<message_t>>
  tryEncodeMessage(const Schema &schema) const;

  StreamType getStreamType() const;

  SerializationType getSerializationType() const;

  uint64_t getId() const;

  std::string getSchemaName() const;
};


class TopicMessenger {
  std::unique_ptr<MessagingQueue> messagingQueue;
  std::shared_ptr<TopicTranslator> translator;

  public:
  TopicMessenger(std::unique_ptr<MessagingQueue>&& messagingQueue, const std::shared_ptr<TopicTranslator> translator)
    : messagingQueue(std::move(messagingQueue)), translator(translator) {}
  
  void sendMessage(const Schema &schema);

  StreamType getStreamType() const;

  SerializationType getSerializationType() const;

  uint64_t getId() const;

  std::string getSchemaName() const;

  Optional<std::unique_ptr<Schema>> tryGetNexMessage();

  void clearAllMessages();
};




} // namespace core
} // namespace nat
