#include <libnatkit-core.hpp>

namespace nat {
namespace core {

  void TopicMessenger::sendMessage(const Schema &schema) {
    auto encodedMessageMaybe = translator->tryEncodeMessage(schema);
    if (encodedMessageMaybe.has_value()) {
      messagingQueue->enqueueMessageToSend(std::move(encodedMessageMaybe.value()));
    }
  }

  Optional<std::unique_ptr<Schema>> TopicMessenger::tryGetNexMessage() {
    const auto message = messagingQueue->tryGetNextMessage();
    if (message.has_value()) {
      return translator->tryDecodeMessage(*message.value());
    } else {
      return {};
    }
  }

} // namespace core
} // namespace nat
