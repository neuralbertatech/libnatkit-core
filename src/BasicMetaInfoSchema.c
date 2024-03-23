#include <libnatkit-core.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef SERVER
#include <nlohmann/json.hpp>
#else
#include "cJSON.h"
#endif

typedef uint8_t* uint8_ptr;
BasicMetaInfoSchema* encodeToBytes(const SerializationType *type) {
    switch (type) {
        case Json:
            #ifdef SERVER
               json j;
               j["name"] = streamName;
               const auto jsonStr = j.dump();
               
               uint8_ptr make_unique_vector(const char* jsonStr, size_t length) {
                uint8_ptr vector = (uint8_ptr)malloc(length * size_t(uint8_t));
                if (vector == NULL) {
                    return NULL;
                }
                memcpy(vector, jsonStr, length);
                return vector;
               }
            #else
                cJSON *jsonObject = cJSON_CreateObject();
                cJSON_AddStringToObject(jsonObject, "name", streamName);
                
                Vector* cJSON_toVector(cJSON* jsonObject) {
                    char* jsonString = cJSON_Print(jsonObject);
                    if (jsonString == NULL) {
                        return NULL;
                    }
                    size_t length = strlen(jsonString);
                    Vector* vector = (Vector*)malloc(sizeof(Vector));
                    if (vector == NULL) {
                        free(jsonString);
                        return NULL;
                    }
                    vector -> data = (uint8_t*)malloc((length + 1) * sizeof(uint8_t));
                    if (vector -> data == NULL) {
                        free(jsonString);
                        free(vector);
                        return NULL;
                    }
                    memcpy(vector->data, jsonString, length);
                    vector -> data[length] = '\0';
                    vector -> size = length;

                    free(jsonString);
                    return vector;
                }
            #endif
        
        assert(0);
    }

    bool BasicMetaInfoSchema isSerializationTypeSupported(const SerializationType type) {
        switch (type) {
            case Json:
                return 1;
            default:
                assert(0);
        }
    }

    BasicMetaInfoSchema toString() {
        const char* name = getName();
        char* jsonStr = concat_strings(name, ": {\"name\": ", streamName);
        if (jsonStr == NULL) {
            return NULL;
        }
        strcat(jsonStr, "}");
        return jsonStr;
    }

    Optional decodeJson(const uint8_t *message, size_t message_size) {
        char *jsonStr(const uint8_t *begin, const uint8_t *end);

        #ifdef SERVER
            const auto json = parse(jsonStr);
            return createBasicMetaInfoSchema(json["name"]);

        #else
            cJSON *json = cJson_Parse(char *jsonStr);
            cJSON *name = CJSON_GetObjectItemCaseSensitive(json, "name");
            return createBasicMetaInfoSchema(name -> valuestring);
        #endif
    }

    Optional decodeAll(const uint8_t *message, const SerializationType *type) {
        switch(type) {
            case Json:
                return decodeJson(message);
            default:
                assert(0);
        }
    }

    typedef void (*DispatchMethod)(const Schema *);

    BasicMetaInfoSchema decodeAndDispatch(uint8_t *message, SerializationType *type, DispatchMethod dispatchMethod) {
        Optional decodedMessageMaybe = decodeAll(message, type);
        if (decodedMessageMaybe != NULL) {
            DispatchMethod(decodedMessageMaybe.value());
        } 
    }

    BasicMetaInfoSchema tryDecode(uint8_t *message, SerializationType *type) {
        return decodeAll(message, type);
    }

    BasicMetaInfoSchema registerWithRegistry(Registry *registry) {
        registry.registerDecoder(name, Json, decodeAll);
    }

    BasicMetaInfoSchema getName() {return name;}

    BasicMetaInfoSchema getStreamName() {return streamName;}

}

void free_uint8_ptr_vector(uint8_ptr* vector) {
    free(vector);
}

void free_Vector_vector(Vector* vector) {
    if (vector != NULL) {
        free(vector -> data);
        free(vector);
    }
}

