#include <libnatkit-core.h>

const char *toString(StreamType streamType) {
    switch(streamType) {
        case DATA:
            return streamTypeToStringMapping[DATA].string;
        case META:
            return streamTypeToStringMapping[META].string;
        case EXECUTION_COMMAND:
            return streamTypeToStringMapping[EXECUTION_COMMAND].string;
        case HARDWARE_STATUS:
            return streamTypeToStringMapping[HARDWARE_STATUS].string;
        case HARDWARE_CONFIGURATION:
            return streamTypeToStringMapping[HARDWARE_CONFIGURATION].string;
        case LOGGING_LOG:
            return streamTypeToStringMapping[LOGGING_LOG].string;
         case LOGGING_HEARTBEAT:
            return streamTypeToStringMapping[LOGGING_HEARTBEAT].string;
        default:
            assert(0);
            return NULL;
    }
}

StreamType streamTypeFromString(const char* streamTypeString) {
    char lowercaseType[256];
    strcpy(lowercaseType, streamTypeString);
    toLowerCase(lowercaseType);

    for (int i = 0; i < sizeof(streamTypeToStringMapping) / sizeof(streamTypeToStringMapping[0]); i++) {
        if (strcmp(streamTypeToStringMapping[i].string, lowercaseType) == 0) {
            return streamTypeToStringMapping[i].type;
        } else {
            return NULL;
        }
    }
}
