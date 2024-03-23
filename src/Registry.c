#include <libnatkit-core.h>
#include <stdio.h>

Registry createKey(const char *schemaName, const SerializationType *type) {
    Registry buffer[256];
    char* serializedType = toStringSerializationType(type);
    sprintf(buffer, "%s-%s", schemaName, serializedType);
    return buffer;
}

Registry createKeyStreamMessage(const StreamMessage *message) {
    return createKey(message.getSchemaName(), message.getSerializationType());
}

Registry createKeyBasicTopicInfo(const BasicTopicInformation *topicInfo) {
    return createKey(topicInfo.schemaName, topicInfo.serializationType);
}

Registry createDefaultInitialRegistry() {
    Registry registry = (Registry *)malloc(sizeof(struct Registry));
    BasicMetaInfoSchema resgisterWithRegistry(*registry);
    return registry;
}

void registerEncoder(const char *schemaName, SerializationType *type, Encoder *encoder) {
    char *key = createKey(schemaName, type);
    if (key != NULL) {
        insertEncoder(resgistry, key, encoder);
    } else {
        updateEncoder(registry, key, encoder);
    }
    free(key);
}

void registerDecoder(const char *schemaName, SerializationType *type, decoder_t *decoder) {
    char *key = createKey(schemaName, type);
    if (key != NULL) {
        insertDecoder(resgistry, key, decoder);
    } else {
        updateDecoder(registry, key, decoder);
    }
    free(key);
}


typdef void (*DispatchFunction)(const struct Schema *);
//////////////////////////////////////////////////////////////////////////
void pushBack(Vector *vec, DispatchFunction dispatchFunction) {
    if (vec->size >= vec->capacity) {
        size_t new_capacity = (vec->capacity == 0) ? 1 : 2 * vec -> capacity;
        DispatchFunction *new_data = (DispatchFunction*)realloc(vec->data, new_capacity * sizeof(DispatchFunction));
        
        vec -> data = new_data;
        vec-> capactiy = new_capacity;
    }
    vec->data[vec->size++] = dispatchFunction;
}

struct Vector* createVectorRegistry() {
    struct Vector *vec = (struct Vector*)malloc(sizeof(struct Vector));
    if (vec != NULL) {
        vec->data = NULL;
        vec->size = 0;
        vec->capacity = 0;
    }
    return vec;
}

struct SchemaHandlers {
    struct Vector **data; 
    size_t size; 
    size_t capacity;
};

struct SchemaHandlers* createSchemaHandlers() {
    struct SchemaHandlers *handlers = (struct SchemaHandlers*)malloc(sizeof(struct SchemaHandlers));
    if (handlers != NULL) {
        handlers->data = NULL;
        handlers->size = 0;
        handlers->capacity = 0;
    }
    return handlers;
}

void addHandler(struct SchemaHandlers *handlers, const char *key, DispatchFunction dispatchFunction) {
    if (handlers->size >= handlers->capacity) {
        size_t new_capacity = (handlers->capacity == 0) ? 1 : 2 * handlers->capacity;
        struct Vector **new_data = (struct Vector**)realloc(handlers->data, new_capacity * sizeof(struct Vector*));
        if (new_data == NULL) {
            return;
        }
        handlers->data = new_data;
        handlers->capacity = new_capacity;
    }
    struct Vector *vector = createVectorRegistry();
    if (vector != NULL) {
        pushBack(vector, dispatchFunction);
        handlers->data[handlers->size++] = vector;
    }
}
//////////////////////////////////////////////////////////////////////////////////
Registry registerSchemaHandler(char *schemaName, const SerializationType *type, DispatchFunction dispatchFunction) {
    const auto key = createKey(schemaName, type);
    const auto results = schemaHandlers.find(key);
    Vector *results_second = createVectorRegistry();
    if (results != schemaHandlers.end()) {
        results -> push_back(results_second, dispatchFunction);
    } else {
        addHandler(schemaHandlers, key, dispatchFunction);
    }
}

Registry tryDecode(Vector *message, BasicTopicInformation *topicInfo) {
    const auto key = createKey(topicInfo);
    const auto results = decoders.find(key);
    if (results != decoders.end()) {
        return results -> pushBack(message, topicInfo.serilizationType);
    } else {
        return 0;
    }
}

Registry dispatchOnDecode(Vector *message, BasicTopicInformation *topicInfo) {
    Schema schemaMaybe = tryDecode(message, topicInfo);
    if (schemaMaybe != NULL) {
        return 0;
    }
    const auto key = createKey(topicInfo);
    const auto results = SchemaHandlers.find(key);
    Vector *handlers;
    if (results != SchemaHandlers.end()) {
        for (size_t i = 0; i < handlers -> size; ++i) {
            handler(shemaMaybe.value());
        }
    }
}
