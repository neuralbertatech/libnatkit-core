#include <libnatkit-core.hpp>

namespace nat {
namespace core {

  void TopicMessenger::sendMessage(const Schema &schema) {
    auto encodedMessageMaybe = translator->tryEncodeMessage(schema);
    if (encodedMessageMaybe.has_value()) {
      messagingQueue->enqueueMessageToSend(std::move(encodedMessageMaybe.value()));
    }
  }

  void TopicMessenger::sendRawMessage(std::unique_ptr<message_t> &&message) {
    if (message) {
      messagingQueue->enqueueMessageToSend(std::move(message));
    }
  }

  void TopicMessenger::sendRawMessage(const message_t &message) {
    messagingQueue->enqueueMessageToSend(
        nat::core::make_unique<message_t>(message.begin(), message.end()));
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

  Optional<std::shared_ptr<message_t>> TopicMessenger::tryGetNextRawMessage() {
    return messagingQueue->tryGetNextMessage();
  }

  Optional<std::unique_ptr<Schema>> TopicMessenger::tryGetNexMessage() {
    const auto message = tryGetNextRawMessage();
    if (message.has_value()) {
      return translator->tryDecodeMessage(*message.value());
    } else {
      return {};
    }
  }

  void TopicMessenger::clearAllMessages() {
      messagingQueue->clearAllMessages();
  }

  void TopicMessenger::flush() {
      messagingQueue->flush();
  }

} // namespace core
} // namespace nat
