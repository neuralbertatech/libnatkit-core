#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
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
  // Value stored INLINE (in-object), like std::optional — NO heap allocation.
  // Previously wrapped std::unique_ptr<T>, which heap-allocated the value on
  // every copy (root cause of the natKit-IMU per-sample std::bad_alloc). This
  // requires a COMPLETE T at each instantiation (the old pointer form did not).
  alignas(T) unsigned char storage_[sizeof(T)];
  bool present_ = false;

  T* ptr_() { return reinterpret_cast<T*>(&storage_[0]); }
  const T* ptr_() const { return reinterpret_cast<const T*>(&storage_[0]); }

public:
  Optional() : present_(false) {}
  Optional(T val) : present_(true) { new (ptr_()) T(std::move(val)); }

  Optional(const Optional<T>& other) : present_(other.present_) {
    if (present_) new (ptr_()) T(*other.ptr_());
  }

  // Move leaves the source EMPTY, matching the prior unique_ptr-based move.
  Optional(Optional<T>&& other) : present_(other.present_) {
    if (present_) { new (ptr_()) T(std::move(*other.ptr_())); other.reset(); }
  }

  ~Optional() { reset(); }

  Optional<T>& operator=(const Optional<T>& other) {
    if (this != &other) {
      reset();
      if (other.present_) { new (ptr_()) T(*other.ptr_()); present_ = true; }
    }
    return *this;
  }

  Optional<T>& operator=(Optional<T>&& other) {
    if (this != &other) {
      reset();
      if (other.present_) {
        new (ptr_()) T(std::move(*other.ptr_()));
        present_ = true;
        other.reset();
      }
    }
    return *this;
  }

  void reset() { if (present_) { ptr_()->~T(); present_ = false; } }

  bool has_value() const { return present_; }
  T& value() const { return *const_cast<Optional<T>*>(this)->ptr_(); }
  void set(T val) { reset(); new (ptr_()) T(std::move(val)); present_ = true; }
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
  MARKER,

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
        {StreamType::MARKER, "Marker"},
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

struct TimelineInterval {
  int64_t start_time_us;
  int64_t end_time_us;
  int32_t value;
};

struct TimelinePoint {
  int64_t time_us;
  int32_t value;
};

std::vector<uint32_t> sortTimestampOrder(
    const std::vector<int64_t> &timestamps);

std::vector<int32_t> assignIntervalsToTimeline(
    const std::vector<int64_t> &timestamps,
    const std::vector<TimelineInterval> &intervals,
    int32_t default_value = -1);

std::vector<int32_t> assignPointsToTimeline(
    const std::vector<int64_t> &timestamps,
    const std::vector<TimelinePoint> &points,
    int32_t default_value = -1);

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

    // Timestamped: every message carries a microsecond timestamp on one uniform
    // axis so the time model (scrubbing, alignment, combine) can operate without
    // per-schema knowledge. Data frames return device_ts_us, markers return
    // emitted_at_us, meta records their created/updated time. The default (0)
    // means "no timestamp"; timestamped schemas override it.
    virtual uint64_t getTimestampUs() const { return 0; }
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
class MetaRecord;
using meta_record_decoder_t = std::function<
    Optional<std::unique_ptr<MetaRecord>>(
        const std::vector<uint8_t>& message,
        const SerializationType& type,
        uint16_t recordVersion)>;

class MetaRecord : public Schema, public Decoder {
public:
  static const std::string name;

  virtual uint32_t getRecordTypeId() const = 0;

  virtual uint16_t getRecordVersion() const = 0;

  static Optional<std::unique_ptr<MetaRecord>> decodeAll(
      const std::vector<uint8_t> &message,
      const SerializationType &type);

  virtual Optional<std::shared_ptr<Schema>> tryDecode(
      const std::vector<uint8_t> &message,
      const SerializationType &type) const override;

  static void registerWithRegistry(Registry &registry);

  static void registerMetaRecordType(
      uint32_t recordTypeId,
      const meta_record_decoder_t &decoder);

private:
  static Optional<std::shared_ptr<Schema>> sharedDecodeAll(
      const std::vector<uint8_t> &message,
      const SerializationType &type);
  static Optional<std::unique_ptr<Schema>> uniqueDecodeAll(
      const std::vector<uint8_t> &message,
      const SerializationType &type);
};

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

class SessionMetadataRecord : public MetaRecord {
  std::string sessionId;
  std::string purpose;
  std::string participantId;
  std::string protocolId;
  std::vector<std::string> deviceIds;
  std::vector<std::string> tags;
  std::string notes;
  uint64_t createdAtUs;
  uint64_t updatedAtUs;

public:
  static const std::string name;
  static const uint32_t recordTypeId;
  static const uint16_t recordVersion;

  SessionMetadataRecord(
      const std::string &sessionId,
      const std::string &purpose,
      const std::string &participantId,
      const std::string &protocolId,
      const std::vector<std::string> &deviceIds,
      const std::vector<std::string> &tags,
      const std::string &notes,
      uint64_t createdAtUs,
      uint64_t updatedAtUs)
      : sessionId(sessionId),
        purpose(purpose),
        participantId(participantId),
        protocolId(protocolId),
        deviceIds(deviceIds),
        tags(tags),
        notes(notes),
        createdAtUs(createdAtUs),
        updatedAtUs(updatedAtUs) {}

  virtual std::unique_ptr<std::vector<uint8_t>>
  encodeToBytes(const SerializationType &type) const override;

  virtual bool isSerializationTypeSupported(
      const SerializationType type) const override;

  virtual std::string getName() const override;

  virtual std::string toString() const override;

  virtual uint32_t getRecordTypeId() const override;

  virtual uint16_t getRecordVersion() const override;

  const std::string &getSessionId() const;
  const std::string &getPurpose() const;
  const std::string &getParticipantId() const;
  const std::string &getProtocolId() const;
  const std::vector<std::string> &getDeviceIds() const;
  const std::vector<std::string> &getTags() const;
  const std::string &getNotes() const;
  uint64_t getCreatedAtUs() const;
  uint64_t getUpdatedAtUs() const;

  static Optional<std::unique_ptr<SessionMetadataRecord>> decodePayloadJson(
      const std::vector<uint8_t> &message,
      uint16_t recordVersion);
  static Optional<std::unique_ptr<SessionMetadataRecord>> decodePayloadBinary(
      const std::vector<uint8_t> &message,
      uint16_t recordVersion);
  static Optional<std::unique_ptr<MetaRecord>> decodePayloadAll(
      const std::vector<uint8_t> &message,
      const SerializationType &type,
      uint16_t recordVersion);
  static void registerWithRegistry(Registry &registry);
};

class TransformProvenanceRecord : public MetaRecord {
  std::string outputIdentifier;
  uint64_t outputStreamId;
  std::string outputSchemaName;
  std::string outputTopic;
  uint64_t sourceStreamId;
  std::string sourceSchemaName;
  std::string sourceTopic;
  std::string transformKind;
  std::string inputMappingId;
  std::string configJson;
  uint64_t createdAtUs;

public:
  static const std::string name;
  static const uint32_t recordTypeId;
  static const uint16_t recordVersion;

  TransformProvenanceRecord(
      const std::string &outputIdentifier,
      uint64_t outputStreamId,
      const std::string &outputSchemaName,
      const std::string &outputTopic,
      uint64_t sourceStreamId,
      const std::string &sourceSchemaName,
      const std::string &sourceTopic,
      const std::string &transformKind,
      const std::string &inputMappingId,
      const std::string &configJson,
      uint64_t createdAtUs);

  virtual std::unique_ptr<std::vector<uint8_t>>
  encodeToBytes(const SerializationType &type) const override;

  virtual bool isSerializationTypeSupported(
      const SerializationType type) const override;

  virtual std::string getName() const override;

  virtual std::string toString() const override;

  virtual uint32_t getRecordTypeId() const override;

  virtual uint16_t getRecordVersion() const override;

  const std::string &getOutputIdentifier() const;
  uint64_t getOutputStreamId() const;
  const std::string &getOutputSchemaName() const;
  const std::string &getOutputTopic() const;
  uint64_t getSourceStreamId() const;
  const std::string &getSourceSchemaName() const;
  const std::string &getSourceTopic() const;
  const std::string &getTransformKind() const;
  const std::string &getInputMappingId() const;
  const std::string &getConfigJson() const;
  uint64_t getCreatedAtUs() const;

  static Optional<std::unique_ptr<TransformProvenanceRecord>> decodePayloadJson(
      const std::vector<uint8_t> &message,
      uint16_t recordVersion);
  static Optional<std::unique_ptr<TransformProvenanceRecord>>
  decodePayloadBinary(
      const std::vector<uint8_t> &message,
      uint16_t recordVersion);
  static Optional<std::unique_ptr<MetaRecord>> decodePayloadAll(
      const std::vector<uint8_t> &message,
      const SerializationType &type,
      uint16_t recordVersion);
  static void registerWithRegistry(Registry &registry);
};

class MarkerEventV1 : public Schema, public Decoder {
  std::string sessionId;
  std::string markerType;
  std::string markerId;
  std::string event;
  std::string label;
  uint64_t emittedAtUs;
  std::string attributesJson;

public:
  static const std::string name;
  static const std::string schemaVersion;

  MarkerEventV1(
      const std::string &sessionId,
      const std::string &markerType,
      const std::string &markerId,
      const std::string &event,
      const std::string &label,
      uint64_t emittedAtUs,
      const std::string &attributesJson = "{}")
      : sessionId(sessionId),
        markerType(markerType),
        markerId(markerId),
        event(event),
        label(label),
        emittedAtUs(emittedAtUs),
        attributesJson(attributesJson.empty() ? "{}" : attributesJson) {}

  virtual std::unique_ptr<std::vector<uint8_t>>
  encodeToBytes(const SerializationType &type) const override;

  virtual bool isSerializationTypeSupported(
      const SerializationType type) const override;

  virtual std::string getName() const override;

  virtual std::string toString() const override;

  const std::string &getSessionId() const;
  const std::string &getMarkerType() const;
  const std::string &getMarkerId() const;
  const std::string &getEvent() const;
  const std::string &getLabel() const;
  uint64_t getEmittedAtUs() const;
  const std::string &getAttributesJson() const;

  // Timestamped: a marker's time axis is emitted_at_us.
  uint64_t getTimestampUs() const override { return emittedAtUs; }

  static Optional<std::unique_ptr<MarkerEventV1>> decodeJson(
      const std::vector<uint8_t> &message);
  static Optional<std::unique_ptr<MarkerEventV1>> decodeBinary(
      const std::vector<uint8_t> &message);
  static Optional<std::unique_ptr<MarkerEventV1>> decodeAll(
      const std::vector<uint8_t> &message,
      const SerializationType &type);
  virtual Optional<std::shared_ptr<Schema>> tryDecode(
      const std::vector<uint8_t> &message,
      const SerializationType &type) const override;
  static void registerWithRegistry(Registry &registry);

private:
  static Optional<std::shared_ptr<Schema>> sharedDecodeAll(
      const std::vector<uint8_t> &message,
      const SerializationType &type);
  static Optional<std::unique_ptr<Schema>> uniqueDecodeAll(
      const std::vector<uint8_t> &message,
      const SerializationType &type);
};

class ExgPillEmgDataSchemaV1 : public Schema, public Decoder {
  std::string deviceId;
  uint64_t seqNo;
  uint64_t deviceTsUs;
  uint32_t sampleRateHz;
  std::vector<std::string> channelLabels;
  std::vector<int16_t> samples;
  uint32_t samplesPerChannel;

public:
  static const std::string name;
  static const std::string schemaVersion;

  ExgPillEmgDataSchemaV1();
  ExgPillEmgDataSchemaV1(
      const std::string &deviceId,
      uint64_t seqNo,
      uint64_t deviceTsUs,
      uint32_t sampleRateHz,
      const std::vector<std::string> &channelLabels,
      const std::vector<int16_t> &samples,
      uint32_t samplesPerChannel);

#ifdef SERVER
  static Optional<std::shared_ptr<ExgPillEmgDataSchemaV1>> tryCreateFromSchema(
      const Optional<const std::shared_ptr<Schema>> &messageMaybe);
#endif

  virtual std::unique_ptr<std::vector<uint8_t>>
  encodeToBytes(const SerializationType &type) const override;

  virtual bool isSerializationTypeSupported(
      const SerializationType type) const override;

  virtual std::string getName() const override;

  virtual std::string toString() const override;

  const std::string &getDeviceId() const;
  uint64_t getSeqNo() const;
  uint64_t getDeviceTsUs() const;
  uint32_t getSampleRateHz() const;
  uint32_t getChannelCount() const;
  uint32_t getSamplesPerChannel() const;
  const std::vector<std::string> &getChannelLabels() const;
  const std::vector<int16_t> &getSamples() const;

  // Timestamped: a data frame's time axis is device_ts_us.
  uint64_t getTimestampUs() const override { return deviceTsUs; }

  static Optional<std::unique_ptr<ExgPillEmgDataSchemaV1>> decodeJson(
      const std::vector<uint8_t> &message);
  static Optional<std::unique_ptr<ExgPillEmgDataSchemaV1>> decodeAll(
      const std::vector<uint8_t> &message,
      const SerializationType &type);
  virtual Optional<std::shared_ptr<Schema>> tryDecode(
      const std::vector<uint8_t> &message,
      const SerializationType &type) const override;
  static void registerWithRegistry(Registry &registry);

private:
  static Optional<std::shared_ptr<Schema>> sharedDecodeAll(
      const std::vector<uint8_t> &message,
      const SerializationType &type);
  static Optional<std::unique_ptr<Schema>> uniqueDecodeAll(
      const std::vector<uint8_t> &message,
      const SerializationType &type);
};

class ExgPillEmgTransformDataSchemaV1 : public Schema, public Decoder {
  std::string deviceId;
  uint64_t seqNo;
  uint64_t deviceTsUs;
  uint32_t sampleRateHz;
  std::vector<std::string> channelLabels;
  std::vector<float> samples;
  uint32_t samplesPerChannel;

public:
  static const std::string name;
  static const std::string schemaVersion;

  ExgPillEmgTransformDataSchemaV1();
  ExgPillEmgTransformDataSchemaV1(
      const std::string &deviceId,
      uint64_t seqNo,
      uint64_t deviceTsUs,
      uint32_t sampleRateHz,
      const std::vector<std::string> &channelLabels,
      const std::vector<float> &samples,
      uint32_t samplesPerChannel);

#ifdef SERVER
  static Optional<std::shared_ptr<ExgPillEmgTransformDataSchemaV1>>
  tryCreateFromSchema(
      const Optional<const std::shared_ptr<Schema>> &messageMaybe);
#endif

  virtual std::unique_ptr<std::vector<uint8_t>>
  encodeToBytes(const SerializationType &type) const override;

  virtual bool isSerializationTypeSupported(
      const SerializationType type) const override;

  virtual std::string getName() const override;

  virtual std::string toString() const override;

  const std::string &getDeviceId() const;
  uint64_t getSeqNo() const;
  uint64_t getDeviceTsUs() const;
  uint32_t getSampleRateHz() const;
  uint32_t getChannelCount() const;
  uint32_t getSamplesPerChannel() const;
  const std::vector<std::string> &getChannelLabels() const;
  const std::vector<float> &getSamples() const;

  // Timestamped: a transform-output frame's time axis is device_ts_us.
  uint64_t getTimestampUs() const override { return deviceTsUs; }

  static Optional<std::unique_ptr<ExgPillEmgTransformDataSchemaV1>> decodeJson(
      const std::vector<uint8_t> &message);
  static Optional<std::unique_ptr<ExgPillEmgTransformDataSchemaV1>> decodeAll(
      const std::vector<uint8_t> &message,
      const SerializationType &type);
  virtual Optional<std::shared_ptr<Schema>> tryDecode(
      const std::vector<uint8_t> &message,
      const SerializationType &type) const override;
  static void registerWithRegistry(Registry &registry);

private:
  static Optional<std::shared_ptr<Schema>> sharedDecodeAll(
      const std::vector<uint8_t> &message,
      const SerializationType &type);
  static Optional<std::unique_ptr<Schema>> uniqueDecodeAll(
      const std::vector<uint8_t> &message,
      const SerializationType &type);
};

class NatSignalFrameDataSchemaV1 : public Schema, public Decoder {
  std::string deviceId;
  uint64_t seqNo;
  uint64_t deviceTsUs;
  uint32_t sampleRateHz;
  std::vector<std::string> channelLabels;
  std::vector<float> samples;
  uint32_t samplesPerChannel;

public:
  static const std::string name;
  static const std::string schemaVersion;

  NatSignalFrameDataSchemaV1();
  NatSignalFrameDataSchemaV1(
      const std::string &deviceId,
      uint64_t seqNo,
      uint64_t deviceTsUs,
      uint32_t sampleRateHz,
      const std::vector<std::string> &channelLabels,
      const std::vector<float> &samples,
      uint32_t samplesPerChannel);

#ifdef SERVER
  static Optional<std::shared_ptr<NatSignalFrameDataSchemaV1>>
  tryCreateFromSchema(
      const Optional<const std::shared_ptr<Schema>> &messageMaybe);
#endif

  virtual std::unique_ptr<std::vector<uint8_t>>
  encodeToBytes(const SerializationType &type) const override;

  virtual bool isSerializationTypeSupported(
      const SerializationType type) const override;

  virtual std::string getName() const override;

  virtual std::string toString() const override;

  const std::string &getDeviceId() const;
  uint64_t getSeqNo() const;
  uint64_t getDeviceTsUs() const;
  uint32_t getSampleRateHz() const;
  uint32_t getChannelCount() const;
  uint32_t getSamplesPerChannel() const;
  const std::vector<std::string> &getChannelLabels() const;
  const std::vector<float> &getSamples() const;

  // Timestamped: a signal-frame's time axis is device_ts_us.
  uint64_t getTimestampUs() const override { return deviceTsUs; }

  static Optional<std::unique_ptr<NatSignalFrameDataSchemaV1>> decodeJson(
      const std::vector<uint8_t> &message);
  static Optional<std::unique_ptr<NatSignalFrameDataSchemaV1>> decodeAll(
      const std::vector<uint8_t> &message,
      const SerializationType &type);
  virtual Optional<std::shared_ptr<Schema>> tryDecode(
      const std::vector<uint8_t> &message,
      const SerializationType &type) const override;
  static void registerWithRegistry(Registry &registry);

private:
  static Optional<std::shared_ptr<Schema>> sharedDecodeAll(
      const std::vector<uint8_t> &message,
      const SerializationType &type);
  static Optional<std::unique_ptr<Schema>> uniqueDecodeAll(
      const std::vector<uint8_t> &message,
      const SerializationType &type);
};

enum class FieldValueType {
  Bool,
  Int16,
  Uint32,
  Uint64,
  Float32,
  Float64,
  String,
  Enum,
  Object,
  Array,
};

class SchemaFieldDescriptor {
  std::string fieldId;
  std::string label;
  std::string description;
  std::string unit;
  bool optional;
  FieldValueType valueType;
  std::vector<std::string> enumValues;
  std::vector<SchemaFieldDescriptor> childFields;
  std::shared_ptr<SchemaFieldDescriptor> arrayItemField;

public:
  SchemaFieldDescriptor();
  SchemaFieldDescriptor(
      const std::string &fieldId,
      const std::string &label,
      FieldValueType valueType,
      const std::string &description = std::string{},
      const std::string &unit = std::string{},
      bool optional = false,
      const std::vector<std::string> &enumValues = std::vector<std::string>{},
      const std::vector<SchemaFieldDescriptor> &childFields =
          std::vector<SchemaFieldDescriptor>{},
      const std::shared_ptr<SchemaFieldDescriptor> &arrayItemField =
          std::shared_ptr<SchemaFieldDescriptor>());

  const std::string &getFieldId() const;
  const std::string &getLabel() const;
  const std::string &getDescription() const;
  const std::string &getUnit() const;
  bool isOptional() const;
  FieldValueType getValueType() const;
  const std::vector<std::string> &getEnumValues() const;
  const std::vector<SchemaFieldDescriptor> &getChildFields() const;
  const std::shared_ptr<SchemaFieldDescriptor> &getArrayItemField() const;

  const SchemaFieldDescriptor *findChildField(
      const std::string &childFieldId) const;
};

class SchemaPath {
public:
  struct Segment {
    bool isArrayIndex;
    std::string fieldId;
    uint32_t arrayIndex;

    Segment();
    explicit Segment(const std::string &fieldId);
    explicit Segment(uint32_t arrayIndex);
  };

private:
  std::vector<Segment> segments;

public:
  SchemaPath();
  explicit SchemaPath(const std::vector<Segment> &segments);

  const std::vector<Segment> &getSegments() const;

  static Optional<SchemaPath> parse(const std::string &path);
};

class FieldValueRef {
  union ScalarStorage {
    bool boolValue;
    int16_t int16Value;
    uint32_t uint32Value;
    uint64_t uint64Value;
    float float32Value;
    double float64Value;

    ScalarStorage() : uint64Value(0) {}
  };

  FieldValueType valueType;
  const void *valuePtr;
  size_t elementCount;
  ScalarStorage scalarStorage;

public:
  FieldValueRef();
  FieldValueRef(FieldValueType valueType, const void *valuePtr, size_t elementCount = 0);

  static FieldValueRef fromBool(const bool &value);
  static FieldValueRef fromInt16(const int16_t &value);
  static FieldValueRef fromUint32(const uint32_t &value);
  static FieldValueRef fromUint64(const uint64_t &value);
  static FieldValueRef fromFloat32(const float &value);
  static FieldValueRef fromFloat64(const double &value);
  static FieldValueRef fromString(const std::string &value);
  static FieldValueRef fromArray(size_t elementCount);
  static FieldValueRef fromObject();

  FieldValueType getValueType() const;
  size_t getElementCount() const;

  Optional<bool> getBool() const;
  Optional<int16_t> getInt16() const;
  Optional<uint32_t> getUint32() const;
  Optional<uint64_t> getUint64() const;
  Optional<float> getFloat32() const;
  Optional<double> getFloat64() const;
  Optional<std::string> getString() const;
};

class DataSchemaDescriptor : public MetaRecord {
public:
  virtual std::string getTargetSchemaName() const = 0;
  virtual uint16_t getDescriptorVersion() const = 0;
  virtual const SchemaFieldDescriptor &getRootField() const = 0;
  virtual Optional<FieldValueRef> tryGetFieldValue(
      const Schema &record,
      const std::string &path) const = 0;

  virtual std::unique_ptr<std::vector<uint8_t>>
  encodeToBytes(const SerializationType &type) const override;
  virtual bool isSerializationTypeSupported(
      const SerializationType type) const override;
  virtual std::string getName() const override;
  virtual std::string toString() const override;
  virtual uint32_t getRecordTypeId() const override;
  virtual uint16_t getRecordVersion() const override;

  Optional<std::string> getString(
      const Schema &record,
      const std::string &path) const;
  Optional<int16_t> getInt16(
      const Schema &record,
      const std::string &path) const;
  Optional<uint32_t> getUint32(
      const Schema &record,
      const std::string &path) const;
  Optional<uint64_t> getUint64(
      const Schema &record,
      const std::string &path) const;
};

class DataSchemaDescriptorRegistry {
  std::unordered_map<std::string, std::shared_ptr<const DataSchemaDescriptor>>
      descriptorsBySchemaName;

public:
  void registerDescriptor(
      const std::shared_ptr<const DataSchemaDescriptor> &descriptor);
  Optional<std::shared_ptr<const DataSchemaDescriptor>> findBySchemaName(
      const std::string &schemaName) const;

  static DataSchemaDescriptorRegistry &getDefault();
};

class ExgPillEmgDataSchemaV1Descriptor : public DataSchemaDescriptor {
public:
  static const std::string name;
  static const uint16_t descriptorVersion;

  virtual std::string getTargetSchemaName() const override;
  virtual uint16_t getDescriptorVersion() const override;
  virtual const SchemaFieldDescriptor &getRootField() const override;
  virtual Optional<FieldValueRef> tryGetFieldValue(
      const Schema &record,
      const std::string &path) const override;

  static void registerWithRegistry(DataSchemaDescriptorRegistry &registry);
};

class ExgPillEmgTransformDataSchemaV1Descriptor : public DataSchemaDescriptor {
public:
  static const std::string name;
  static const uint16_t descriptorVersion;

  virtual std::string getTargetSchemaName() const override;
  virtual uint16_t getDescriptorVersion() const override;
  virtual const SchemaFieldDescriptor &getRootField() const override;
  virtual Optional<FieldValueRef> tryGetFieldValue(
      const Schema &record,
      const std::string &path) const override;

  static void registerWithRegistry(DataSchemaDescriptorRegistry &registry);
};

class NatSignalFrameDataSchemaV1Descriptor : public DataSchemaDescriptor {
public:
  static const std::string name;
  static const uint16_t descriptorVersion;

  virtual std::string getTargetSchemaName() const override;
  virtual uint16_t getDescriptorVersion() const override;
  virtual const SchemaFieldDescriptor &getRootField() const override;
  virtual Optional<FieldValueRef> tryGetFieldValue(
      const Schema &record,
      const std::string &path) const override;

  static void registerWithRegistry(DataSchemaDescriptorRegistry &registry);
};

class NatImuDataSchemaDescriptor : public DataSchemaDescriptor {
public:
  static const std::string name;
  static const uint16_t descriptorVersion;

  virtual std::string getTargetSchemaName() const override;
  virtual uint16_t getDescriptorVersion() const override;
  virtual const SchemaFieldDescriptor &getRootField() const override;
  virtual Optional<FieldValueRef> tryGetFieldValue(
      const Schema &record,
      const std::string &path) const override;

  static void registerWithRegistry(DataSchemaDescriptorRegistry &registry);
};

// --- Device health: NatKitNodeStatusV1 (TEC-NATKIT-33) --------------------
//
// The per-leaf health the primary publishes on `Log-<id>-Binary-NatKitNodeStatusV1`
// at ~1 Hz: what a leaf built, sent and failed to send, its sequence gaps, its
// RSSI, and the clock fit the primary holds for it.
//
// ⚠️ THIS EXISTS SO THE LAYOUT STOPS LIVING IN THREE PLACES. Before it, the only
// definitions were the firmware's `UplinkNodeStatus` struct and a hardcoded
// `struct.unpack` in ~/natkit-verification (not in git), which has already failed
// silently in the predictable way: a sibling script pinned NODE_SIZE = 168 and
// skipped every frame of any other size, so a struct that grew a field produced
// empty windows that read as a dead rig.
//
// ⚠️ Decoded FIELD BY FIELD in explicit little-endian, never by memcpy onto a
// struct. The wire bytes are a memcpy of the firmware's struct on an xtensa
// build; reproducing that by declaring a matching struct here would make the
// decode depend on this compiler agreeing about padding, which is the failure
// nobody notices until a different target is used. The explicit offsets are
// asserted against a captured live frame in the tests.
class NatKitNodeStatusV1Schema : public Schema, public Decoder {
public:
  static const std::string name;
  // The published payload is exactly this size; anything else is refused rather
  // than decoded partially.
  static const size_t kWireSize;

  uint64_t deviceId = 0;
  uint8_t mac[6] = {};
  // ⚠️ The formatted MAC is STORED, not formatted on demand, because
  // FieldValueRef is a reference type: it keeps a `const void*` to the caller's
  // string. Returning fromString() on a temporary compiles, resolves, and hands
  // back an empty value from a dangling pointer -- which is what it did until a
  // test printed it.
  std::string macText;
  int8_t leafNoiseFloorDbm = 0;

  uint32_t dataFrames = 0;
  uint32_t seqGaps = 0;
  uint32_t seqDuplicates = 0;
  uint32_t seqRestarts = 0;
  uint32_t heartbeats = 0;
  uint64_t lastSeenUs = 0;      // in the PRIMARY's clock

  // The leaf's clock fit, as the primary holds it.
  uint64_t syncDeviceId = 0;
  uint32_t syncEpoch = 0;
  uint64_t syncRefLocalUs = 0;
  int64_t syncRefOffsetUs = 0;
  int32_t syncSkewPpb = 0;
  uint32_t syncResidualRmsNs = 0;
  uint32_t syncPeakResidualNs = 0;
  uint64_t syncLastBeaconLocalUs = 0;
  uint32_t beaconsSeen = 0;
  uint32_t beaconsMissed = 0;
  uint32_t pairsUsed = 0;
  uint32_t pairsOrphaned = 0;
  uint32_t outliersRejected = 0;
  uint32_t epochChanges = 0;
  uint32_t macSpreadUs = 0;
  uint16_t samplesUsed = 0;
  uint8_t quality = 0;
  uint8_t implausibleResiduals = 0;

  uint8_t syncValid = 0;
  int8_t rssiLast = 0;
  int8_t rssiBest = 0;
  int8_t rssiWorst = 0;
  uint8_t rssiSeen = 0;
  uint8_t leafScanChannel = 0;
  int8_t leafRssiOfPrimary = 0;
  uint8_t leafTxPowerQuarterDbm = 0;

  uint32_t leafFramesBuilt = 0;
  uint32_t leafFramesDropped = 0;
  uint32_t leafSendFailures = 0;
  uint32_t leafChannelHops = 0;
  uint32_t publishNoSync = 0;
  uint32_t publishNoShift = 0;

  NatKitNodeStatusV1Schema() = default;

  bool isSerializationTypeSupported(const SerializationType) const override;
  std::unique_ptr<message_t> encodeToBytes(const SerializationType& type) const override;
  std::string getName() const override;
  std::string toString() const override;
  uint64_t getTimestampUs() const override;

  Optional<std::shared_ptr<Schema>> tryDecode(
      const std::vector<uint8_t>& message,
      const SerializationType& type) const override;

  // Round-trippable on purpose: an encoder that cannot reproduce the bytes it
  // decoded is not a shared definition, it is a second guess.
  // Registered so the GENERIC decode path can read these: a log viewer resolves
  // a topic through the Registry rather than knowing schema names, so a schema
  // that is only decodable by calling decodeBinary() directly is invisible to it.
  static void registerWithRegistry(Registry &registry);

  static Optional<NatKitNodeStatusV1Schema> decodeBinary(
      const std::vector<uint8_t>& message);
  std::vector<uint8_t> encodeBinary() const;
  std::string toJson() const;
};

class NatKitNodeStatusV1Descriptor : public DataSchemaDescriptor {
public:
  static const std::string name;
  static const uint16_t descriptorVersion;

  virtual std::string getTargetSchemaName() const override;
  virtual uint16_t getDescriptorVersion() const override;
  virtual const SchemaFieldDescriptor &getRootField() const override;
  virtual Optional<FieldValueRef> tryGetFieldValue(
      const Schema &record,
      const std::string &path) const override;

  static void registerWithRegistry(DataSchemaDescriptorRegistry &registry);
};

// --- Device health: NatKitPrimaryStatusV1 (TEC-NATKIT-33) -----------------
//
// The hub's own health, published on `Log-<id>-Binary-NatKitPrimaryStatusV1` at
// ~1 Hz: uplink counters, the rig-wide time-coherence metric, the hub's noise
// floor and die temperature, and the command-relay counters.
//
// Same reasoning as NatKitNodeStatusV1: decoded field by field in explicit
// little-endian, offsets pinned by a test against a captured live frame.
class NatKitPrimaryStatusV1Schema : public Schema, public Decoder {
public:
  static const std::string name;
  static const size_t kWireSize;

  uint64_t deviceId = 0;
  uint64_t uptimeUs = 0;
  uint32_t epoch = 0;
  uint32_t freeHeap = 0;
  uint32_t minFreeHeap = 0;
  uint32_t nodesKnown = 0;
  uint32_t nodesRejected = 0;
  uint32_t unknownPackets = 0;
  uint32_t framesQueued = 0;
  uint32_t framesSent = 0;
  uint32_t framesDropped = 0;
  uint32_t writeTimeouts = 0;
  uint64_t bytesSent = 0;
  uint32_t coherenceTypicalUs = 0;
  uint32_t coherenceBoundUs = 0;
  uint32_t coherenceWorstUs = 0;
  uint32_t coherenceSamples = 0;
  uint8_t coherenceQuality = 0;
  uint8_t coherenceMeasured = 0;
  uint8_t registrySealed = 0;
  int8_t noiseFloorDbm = 0;
  int8_t chipTempC = 0;
  uint8_t chipTempErr = 0;
  uint32_t commandsReceived = 0;
  uint32_t commandsRelayed = 0;
  uint32_t commandsMalformed = 0;
  uint32_t commandsUnknownDevice = 0;
  uint32_t commandsSendFailed = 0;
  uint32_t commandSubscriptions = 0;
  uint32_t commandAnswersReceived = 0;
  uint32_t commandAnswersPublished = 0;
  uint32_t commandAnswersDuplicate = 0;
  uint32_t commandsDelivered = 0;
  uint32_t commandRetransmits = 0;
  uint32_t commandsUndelivered = 0;
  uint32_t resetReason = 0;

  NatKitPrimaryStatusV1Schema() = default;

  bool isSerializationTypeSupported(const SerializationType) const override;
  std::unique_ptr<message_t> encodeToBytes(const SerializationType& type) const override;
  std::string getName() const override;
  std::string toString() const override;
  uint64_t getTimestampUs() const override;

  Optional<std::shared_ptr<Schema>> tryDecode(
      const std::vector<uint8_t>& message,
      const SerializationType& type) const override;

  // Registered so the GENERIC decode path can read these: a log viewer resolves
  // a topic through the Registry rather than knowing schema names, so a schema
  // that is only decodable by calling decodeBinary() directly is invisible to it.
  static void registerWithRegistry(Registry &registry);

  static Optional<NatKitPrimaryStatusV1Schema> decodeBinary(
      const std::vector<uint8_t>& message);
  std::vector<uint8_t> encodeBinary() const;
  std::string toJson() const;
};

class NatKitPrimaryStatusV1Descriptor : public DataSchemaDescriptor {
public:
  static const std::string name;
  static const uint16_t descriptorVersion;

  virtual std::string getTargetSchemaName() const override;
  virtual uint16_t getDescriptorVersion() const override;
  virtual const SchemaFieldDescriptor &getRootField() const override;
  virtual Optional<FieldValueRef> tryGetFieldValue(
      const Schema &record,
      const std::string &path) const override;

  static void registerWithRegistry(DataSchemaDescriptorRegistry &registry);
};

class NatMuseDataSchemaDescriptor : public DataSchemaDescriptor {
public:
  static const std::string name;
  static const uint16_t descriptorVersion;

  virtual std::string getTargetSchemaName() const override;
  virtual uint16_t getDescriptorVersion() const override;
  virtual const SchemaFieldDescriptor &getRootField() const override;
  virtual Optional<FieldValueRef> tryGetFieldValue(
      const Schema &record,
      const std::string &path) const override;

  static void registerWithRegistry(DataSchemaDescriptorRegistry &registry);
};

// Channel-major view of a NatImuBulkDataSchema frame: exposes the frame envelope
// (seq_no/device_ts_us/sample_rate_hz) plus per-axis Float32 sample arrays
// (accel_x..gyro_z) projected across the bulk's samples. This is what lets the
// (sample-major) IMU stream satisfy the transform pipeline's channel-frame input
// contract without changing the wire format.
class NatImuBulkDataSchemaDescriptor : public DataSchemaDescriptor {
public:
  static const std::string name;
  static const uint16_t descriptorVersion;

  virtual std::string getTargetSchemaName() const override;
  virtual uint16_t getDescriptorVersion() const override;
  virtual const SchemaFieldDescriptor &getRootField() const override;
  virtual Optional<FieldValueRef> tryGetFieldValue(
      const Schema &record,
      const std::string &path) const override;

  static void registerWithRegistry(DataSchemaDescriptorRegistry &registry);
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
  // 13 positional floats, NOT named:
  //   0-2  accel x/y/z (m/s^2)      6-9   quat real/i/j/k
  //   3-5  gyro  x/y/z (rad/s)      10-12 mag   x/y/z (uT)
  //
  // ⚠️ THE MAGNETOMETER (10-12) IS ONLY ON THE WIRE IN FRAME VERSION 2. A v1
  // frame carries ten floats; decoding one leaves 10-12 zeroed and the
  // magnetometer_has_data bit clear, which is what distinguishes "no
  // magnetometer in this recording" from "a magnetometer reading of zero".
  // ALWAYS CHECK has_data -- a zero field is not the same as an absent one, and
  // every recording made before 2026-08 is v1.
  float data[13];
  uint8_t accuracies; // 0bXX XX XX XX
                      //   ^  ^  ^  ^
                      //   |  |  |  L rotation_accuracy
                      //   |  |  L gryoscope_accuracy
                      //   |  L acceleration_accuracy
                      //   L magnetometer_accuracy (was unused; v2)
  uint8_t has_data;   // 0bXXXX X X X X
                      //      ^ ^ ^ ^
                      //      | | | L rotation_has_data
                      //      | | L gyroscope_has_data
                      //      | L acceleration_has_data
                      //      L magnetometer_has_data (was unused; v2)

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

  SensorAccuracy getMagnetometerAccuracy() const;

  bool wasDataSetForAcceleration() const;

  bool wasDataSetForGryoscope() const;

  bool wasDataSetForRotation() const;

  // False for every frame written before version 2, which had no magnetometer
  // field. Check this before using data[10..12].
  bool wasDataSetForMagnetometer() const;

  const float* getData() const;

private:
    friend class NatImuBulkDataSchema;

};

class NatImuBulkDataSchema : public Schema, public Decoder {
    NatImuDataSchema data[100];
    uint8_t size;

    // Frame envelope (see kFrameHeaderSize). deviceTsUs is the timestamp of the
    // first sample in microseconds; seqNo is a monotonic per-device frame
    // counter; sampleRateHz is the per-frame sampling rate.
    uint16_t schemaVersion;
    uint32_t sampleRateHz;
    uint64_t seqNo;
    uint64_t deviceTsUs;

public:
    static const std::string name;

    // Binary wire format: a fixed 24-byte little-endian header followed by
    // `sampleCount` fixed-size samples.
    //   uint16 schemaVersion | uint16 sampleCount | uint32 sampleRateHz
    //   uint64 seqNo         | uint64 deviceTsUs
    static const uint16_t kFrameSchemaVersion;
    static const size_t kFrameHeaderSize;

    // has_data masks per frame version. A v1 frame cannot say anything about the
    // magnetometer, so its bit is cleared rather than trusted -- a v1 writer left
    // that bit unused, and "unused" is not the same as "false" once something
    // starts reading it.
    static const uint8_t kHasDataMaskV1;
    static const uint8_t kHasDataMaskV2;

    // Same argument for the accuracy byte: v1 writers left bits 7-6 unused and
    // some set them, so they are cleared rather than read as a magnetometer
    // accuracy. Masking has_data alone would leave a v1 frame reporting "no
    // magnetometer, accuracy high", which is the kind of half-true that gets
    // quoted.
    static const uint8_t kAccuraciesMaskV1;
    static const uint8_t kAccuraciesMaskV2;

    // How a frame of the given version is laid out on the wire. ⚠️ ADDING A
    // VERSION MEANS RE-CHECKING THE LEGACY SENTINEL in decodeBinary: the
    // headerless format is recognised purely by being exactly 5000 bytes, which
    // is only unambiguous while 24 + n*sampleSize == 5000 has no integer
    // solution for every supported sampleSize.
    static size_t binaryFloatsPerSample(uint16_t frameVersion);
    static size_t binarySampleSize(uint16_t frameVersion);

    NatImuBulkDataSchema();

    NatImuBulkDataSchema(const NatImuDataSchema* data, uint8_t size);

    bool isFull() const;

    void add(const NatImuDataSchema& datum);

    void setData(const NatImuDataSchema* data, int32_t size);

    // Populate the frame envelope prior to encoding. schemaVersion is left at
    // kFrameSchemaVersion.
    void setFrameHeader(uint64_t seqNo, uint64_t deviceTsUs, uint32_t sampleRateHz);

    uint16_t getSchemaVersion() const;
    uint64_t getSeqNo() const;
    uint64_t getDeviceTsUs() const;
    uint32_t getSampleRateHz() const;
    uint8_t getSampleCount() const;

    // Direct (non-copying) view of the first `getSampleCount()` samples. Used by
    // NatImuBulkDataSchemaDescriptor to project per-axis sample arrays without the
    // per-access allocation `createImuRecords()` would incur.
    const NatImuDataSchema* getSamples() const;

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

// Stable FNV-1a stream id for a (namespace, identifier) pair, masked to 63 bits
// with a 0 result remapped to 1. Single source of truth for both the C++
// backend and, via nat_core_v1_stream_id, every non-C++ binding.
uint64_t stableStreamId(const std::string &topic_namespace,
                        const std::string &identifier);

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

  // Block until every message enqueued for sending has actually been handed to
  // the transport. Default is a no-op for queues that send synchronously; the
  // Kafka queue overrides it to drain its async send queue. Callers that
  // produce-then-exit (one-shot publishers) must call this before destroying
  // the messenger or in-flight messages can be lost.
  virtual void flush() {}
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

  void sendRawMessage(std::unique_ptr<message_t> &&message);

  void sendRawMessage(const message_t &message);

  StreamType getStreamType() const;

  SerializationType getSerializationType() const;

  uint64_t getId() const;

  std::string getSchemaName() const;

  Optional<std::shared_ptr<message_t>> tryGetNextRawMessage();

  Optional<std::unique_ptr<Schema>> tryGetNexMessage();

  void clearAllMessages();

  void flush();
};




} // namespace core
} // namespace nat

// The stable C ABI lives in its own pure-C header so FFI generators can parse
// it; the declarations are shared by every non-C++ binding. See that file for
// the ABI conventions.
#include "libnatkit-core-abi.h"
