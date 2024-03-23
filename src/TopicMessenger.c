#include <libnatkit-core.h>

void TopicMessenger_sendMessage(const Schema *schema) {
    auto encodedMessageMaybe = TopicTranslator_tryEncodeMessage(schema);
    if(encodeMessageMaybe) {
        MessagingQueue_enqueueMessageToSend(encodedMessageMaybe);
    }
}

void *TopicMessenger_tryGetNextMessage() {
    const void *message = MessagingQueue_tryGetNextMessage();
    if(message) {
        return TopicTranslator_tryDecodeMessage(message);
    } else {
        return NULL;
    }
}
