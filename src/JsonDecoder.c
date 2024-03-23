#include <libnatkit-core.h>

bool JsonDecoder_isSerializationTypeSupported(SerializationType type) {
    switch(type) {
        case Json:
            return true;
        default:
            return false;
    }
}

void JsonDecoder_tryDecode(const uint8_t *message, size_t message_size, SerializationType type) {
    switch(type) {
        case Json:
            return NULL;
        default:
            return NULL;
    }
}
