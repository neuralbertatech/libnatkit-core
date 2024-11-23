#include <libnatkit-core.hpp>

namespace nat {
namespace core {

  void TopicMessenger::sendMessage(const Schema &schema) {
    auto encodedMessageMaybe = translator->tryEncodeMessage(schema);
    if (encodedMessageMaybe.has_value()) {
      messagingQueue->enqueueMessageToSend(std::move(encodedMessageMaybe.value()));
    }
  }

  StreamType TopicMessenger::getStreamType() const {
      return translator->getStreamType();
  }

  SerializationType TopicMessenger::getSerializationType() const {
      return translator->getSerializationType();
  }

  uint64_t TopicMessenger::getId() const {
      return translator->getId();
  }

  std::string TopicMessenger::getSchemaName() const {
      return translator->getSchemaName();
  }

  Optional<std::unique_ptr<Schema>> TopicMessenger::tryGetNexMessage() {
    const auto message = messagingQueue->tryGetNextMessage();
    if (message.has_value()) {
      return translator->tryDecodeMessage(*message.value());
    } else {
      return {};
    }
  }

  void TopicMessenger::clearAllMessages() {
      messagingQueue->clearAllMessages();
  }

} // namespace core
} // namespace nat
