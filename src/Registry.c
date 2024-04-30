#include <libnatkit-core.h>
#include <stdio.h>

const char* createKey(const char* schemaName, const SerializationType* type) {
    char buffer[256];
    char* serializedType = toStringSerializationType(type);
    sprintf(buffer, "%s-%s", schemaName, serializedType);
    
    Registry* key = (Registry*)malloc(strlen(buffer) + 1);
    if (key != NULL) {
        strcpy(key, buffer);
    }
    return key;
}

const char* createKeyStreamMessage(const StreamMessage *message) {
    return *createKey(getSchemaName(), getSerializationType());
}

const char* createKeyBasicTopicInfo(const BasicTopicInformation *topicInfo) {
    return *createKey(topicInfo->schemaName, topicInfo->serializationType);
}

Registry* createDefaultInitialRegistry() {
    Registry* registry = (Registry*)malloc(sizeof(Registry));
    if (registry != NULL) {
        resgisterWithRegistry(*registry);
    }
       
    return registry;
}

void registerEncoder(Registry *registry, const char *schemaName, SerializationType *type, Encoder *encoder) {
    char *key = createKey(schemaName, type);
    if (key != NULL) {
        insertEncoder(registry, key, encoder);
    } else {
        updateEncoder(registry, key, encoder);
    }
    free(key);
}

void registerDecoder(Registry *registry, const char *schemaName, SerializationType *type, decoder_t *decoder) {
    char *key = createKey(schemaName, type);
    if (key != NULL) {
        insertDecoder(registry, key, decoder);
    } else {
        updateDecoder(registry, key, decoder);
    }
    free(key);
}

typedef void (*DispatchFunction);

//////////////////////////////////////////////////////////////////////////
void pushBack(vector_uint8_t *vec, DispatchFunction dispatchFunction) {
    if (vec->size >= vec->capacity) {
        size_t new_capacity = (vec->capacity == 0) ? 1 : 2 * vec -> capacity;
        DispatchFunction *new_data = (DispatchFunction*)realloc(vec->data, new_capacity * sizeof(DispatchFunction));
        
        vec -> data = new_data;
        vec-> capacity = new_capacity;
    }
    vec->data[vec->size++] = dispatchFunction;
}

struct Vector* createVectorRegistry() {
    vector_uint8_t* vec = (struct Vector*)malloc(sizeof(Vector));
    if (vec != NULL) {
        vec->data = NULL;
        vec->size = 0;
        vec->capacity = 0;
    }
    return vec;
}

typedef struct{
    struct Vector **data; 
    size_t size; 
    size_t capacity;
    SchemaHandler* handlers;
} SchemaHandlers;

SchemaHandlers *schemaHandlers;

struct SchemaHandlers* createSchemaHandlers() {
    SchemaHandlers *handlers = (struct SchemaHandlers*)malloc(sizeof(SchemaHandlers));
    if (handlers != NULL) {
        handlers->data = NULL;
        handlers->size = 0;
        handlers->capacity = 0;
    }
    return handlers;
}

void addHandler(SchemaHandlers *handlers, const char *key, DispatchFunction dispatchFunction) {
    if (handlers->size >= handlers->capacity) {
        size_t new_capacity = (handlers->capacity == 0) ? 1 : 2 * handlers->capacity;
        struct Vector **new_data = (struct Vector**)realloc(handlers->data, new_capacity * sizeof(struct Vector*));
        if (new_data == NULL) {
            return;
        }
        handlers->data = new_data;
        handlers->capacity = new_capacity;
    }
    Vector *vector = createVectorRegistry();
    if (vector != NULL) {
        pushBack(vector, dispatchFunction);
        handlers->data[handlers->size++] = vector;
    }
}

typedef struct {
    DispatchFunction **data;
    size_t size;
    size_t capacity;
} FunctionVector;

FunctionVector* findElement(const char* key) {
    for (size_t i = 0; i < schemaHandlers -> size; i++) {
        if (strcmp(schemaHandlers->data[i], key) == 0) {
            return &schemaHandlers->data[i];
        }
    }
    return NULL;
}

typedef void (*SchemaHandler)(const Schema*);


//////////////////////////////////////////////////////////////////////////////////

void registerSchemaHandler(const char* schemaName, const SerializationType* type, DispatchFunction dispatchFunction) {
    const char* key = createKey(schemaName, type);
    Vector* results = findElement(key);
    if (results != NULL) {
        pushBack(results, dispatchFunction);
    } else {
        addHandler(schemaHandlers, key, dispatchFunction);
    }
}


void dispatchOnDecode(const uint8_t* message, size_t messageSize, const BasicTopicInformation* topicInfo) {
    const Schema* schemaMaybe = tryDecode(message, topicInfo->serializationType);

    if (schemaMaybe == NULL) {
        return;
    }

    const char* key = createKey(topicInfo->schemaName, topicInfo->serializationType);
    SchemaHandlers* result = findElement(key);

    for (int i = 0; i < result -> size; i++) {
        SchemaHandler handler = result->handlers[i];
        handler(schemaMaybe);
    }
}
