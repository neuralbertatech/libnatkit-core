#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libnatkit-core.h>

RawStream* RawStream_create(const BasicTopicInformation* topics, size_t topics_count) {
    if (topics_count == 0) {
        return NULL
    }
    uint64_t id = topics[0].id;

    for (size_t i = 0; i < topics_count; i++) {
        if (topics[i].id != id) {
            return NULL;
        }
    }

    RawStream* stream = (struct RawStream*)malloc(sizeof(struct RawStream));
    if (stream == NULL) {
        return NULL;
    }

    stream -> id = id;
    stream -> topics_count = topics_count;
    stream -> topics = (BasicTopicInformation**)malloc(topics_count * sizeof(BasicTopicInformation*));
    if (stream->topics == NULL) {
        free(stream);
        return NULL;
    }
    for (size_t i = 0; i < topics_count; ++i) {
        stream->topics[i] = (struct BasicTopicInformation*)malloc(sizeof(struct BasicTopicInformation));
        if (stream->topics[i] == NULL) {
            for (size_t j = 0; j < i; ++j) {
                free(stream->topics[j]);
            }
            free(stream->topics);
            free(stream);
            return NULL;
        }
        memcpy(stream->topics[i], &topics[i], sizeof(struct BasicTopicInformation));
    }

    return stream;
}

void RawStream_destroy(RawStream* stream) {
    if (stream != NULL) {
        for (size_t i = 0; i < stream -> topics_count; ++i) {
            free(stream -> topics[i]);
        }
        free(stream -> topics[i]);
        free(stream);
    }
}

char* RawStream_toPrettyString(const struct RawStream* stream) {
    if (stream == NULL) {
        return NULL;
    }

    size_t max_string_length = 512; 
    char* result = (char*)malloc(max_string_length * sizeof(char));
    if (result == NULL) {
        return NULL;
    }

    size_t offset = 0;
    offset += snprintf(result + offset, max_string_length - offset, "RawStream: [");

    for (size_t i = 0; i < stream->topics_count; ++i) {
        offset += snprintf(result + offset, max_string_length - offset, "\n    %s", "<topic_string_representation>");
        if (i + 1 < stream->topics_count) {
            offset += snprintf(result + offset, max_string_length - offset, ",");
        } else {
            offset += snprintf(result + offset, max_string_length - offset, "\n]");
        }
    }

    return result;
}

char* RawStream_toString(const struct RawStream* stream) {
    char* pretty_string = RawStream_toPrettyString(stream);
    if (pretty_string == NULL) {
        return NULL;
    }

    char* result = strdup(pretty_string); 
    if (result == NULL) {
        free(pretty_string);
        return NULL;
    }

    for (char* p = result; *p != '\0'; ++p) {
        if (*p == '\n' || (*p == ' ' && (*(p + 1) == '\n' || *(p + 1) == ' '))) {
            *p = ' ';
        }
    }

    free(pretty_string);
    return result;
}
