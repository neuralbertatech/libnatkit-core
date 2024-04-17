#include <libnatkit-core.hpp>

namespace nat {
namespace core {

    bool JsonDecoder::isSerializationTypeSupported(const SerializationType type) const {
      switch(type) {
        case SerializationType::Json:
          return true;
        default:
          return false;
      }
    }

    // TODO: Change this return type
  Optional<std::shared_ptr<Schema>> JsonDecoder::tryDecode(const std::vector<uint8_t> &message, const SerializationType &type) const {
    switch(type) {
      case SerializationType::Json:
        // TODO: Need a json library
        return {};
        break;
      default:
        return {};
    }
  }

} // namespace nat
} // namespace core
