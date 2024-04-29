#include <libnatkit-core.h>
#include <stdio.h>
#include <stdlib.h>

BasicTopicInformation newBasicTopicInformation(const char *kafkaTopicString) {
    const auto *splitName = strtok(kafkaTopicString, '-');
    if(len(splitName) != 4) {
        printf("Topic String does not contain the four parts\n");
        return;
    }
    const auto streamTypeName = splitName[0];
    const auto streamIdString = splitName[1];
    const auto streamEncoderName = splitName[2];
    const auto streamSchemaName = splitName[3];

    auto streamTypeMaybe = streamTypeFromString(streamTypeName);
    const auto serializationTypeMaybe = getSerializationTypeFromString(streamEncoderName);
    const auto streamId = strtoll(streamIdString, NULL, 10);  //base 10

    if (streamTypeMaybe == NULL) {
        printf("\"%s\" is not a valid stream type\n", streamTypeName);
        return;
    }
    if (serializationTypeMaybe == NULL) {
        printf("\"%s\" is not a valid serialization type\n");
        return;
    }

    createBasicTopicInformation(streamTypeMaybe, serializationTypeMaybe, streamId, streamSchemaName);
}

char* basicTopicInformationToString(const BasicTopicInformation *topicInfo) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "BasicTopicInformation: {type=\"%s\", id=\"%s\", serializationType=\"%s\", schemaName=\"%s\"}", streamTypeToString(topicInfo->type), topicInfo->id, serializationTypeToString(topicInfo->serializationType), topicInfo->schemaName);
    return strdup(buffer);
}

char* basicTopicInformationToTopicString(const BasicTopicInformation *topicInfo) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s-%d-%s-%s", streamTypeToString(topicInfo->type), topicInfo->id, serializationTypeToString(topicInfo->serializationType), topicInfo->schemaName);
    return strdup(buffer);
}
