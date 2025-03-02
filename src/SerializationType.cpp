#include <libnatkit-core.hpp>
#include <iostream>

namespace nat {
namespace core {

// TODO: DELETEME
SerializationType getSerializationType(const std::string& encoderName) {
  const auto lowercaseEncoderName = Strings::toLowercase(encoderName);
  if (lowercaseEncoderName == "json") {
    return SerializationType::Json;
  }
  else if (lowercaseEncoderName == "csv") {
      return SerializationType::Csv;
  } else if (lowercaseEncoderName == "binary") {
    return SerializationType::Binary;
  }

  std::cout << "Fatal Error: Invalid serialization type '" << encoderName << "'" << std::endl;
  assert(0);
}

std::string toString(const SerializationType& serializationType) {
  switch (serializationType) {
    case SerializationType::Json:
      return serializationTypeToStringMapping.at(SerializationType::Json);
    case SerializationType::Csv:
        return serializationTypeToStringMapping.at(SerializationType::Csv);
    case SerializationType::Binary:
      return serializationTypeToStringMapping.at(SerializationType::Binary);
    default:
      assert(0);
  }
}

Optional<SerializationType>
serializationTypeFromString(const std::string &serializationTypeString) {
  const auto lowercaseType = Strings::toLowercase(serializationTypeString);
  auto searchResult = lowercaseStringToSerializationTypeMapping.find(lowercaseType);
  if (searchResult != lowercaseStringToSerializationTypeMapping.end()) {
    return Optional<SerializationType>{searchResult->second};
  } else {
    return {};
  }
}

} // namespace core
} // namespace nat
