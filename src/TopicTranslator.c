#include <libnatkit-core.h>

void *TopicTranslator_tryDecodeMessage(const message_t *message) {
    return Registry_TryDecode(message, *topicInfo);
}

void *TopicTranslator_tryEncodeMessage(const Schema *schema) {
    if(isSerializationTypeSupported(schema, topicInfo -> SerializationType)) {
        return encodeToBytes(schema, topicInfo -> serializationType);
    } else {
        return NULL;
    }
}
