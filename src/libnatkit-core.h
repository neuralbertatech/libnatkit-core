#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include <stdbool.h>


// 1. STRINGS //
typedef struct {
    char** array;
    size_t size;
} StringArray;

StringArray split(const char* string, char delimiter) {
    StringArray strings;
    strings.array = NULL;
    strings.size = 0;

    const char* token = strtok((char*)string, &delimiter);
    while (token != NULL) {
        strings.array = realloc(strings.array, (strings.size + 1) * sizeof(char*));
        strings.array[strings.size] = strdup(token);
        strings.size++;

        token = strtok(NULL, &delimiter);
    }

    return strings;
}



// 2. LOWERCASE //
char* toLowerCase(const char* string) {
    size_t length = lenstr(string);
    char* lowerCaseString = malloc(length + 1);

    for (size_t i = 0, i < length, i++) {
        lowerCaseString[i] = tolower(string[i]);
    }

    lowerCaseString[length] = '\0';

    return lowerCaseString;
}


//3. VECTORS //

typedef struct {
    int* data;
    size_t size;
} Vector;

typedef struct {
    uint8_t *data;
    size_t size;
    size_t capacity;
} vector_uint8_t;

Vector* createVector(const int* array, size_t size) {
    Vector* vec = (Vector*)malloc(sizeof(Vector));
    if(vec == NULL) {
        return NULL;
    }
    vec -> data = (int*)malloc(size * sizeof(int));
    if (vec -> data == NULL) {
        free(vec);
        return NULL;
    }
    vec -> size = size;
    for (size_t i = 0; i < size; i++) {
        vec -> data[i] = array[i];
    }
    return vec;
}

void freeVector(Vector* vec) {
    if (vec != NULL) {
        free(vec -> data);
        free(vec);
    }
}

Vector* wrapContainedValueWithUnique(const Vector* vec) {
    Vector* wrappedVec = createVector(NULL, vec -> size);
    if(wrappedVec == NULL) {
        return;
    }
    for (size_t i = 0; i < vec -> size; i++) {
        wrappedVec -> data[i] = vec -> data[i];
    }
    return wrappedVec;
}

// 4. MESSAGE_T //

typedef struct {
    uint8_t* data;
    size_t size_t;
} message_t;

typedef struct {
    char* streamName;
} StreamInfo;

StreamInfo* createStreamInfo(const char* streamName) {
    StreamInfo* streamInfo = (StreamInfo*)malloc(sizeof(StreamInfo));
    if(streamInfo == NULL) {
        return NULL;
    }
    streamInfo -> streamName = strdup(streamName);
    if (streamInfo -> streamName == NULL) {
        free(streamInfo);
        return NULL;
    }
    return streamInfo;
}

void freeStreamInfo(StreamInfo* streamInfo) {
    if (streamInfo != NULL) {
        free(streamInfo -> streamName);
        free(streamInfo);
    }
}

// 5. Enums //

typedef enum {
    Json
} SerializationType;

typedef struct {
    SerializationType type;
    const char* string;
} MappingEntry;

static const MappingEntry serializationTypeToStringMapping[] = {
    {Json, "Json"}
};
static const size_t serializationTypeToStringMappingSize = sizeof(serializationTypeToStringMapping) / sizeof(serializationTypeToStringMapping[0]);

static MappingEntry stringToSerializationTypeMapping[] = {
    {Json, "Json"}
};
static const size_t stringToSerializationTypeMappingSize = sizeof(stringToSerializationTypeMapping) / sizeof(stringToSerializationTypeMapping[0]);

void toLowercase(char* str) {
    while (*str) {
        *str = (char)tolower(*str);
        str++;
    }
}

void initializeStringToSerializationTypeMapping() {
    for (size_t i = 0; i < serializationTypeToStringMappingSize; i++) {
        char* lowercaseString = strdup(serializationTypeToStringMapping[i].string);
        toLowercase(lowercaseString);
        stringToSerializationTypeMapping[i].string = lowercaseString;
    }
}

SerializationType getSerializationTypeFromString(const char* str) {
    for (size_t i = 0; i < stringToSerializationTypeMappingSize; i++) {
        if (strcmp(str, stringToSerializationTypeMapping[i].string) == 0) {
            return stringToSerializationTypeMapping[i].type;
        }
    }
    return -1; 
}

void freeSerialization() {
    for (size_t i = 0; i < stringToSerializationTypeMappingSize; i++) {
        free((void*)stringToSerializationTypeMapping[i].string);
    }
}

typedef enum {
    //Core types
    DATA,
    META,

    // Execution Extension
    EXECUTION_COMMAND,

    // Hardware Extension
    HARDWARE_STATUS,
    HARDWARE_CONFIGURATION,

    // Logging Extension
    LOGGING_LOG,
    LOGGING_HEARTBEAT,

} StreamType;

typedef struct {
    StreamType type;
    const char* string;

} StreamTypeStringPair;

static const StreamTypeStringPair streamTypeToStringMapping[] = { 
    {DATA, "Data"},
    {META, "Meta"},
    {EXECUTION_COMMAND, "Command"},
    {HARDWARE_STATUS, "Status"},
    {HARDWARE_CONFIGURATION, "Configuration"},
    {LOGGING_LOG, "Log"},
    {LOGGING_HEARTBEAT, "Heartbeat"},
};

void initializeStringToStreamTypeMapping(const StreamTypeStringPair* mapping, size_t size, char** keys, StreamType* values) {

    for (size_t i = 0; i < size; i++) {
        char* lowerCaseString = strdup(mapping[i].string);
        toLowerCase(lowerCaseString);

        keys[i] = lowerCaseString;
        values[i] = mapping[i].type;
    }
}

void freeStringToStreamTypeMapping(char** keys, StreamType* values, size_t size) {
    for (size_t i = 0; i < size; i++) {
        free(keys[i]);
    }
    free(keys);
    free(values);
}

char* toString(const StreamType* streamType);

typedef struct {
    int hasValue;
    StreamType value;
} OptionalStreamType;

OptionalStreamType streamTypeFromString(const char *streamTypeString);

//6. STREAM //

struct Stream {
    const char* name;
    StreamType type;
    uint64_t id;
    const char* encoderName;
    SerializationType serializationType;
    const char* schemaName;
};

void Stream_init(struct Stream *stream, const char *name, StreamType type, uint64_t id, const char *encoderName, SerializationType serializationType, const char *schemaName) {
    Stream stream;
    stream.name = *name;
    stream.type = type;
    stream.id = id;
    stream.encoderName = *encoderName;
    getSerializationTypeFromString(encoderName);
    stream.schemaName = *schemaName;
    return stream;
}


Stream createKafkaBrokerName(const String *brokerName) {
    Stream stream;
    stream.name = brokerName;
    stream.type = getSerializationTypeFromString(brokerName);
    return stream;
}

typedef struct {
    char *data;
} String;

String Stream_getName(const Stream *stream) {
    return stream -> name;
}

StreamType Stream_getType(const Stream *stream) {
    return stream -> type;
}

uint64_t Stream_getId(const Stream *stream) {
    return stream -> id;
}

String Stream_getEncoderName(const Stream *stream) {
    return stream -> encoderName;
}

SerializationType Stream_getSerializationType(const Stream *stream) {
    return stream -> serializationType;
}

String Stream_getSchemaName(const Stream *stream) {
    return stream -> schemaName;
}

String toTopicString(const Stream *stream) {
    String result;
    result.data = malloc(strlen(stream->encoderName.data) + 50);
    sprintf(result.data, "%d-%llu-%s-%s", stream->type, stream->id, stream.encoderName.data, stream-> schemaName.data);
    return result;
}

void String_free(String *str) {
    free(str->data);
    str->data = NULL;
}

typedef struct {
    uint8_t* data;
    size_t size;
} Vector;

typedef struct {
    Vector message;
    Stream stream;
} StreamMessage;

StreamMessage StreamMessage_create(const Vector *message, const Stream *stream) {
    StreamMessage msg;
    msg.message = *message;
    msg.stream = *stream;
    return msg;
}

Vector StreamMessage_getMessage(const StreamMessage *msg) {
    return msg -> message;
}

SerializationType StreamMessage_getSerializationType(const StreamMessage *msg) {
    return getSerializationType(&msg-> stream);
}

String StreamMessage_getSchemaName(const StreamMessage *msg) {
    return getSchemaName(&msg -> stream);
}

// 7. SCHEMA //

typedef struct {
char content[256];
} message_t;

typedef struct {
    bool (*isSerializationTypeSupported)(const SerializationType);
    message_t* (*encodeToBytes)(const SerializationType*);
    const char* (*Stream_getSchemaName)();
    const char* (*toString)();
    const char* (*getName)(struct Schema* self);
} Schema;

Schema* createSchema() {
    Schema* schema = malloc(sizeof(Schema));
    if (schema != NULL) {
        schema -> isSerializationTypeSupported = NULL;
        schema -> encodeToBytes = NULL;
        schema -> Stream_getSchemaName = NULL;
        schema -> toString = NULL;
    }
    return schema;
}

void freeSchema(Schema* schema) {
    free(schema);
}

typedef Schema* (*decoder_t)(const uint8_t* message, size_t message_size, const SerializationType* type);

// 8. DECODE & ENCODE //
typedef struct {
    bool (*isSerializationTypeSupported)(const SerializationType);
    decoder_t tryDecode;
} Decoder;

Decoder* createDecoder(){
    Decoder* decoder = malloc(sizeof(Decoder));
    if (decoder != NULL) {
        decoder -> isSerializationTypeSupported = NULL;
        decoder -> tryDecode = NULL;
    }
    return decoder;
} 

void freeDecoder(Decoder* decoder) {
    free(decoder);
}

typedef struct {
    bool (*isSerializationTypeSupported)(const SerializationType);
    uint8_t* (*encode)(const StreamMessage);
} Encoder;

Encoder* createEncoder() {
    Encoder* encoder = malloc(sizeof(Encoder));
    if (encoder != NULL) {
        encoder -> isSerializationTypeSupported = NULL;
        encoder -> encode = NULL;
    }
    return encoder;
}

void freeEncoder(Encoder* encoder) {
    free(encoder);
}

typedef struct {
    Schema baseSchema;
    Decoder baseDecoder;
    const char* streamName;
} BasicMetaInfoSchema;

Schema* decodeJson(const uint8_t* message, size_t message_t, const SerializationType* type) {
    const char* streamName = (const char*)message;  //might have to parse?
}

// 9. BASIC META //

BasicMetaInfoSchema* createBasicMetaInfoSchema(const char* streamName) {
    BasicMetaInfoSchema* schema = malloc(sizeof(BasicMetaInfoSchema));

    if (schema != NULL) {
        schema->baseSchema.isSerializationTypeSupported = &isSerializationTypeSupported;
        schema->baseSchema.encodeToBytes = &encodeToBytes;
        schema->baseSchema.getName = NULL; 
        schema->baseSchema.toString = &toString;
        schema->baseDecoder.isSerializationTypeSupported = &isSerializationTypeSupported;
        schema->baseDecoder.tryDecode = &decodeJson;
        schema->streamName = streamName;
    }
    return schema;
}

void freeBasicMetaInfoSchema(BasicMetaInfoSchema* schema) {
    free(schema);
}

typedef struct Schema Schema;
typedef struct {
    Schema** schemas;
    size_t count;
    size_t capacity;
} Registry;

typedef void (*DispatchMethod)(Schema*);
void decodeAndDispatch(const uint8_t* message, size_t message_size, const SerializationType* type, DispatchMethod dispatchMethod);

Schema* tryDecode(const uint8_t* message, size_t message_size, const SerializationType* type);

void registerWithRegistry(Registry* registry, Schema* schema) {
    printf("%s\n", schema -> getName(schema));
}

const char* getName(Schema* self) {
    return "SchemaName";
}

const char* getStreamName(Schema* self) {
    return "StreamName";
}

// 10. BASIC TOPIC //

typedef struct {
    const StreamType type;
    const SerializationType serializationType;
    const uint64_t id;
    const char *schemaName;
} BasicTopicInformation;

BasicTopicInformation *createBasicTopicInformation(StreamType type, SerializationType serializationType, uint64_t id, const char *schemaName) {
    BasicTopicInformation *info = (BasicTopicInformation *)malloc(sizeof(BasicTopicInformation));
    if (info != NULL) {
        info -> type = type;
        info -> serializationType = serializationType;
        info -> id = id;
        info -> schemaName = strdup(schemaName);
        if (info -> schemaName == NULL) {
            free(info);
            return NULL;
        }
    }
    return info;
}

void freeBasicTopicInformation(BasicTopicInformation *info) {
    if (info != NULL) {
        free((void *)info -> schemaName);
        free(info);
    }
}

char *toString_BasicTopicInformation(const BasicTopicInformation *info) {
    if (info == NULL)
        return NULL;

    char *result = (char *)malloc(100 * sizeof(char));
    return result;
}

char *toTopicString_BasicTopicInformation(const BasicTopicInformation *info) {
    if (info == NULL)
        return NULL;

    char *result = (char *)malloc(100 * sizeof(char));
    return result;
}

// 11. JSON //

typedef struct JsonDecoder {
    Decoder base;
    bool (*isSerializationTypeSupported)(const SerializationType type);
} JsonDecoder;

bool JsonDecoder_isSerializationTypeSupported(const Decoder *decoder, SerializationType type);
Schema* JsonDecoder_tryDecode(const JsonDecoder *decoder, const uint8_t *message, size_t message_length, const SerializationType *type);

typedef struct {
    Encoder base;
} JsonEncoder;

bool JsonEncoder_isSerializationTypeSupported(const Encoder *encoder, SerializationType type);
uint8_t* JsonEncoder_encode(const JsonEncoder *encoder, const StreamMessage *message);

// 12. MESSAGE //

typedef struct {
    void (*enqueueMessageToSend)(struct MessagingQueue *queue, message_t *message);
    void (*enqueueMessageToReceive)(struct MessagingQueue *queue, const message_t *message);
    message_t* (*tryGetNextMessage)(struct MessagingQueue *queue);
} MessagingQueue;

void MessagingQueue_enqueueMessageToSend(struct MessagingQueue *queue, message_t *message);
void MessagingQueue_enqueueMessageToReceive(struct MessagingQueue *queue, const message_t *message);
message_t* MessagingQueue_tryGetNextMessage(struct MessagingQueue *queue);

typedef struct {
    const char *plainTextMessage;
} PlainTextMessage;

PlainTextMessage* PlainTextMessage_create(const char *message) {
    PlainTextMessage *new_message = (PlainTextMessage *)malloc(sizeof(PlainTextMessage));
    if (new_message != NULL) {
        new_message -> plainTextMessage = strdup(message);
    }
    return new_message;
}

const char* getPlainTextMessage(const PlainTextMessage *message) {
    return message -> plainTextMessage;
}

void freePlainTextMessage(PlainTextMessage *message) {
    if (message != NULL) {
        free((void *)message -> plainTextMessage);
        free(message);
    }
}

typedef struct {
    const uint64_t id;
    BasicTopicInformation** topics;
    size_t num_topics;
} RawStream;

RawStream* RawStream_create(const uint64_t id, BasicTopicInformation** topics, size_t num_topics) {
    RawStream* stream = (RawStream*)malloc(sizeof(RawStream));
    if (stream != NULL) {
        stream -> id = id;
        stream -> topics = topics;
        stream -> num_topics = num_topics;
    }
    return stream;
}

void freeRawStream(RawStream* stream) {
    if (stream != NULL) {
        for (size_t i = 0; i < stream -> num_topics; i++) {
            free(stream -> topics[i]);
        }
        free(stream -> topics);
        free(stream);
    }
}

void RawStream_addTopic(RawStream* stream, const BasicTopicInformation* topic) {
    size_t new_num_topics = stream -> num_topics + 1;
    stream -> topics = (BasicTopicInformation**)realloc(stream -> topics, sizeof(BasicTopicInformation*) * new_num_topics);
    if (stream -> topics != NULL) {
        stream -> topics[new_num_topics - 1] = (BasicTopicInformation*)malloc(sizeof(BasicTopicInformation));
        if (stream -> topics[new_num_topics - 1] != NULL) {
            memcpy(stream -> topics[new_num_topics - 1], topic, sizeof(BasicTopicInformation));
            stream -> num_topics = new_num_topics;
        }
    }
}

uint64_t RawStream_getId(const RawStream* stream) {
    return stream -> id;
}

char* RawStream_toPrettyString(const RawStream* stream);
char* RawStream_toString(const RawStream* stream);

typedef struct {
    Registry* createDefaultInitalizeRegistry();
    void registerEncoder(Registry* registry, const char* schemaName, const char* type, const Encoder* encoder);
    void registerDecoder(Registry* registry, const char* schemaName, const char* type, decoder_t decoder);
    void registerSchemaHandler(Registry* registry, const char* schemaName, const char* type, void (*dispatchFunction)(const Schema*));
    Schema* tryDecode(const Registry* registry, const unsigned char* message, size_t message_length, const BasicTopicInformation* topicInfo);
    void dispatchOnDecode(const Registry* registry, const unsigned char* message, size_t message_length, const BasicTopicInformation* topicInfo);
} Registry;

// 13. TRANSLATE //

typedef struct {
    void *ptr;
} sharedPtr;

typedef struct {
    void *ptr;
} uniquePtr;

typedef struct {
    int has_value;
    void *value;
} Optional;

typedef struct {
    const sharedPtr topicInfo;
    const sharedPtr registry;
} TopicTranslator;

TopicTranslator* TopicTranslator_create(const sharedPtr *topicInfo, const sharedPtr *registry) {
    TopicTranslator *translator = (TopicTranslator)malloc(sizeof(TopicTranslator));
    translator -> topicInfo = *topicInfo;
    translator -> registry = *registry;
    return translator;
}

void freeTopicTranslator(TopicTranslator *translator) {
    free(translator);
}

Optional TopicTranslator_tryDecodeMessage(const TopicTranslator *translator, const message_t *message) {
    Optional result;
    result.has_value = 0;
    result.value = NULL;
    return result;
} 

Optional TopicTranslator_tryEncodeMessage(const TopicTranslator *translator, const Schema *schema) {
    Optional result;
    result.has_value = 0;
    result.value = NULL;
    return result;
} 

typedef struct {
    uniquePtr messagingQueue;
    sharedPtr translator;
} TopicMessenger;

TopicMessenger* TopicMessenger_create(uniquePtr *messagingQueue, sharedPtr *translator) {
    TopicMessenger *messenger = (TopicMessenger*)malloc(sizeof(TopicMessenger));
    messenger -> messagingQueue = *messagingQueue;
    messenger -> translator = *translator;
    return messenger;
}

void freeTopicMessenger(TopicMessenger *messenger) {
    free(messenger);
}

void TopicMessenger_sendMessage(const TopicMessenger *messenger, const Schema *schema);

Optional TopicMessenger_tryGetNextMessage(const TopicMessenger *messenger) {
    Optional result;
    result.has_value = 0;
    result.value = NULL;
    return result;
}
