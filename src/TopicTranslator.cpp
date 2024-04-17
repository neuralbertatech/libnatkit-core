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

} // namespace core
} // namespace nat
