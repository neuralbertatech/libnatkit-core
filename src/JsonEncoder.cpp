#include <libnatkit-core.hpp>

namespace nat::core {

    bool JsonEncoder::isSerializationTypeSupported(const SerializationType type) {
      switch(type) {
        case SerializationType::Json:
          return true;
        default:
          return false;
      }
    }

    std::vector<uint8_t> JsonEncoder::encode(const StreamMessage& message) {
      return {};
    }

}
