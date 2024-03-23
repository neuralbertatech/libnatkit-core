#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <libnatkit-core.h>

MAX_STR_LEN = 256
enum SerializationType getSerializationType(const char* encoderName) {
    char lowercaseEncoderName[MAX_STR_LEN];
    size_t i = 0;
    while (encoderName[i] != '\0' && i < MAX_STR_LEN -1) {
        lowercaseEncoderName[i] = tolower(encoderName[i]);
        i++:
    }
    lowercaseEncoderName[i] = '\0';

    if (strcmp(lowercaseEncoderName, "json") == 0) {
        return Json
    }

    printf("Fatal Error: Invalid serialization type '%s'\n", encoderName);
    assert(0);
}

const char* toString(enum SerializationType serializationType) {
    switch (serializationType) {
        case Json:
            return "Json";
        default:
            assert(0);
            return NULL;
    }
}


SerializationType serializationTypeFromString(const char *serializationTypeString) {
    char lowercaseType[MAX_STR_LEN];
    strcpy(lowercaseType, serializationTypeString);
    toLowercase(lowercaseType);
    for (size_t i = 0; i < sizeof(lowercaseStringToSerializationTypeMapping) / sizeof(lowercaseStringToSerializationTypeMapping[0]); ++i) {
        if (strcmp(lowercaseStringToSerializationTypeMapping[i].lowercaseType, lowercaseType) == 0) {
            return lowercaseStringToSerializationTypeMapping[i].type;
        }
    }
    return -1;

}
