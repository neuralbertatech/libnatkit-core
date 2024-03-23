#include <libnatkit-core.h>

bool JsonEncoder_isSerializationTypeSupported(SerializationType type) {
    switch(type) {
        case Json:
            return true;
        default:
            return false;
    }
}

vector_uint8_t JsonEncoder_encode(const StreamMessage* message) {
    vector_uint8_t result;
    return result;
}

