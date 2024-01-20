#include <libnatkit-core.hpp>


namespace nat::core {

  std::optional<std::unique_ptr<Schema>>
  TopicTranslator::tryDecodeMessage(const message_t &message) {
    return registry->tryDecode(message, *topicInfo);
  }

  std::optional<std::unique_ptr<message_t>>
  TopicTranslator::tryEncodeMessage(const Schema &schema) const {
    if (schema.isSerializationTypeSupported(topicInfo->serializationType)) {
      return schema.encodeToBytes(topicInfo->serializationType);
    } else {
      return {};
    }
  }

} // namespace nat::core
