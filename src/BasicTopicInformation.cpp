#include <libnatkit-core.hpp>
#include <iostream>

namespace nat::core {

  std::optional<std::unique_ptr<BasicTopicInformation>>
  BasicTopicInformation::create(const std::string &kafkaTopicString) {
    const auto splitName = Strings::split(kafkaTopicString, '-');
    if (std::ssize(splitName) != 4) {
      std::cerr << "Topic String does not contain the four parts\n";
      return {};
    }
    const auto streamTypeName = splitName[0];
    const auto streamIdString = splitName[1];
    const auto streamEncoderName = splitName[2];
    const auto streamSchemaName = splitName[3];

    const auto streamTypeMaybe = streamTypeFromString(streamTypeName);
    const auto serializationTypeMaybe =
        serializationTypeFromString(streamEncoderName);
    const auto streamId = std::stoll(streamIdString);

    if (!streamTypeMaybe.has_value()) {
      std::cerr << "\"" << streamTypeName << "\" is not a valid stream type\n";
      return {};
    }
    if (!serializationTypeMaybe.has_value()) {
      std::cerr << "\"" << streamEncoderName << "\" is not a valid serialization type\n";
      return {};
    }

    return std::unique_ptr<BasicTopicInformation>(new BasicTopicInformation(
        streamTypeMaybe.value(), serializationTypeMaybe.value(), streamId,
        streamSchemaName));
  }

  std::string BasicTopicInformation::toString() const {
    return "BasicTopicInformation: {type=\"" + ::nat::core::toString(type) +
           "\", id=" + std::to_string(id) + ", serializationType=\"" +
           ::nat::core::toString(serializationType) + "\", schemaName=\"" +
           schemaName + "\"}";
  }

  std::string BasicTopicInformation::toTopicString() const {
    return ::nat::core::toString(type) + "-" + std::to_string(id) + "-" +
           ::nat::core::toString(serializationType) + "-" + schemaName;
  }

} // namespace nat::core
