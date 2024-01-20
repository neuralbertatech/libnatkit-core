#include <libnatkit-core.hpp>
#include <iostream>

namespace nat::core {

// TODO: DELETEME
SerializationType getSerializationType(const std::string& encoderName) {
  const auto lowercaseEncoderName = Strings::toLowercase(encoderName);
  if (lowercaseEncoderName == "json") {
    return SerializationType::Json;
  }

  std::cout << "Fatal Error: Invalid serialization type '" << encoderName << "'" << std::endl;
  assert(0);
}

std::string toString(const SerializationType& serializationType) {
  switch (serializationType) {
    case SerializationType::Json:
      return serializationTypeToStringMapping.at(SerializationType::Json);
    default:
      assert(0);
  }
}

std::optional<SerializationType>
serializationTypeFromString(const std::string &serializationTypeString) {
  const auto lowercaseType = Strings::toLowercase(serializationTypeString);
  if (lowercaseStringToSerializationTypeMapping.contains(lowercaseType)) {
    return lowercaseStringToSerializationTypeMapping.at(lowercaseType);
  } else {
    return {};
  }
}

}
