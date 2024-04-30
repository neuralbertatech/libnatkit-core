#include <libnatkit-core.h>

Schema* tryDecodeMessage(const message_t *message, const Registry *registry, const *topicInfo) {
    Schema *decodedSchema = registry-> tryDecode(message, *topicInfo);
}

message_t* tryEncodeMessage(const Schema *schema, BasicTopicInformation *topicInfo) {
    if(isSerializationTypeSupported(schema, topicInfo -> serializationType)) {
        return encodeToBytes(schema, topicInfo -> serializationType);
    } else {
        return NULL;
    }
}
