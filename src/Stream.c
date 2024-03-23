#include <libnatkit-core.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Optional createKafkaBrokerName(const char *brokerName) {
    char *splitName[4];
    int i = 0;
    char *token = strtok(brokerName, "-");
    while (token != NULL && i < 4) {
        splitName[i++] = token;
        token = strtok(NULL, "-");
    }
    if (i != 4) {
        Optional result;
        return result;
    }

    const char *streamTypeName = splitName[0];
    const char *streamIdString = splitName[1];
    const char *streamEncoderName = splitName[2];
    const char *streamSchemaName = splitName[3];

    long long streamId = strtoll(streamIdString, NULL, 10);

    Optional streamTypeMaybe = streamTypeFromString(streamTypeName);
    if (streamTypeMaybe.has_value()) {
        return Stream(brokerName, streamTypeMaybe.value(), streamId, streamEncoderName, streamSchemaName);
    } else {
        Optional result;
        return result;
    }
}
