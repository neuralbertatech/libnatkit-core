#include <libnatkit-core.hpp>


namespace nat {
namespace core {

  Optional<std::unique_ptr<Schema>>
  TopicTranslator::tryDecodeMessage(const message_t &message) {
    return registry->tryDecode(message, *topicInfo);
  }

  Optional<std::unique_ptr<message_t>>
  TopicTranslator::tryEncodeMessage(const Schema &schema) const {
    if (schema.isSerializationTypeSupported(topicInfo->serializationType)) {
      return schema.encodeToBytes(topicInfo->serializationType);
    } else {
      return {};
    }
  }

  StreamType TopicTranslator::getStreamType() const {
      return topicInfo->type;
  }

  SerializationType TopicTranslator::getSerializationType() const {
      return topicInfo->serializationType;
  }

  uint64_t TopicTranslator::getId() const {
      return topicInfo->id;
  }

  std::string TopicTranslator::getSchemaName() const {
      return topicInfo->schemaName;
  }

} // namespace core
} // namespace nat
