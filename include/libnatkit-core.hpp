#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>


namespace nat::core {

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
    std::transform(lowercaseString.begin(), lowercaseString.end(), lowercaseString.begin(), [](const auto& character) { return std::tolower(character); });
    return lowercaseString;
}

}

namespace Vectors {

template <typename T>
inline std::vector<std::unique_ptr<T>> wrapContainedValueWithUnique(const std::vector<T>& vec) {
  std::vector<std::unique_ptr<T>> wrappedVec{};
  for (const auto& val : vec) {
	  wrappedVec.emplace_back(std::make_unique<T>(val));
  }

  return wrappedVec;
}

}


using message_t = std::vector<uint8_t>;

class StreamInfo {
  std::string streamName;

  public:
  StreamInfo(const std::string& streamName) : streamName(streamName) {}
};



// Enums ///////////////////////////////////////////////////////////////////////
enum class SerializationType {
  Json
};

static const std::unordered_map<SerializationType, std::string>
    serializationTypeToStringMapping = {
        {SerializationType::Json, "Json"},
};

static const std::unordered_map<std::string, SerializationType>
    stringToSerializationTypeMapping = []() {
      std::unordered_map<std::string, SerializationType> newMap{};
      std::transform(
          serializationTypeToStringMapping.begin(), serializationTypeToStringMapping.end(),
          std::inserter(newMap, newMap.end()),
          [](const auto &pair) -> std::pair<std::string, SerializationType> {
            return {Strings::toLowercase(pair.second), pair.first};
          });
      return newMap;
    }();

static const std::unordered_map<std::string, SerializationType>
    lowercaseStringToSerializationTypeMapping = []() {
      std::unordered_map<std::string, SerializationType> lowercaseMap{};
      for (const auto& [key, val] : stringToSerializationTypeMapping) {
        lowercaseMap[Strings::toLowercase(key)] = val;
      }
      return lowercaseMap;
    }();

SerializationType getSerializationType(const std::string& encoderName);

std::string toString(const SerializationType& streamType);

std::optional<SerializationType>
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
          [](const auto &pair) -> std::pair<std::string, StreamType> {
            return {Strings::toLowercase(pair.second), pair.first};
          });
      return newMap;
    }();

static const std::unordered_map<std::string, StreamType>
    lowercaseStringToStreamTypeMapping = []() {
      std::unordered_map<std::string, StreamType> lowercaseMap{};
      for (const auto& [key, val] : stringToStreamTypeMapping) {
        lowercaseMap[Strings::toLowercase(key)] = val;
      }
      return lowercaseMap;
    }();

std::string toString(const StreamType &streamType);

std::optional<StreamType>
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

  static std::optional<Stream>
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

using decoder_t = std::function<std::optional<std::unique_ptr<Schema>>(const std::vector<uint8_t>& message, const SerializationType& type)>;

class Decoder {
  public:
    virtual ~Decoder() {}

    virtual bool isSerializationTypeSupported(const SerializationType) const = 0;

    virtual std::optional<std::shared_ptr<Schema>> tryDecode(const std::vector<uint8_t> &message, const SerializationType &type) const = 0;
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
  inline static const std::string name = "BasicMetaInfoSchema";

  BasicMetaInfoSchema(const std::string &streamName) : streamName(streamName) {}

  virtual std::unique_ptr<std::vector<uint8_t>>
  encodeToBytes(const SerializationType &type) const override;

  virtual bool isSerializationTypeSupported(const SerializationType type) const override;

  virtual std::string toString() const override;

  static std::optional<std::unique_ptr<BasicMetaInfoSchema>> decodeJson(const std::vector<uint8_t> &message);

  static std::optional<std::unique_ptr<BasicMetaInfoSchema>> decodeAll(const std::vector<uint8_t> &message,
                                    const SerializationType &type);

  static void
  decodeAndDispatch(const std::vector<uint8_t> &message,
                    const SerializationType &type,
                    const std::function<void(const std::shared_ptr<Schema> &)>
                        &dispatchMethod);

  virtual std::optional<std::shared_ptr<Schema>> tryDecode(const std::vector<uint8_t> &message,
                                    const SerializationType &type) const override;

  static void registerWithRegistry(Registry &registry);

  virtual std::string getName() const override;

  std::string getStreamName() const;

};

class NatImuDataSchema: public Schema, public Decoder {
  uint64_t time;
  float data[9];

public:
  inline static const std::string name = "NatImuDataSchema";

  NatImuDataSchema(uint64_t time, float* data, int size) : time(time) {
    assert(size <= 9);
    for (int i = 0; i < 9; ++i)
      if (i < size)
        this->data[i] = data[i];
      else
        this->data[i] = 0;
  }

  virtual std::unique_ptr<std::vector<uint8_t>>
  encodeToBytes(const SerializationType &type) const override;

  virtual bool isSerializationTypeSupported(const SerializationType type) const override;

  virtual std::string toString() const override;

  static std::optional<std::unique_ptr<BasicMetaInfoSchema>> decodeJson(const std::vector<uint8_t> &message);

  static std::optional<std::unique_ptr<BasicMetaInfoSchema>> decodeAll(const std::vector<uint8_t> &message,
                                    const SerializationType &type);

  static void
  decodeAndDispatch(const std::vector<uint8_t> &message,
                    const SerializationType &type,
                    const std::function<void(const std::shared_ptr<Schema> &)>
                        &dispatchMethod);

  virtual std::optional<std::shared_ptr<Schema>> tryDecode(const std::vector<uint8_t> &message,
                                    const SerializationType &type) const override;

  static void registerWithRegistry(Registry &registry);

  virtual std::string getName() const override;

  std::string getStreamName() const;

};

struct BasicTopicInformation {
  const StreamType type;
  const SerializationType serializationType;
  const uint64_t id;
  const std::string schemaName;

  BasicTopicInformation() = delete;
  BasicTopicInformation(const BasicTopicInformation &other) = default;
  auto operator<=>(const BasicTopicInformation &) const = default;

  static std::optional<std::unique_ptr<BasicTopicInformation>>
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
  virtual std::optional<std::shared_ptr<Schema>> tryDecode(const std::vector<uint8_t> &message, const SerializationType &type) const override;
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

  virtual std::optional<std::shared_ptr<message_t>> tryGetNextMessage() = 0;
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

	static std::optional<std::unique_ptr<RawStream>> create(const std::vector<BasicTopicInformation>& topics);

	static std::optional<std::unique_ptr<RawStream>> create(std::vector<std::unique_ptr<BasicTopicInformation>>&& topics);

	std::string toPrettyString() const;

	std::string toString() const;

	bool addTopic(const BasicTopicInformation& topic);

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
    
    std::optional<std::unique_ptr<Schema>> tryDecode(const std::vector<uint8_t>& message, const BasicTopicInformation& topicInfo) const;

    void dispatchOnDecode(const std::vector<uint8_t>& message, const BasicTopicInformation& topicInfo);
};


class TopicTranslator {
  const std::shared_ptr<BasicTopicInformation> topicInfo;
  const std::shared_ptr<Registry> registry;

public:
  TopicTranslator(const std::shared_ptr<BasicTopicInformation> &topicInfo,
                  const std::shared_ptr<Registry> &registry)
      : topicInfo(topicInfo), registry(registry) {}

  std::optional<std::unique_ptr<Schema>>
  tryDecodeMessage(const message_t &message);

  std::optional<std::unique_ptr<message_t>>
  tryEncodeMessage(const Schema &schema) const;
};


class TopicMessenger {
  std::unique_ptr<MessagingQueue> messagingQueue;
  std::shared_ptr<TopicTranslator> translator;

  public:
  TopicMessenger(std::unique_ptr<MessagingQueue>&& messagingQueue, const std::shared_ptr<TopicTranslator> translator)
    : messagingQueue(std::move(messagingQueue)), translator(translator) {}
  
  void sendMessage(const Schema &schema);

  std::optional<std::unique_ptr<Schema>> tryGetNexMessage();
};




} // namespace nat::core
