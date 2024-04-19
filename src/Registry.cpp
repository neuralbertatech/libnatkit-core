#include <libnatkit-core.hpp>

namespace nat {
namespace core {

    std::string Registry::createKey(const std::string& schemaName, const SerializationType& type) {
      return schemaName + '-' + toString(type);
    }

    std::string Registry::createKey(const StreamMessage& message) {
      return createKey(message.getSchemaName(), message.getSerializationType());
    }

    std::string Registry::createKey(const BasicTopicInformation& topicInfo) {
      return createKey(topicInfo.schemaName, topicInfo.serializationType);
    }

std::unique_ptr<Registry> Registry::createDefaultInitalizeRegistry() {
  auto registry = nat::core::make_unique<Registry>();
  BasicMetaInfoSchema::registerWithRegistry(*registry);

  return registry;
}

    void Registry::registerEncoder(const std::string& schemaName, const SerializationType& type, const std::shared_ptr<Encoder>& encoder) {
      const auto key = createKey(schemaName, type);
      const auto search = encoders.find(key);
      if (search != encoders.end())
        encoders.emplace(key, encoder);
    }

    void Registry::registerDecoder(const std::string& schemaName, const SerializationType& type, const decoder_t& decoder) {
      const auto key = createKey(schemaName, type);
      const auto search = decoders.find(key);
      if (search != decoders.end())
        decoders.emplace(key, decoder);
    }

    void Registry::registerSchemaHandler(const std::string& schemaName, const SerializationType& type, const std::function<void(const std::shared_ptr<Schema>&)>& dispatchFunction) {
      const auto key = createKey(schemaName, type);
      const auto results = schemaHandlers.find(key);
      if (results != schemaHandlers.end()) {
        results->second.push_back(dispatchFunction);
      } else {
        schemaHandlers.emplace(std::make_pair(key, std::vector<std::function<void(const std::shared_ptr<Schema>&)>>{dispatchFunction}));
      }
    }
    
    Optional<std::unique_ptr<Schema>> Registry::tryDecode(const std::vector<uint8_t>& message, const BasicTopicInformation& topicInfo) const {
      const auto key = createKey(topicInfo);
      const auto results = decoders.find(key);
     if (results != decoders.end()) {
       return results->second(message, topicInfo.serializationType);
     } else {
        return {};
     }
    }

    void Registry::dispatchOnDecode(const std::vector<uint8_t>& message, const BasicTopicInformation& topicInfo) {
      Optional<std::unique_ptr<Schema>> schemaMaybe = tryDecode(message, topicInfo);
      if (!schemaMaybe.has_value()) {
        return;
      }

      std::shared_ptr<Schema> schema = std::move(schemaMaybe.value());
      const auto key = createKey(topicInfo);
      const auto results = schemaHandlers.find(key);
      if (results != schemaHandlers.end()) {
          for (const auto& handler : results->second) {
            handler(schema);
          }
      }
    }

} // namespace core
} // namespace nat
