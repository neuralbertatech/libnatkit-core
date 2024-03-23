#include <libnatkit-core.h>

const char *toString(StreamType streamType) {
    switch(streamType) {
        case DATA:
            return streamTypeToStringMapping[DATA];
        case META:
            return streamTypeToStringMapping[META];
        case EXECUTION_COMMAND:
            return streamTypeToStringMapping[EXECUTION_COMMAND];
        case HARDWARE_STATUS:
            return streamTypeToStringMapping[HARDWARE_STATUS];
        case HARDWARE_CONFIGURATION:
            return streamTypeToStringMapping[HARDWARE_CONFIGURATION];
        case LOGGING_LOG:
            return streamTypeToStringMapping[LOGGING_LOG];
         case LOGGING_HEARTBEAT:
            return streamTypeToStringMapping[LOGGING_HEARTBEAT];
        default:
            assert(0);
            return NULL;
    }
}

const StreamType *streamTypeFromString(const char *streamTypeString) {
    char lowercaseType[256];
    toLowerCase(streamTypeString, lowercaseType);

    if(strstr(initializeStringToStreamTypeMapping, lowercaseType) != NULL) {
        return &initializeStringToStreamTypeMapping[lowercaseType];
    } else {
        return NULL;
    }
}
