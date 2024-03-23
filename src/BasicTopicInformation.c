#include <libnatkit-core.h>
#include <stdio.h>
#include <stdlib.h>

BasicTopicInformation createBasicTopicInformation(const char *kafkaTopicString) {
    const auto splitName = split(kafkaTopicString, '-');
    if(len(splitName) != 4) {
        printf("Topic String does not contain the four parts\n");
        return 0;
    }
    const auto streamTypeName = splitName[0];
    const auto streamIdString = splitName[1];
    const auto streamEncoderName = splitName[2];
    const auto streamSchemaName = splitName[3];

    const auto streamTypeMaybe = streamTypeFromString(streamTypeName);
    const auto serializationTypeMaybe = getSerializationTypeFromString(streamEncoderName);
    const auto streamId = strtoll(streamIdString, NULL, 10);  //base 10

    if (streamTypeMaybe == NULL) {
        printf("\"%s\" is not a valid stream type\n", streamTypeName);
        return 0;
    }
    if (serializationTypeMaybe == NULL) {
        printf("\"%s\" is not a valid serialization type\n");
        return 0;
    }

    return BasicTopicInformation(streamTypeMaybe, serializationTypeMaybe, streamId, streamSchemaName);
}

BasicTopicInformation toString() {
    BasicTopicInformation buffer[256];
    sprintf(buffer, "BasicTopicInformation: {type=\"%s\", id=\"%s\", serializationType=\"%s\", schemaName=\"%s\"}", type, id, serializationType, schemaName);
    return buffer
}

BasicTopicInformaiton toTopicString() {
    BasicTopicInformation buffer[256];
    sprintf(buffer, "\"%s\"-\"%s\"-\"%s\"-\"%s\"}", type, id, serializationType, schemaName);
    return buffer;
}
