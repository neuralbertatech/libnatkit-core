#include <cstdint>
#include <iostream>
#include <optional>
#include <string>

#include <libnatkit-core.hpp>


namespace nat {
namespace core {

  Optional<Stream>
  Stream::createFromKafkaBrokerName(const std::string &brokerName) {
    const auto splitName = Strings::split(brokerName, '-');
    if (splitName.size() != 4) {
      return {};
    }
    const auto streamTypeName = splitName[0];
    const auto streamIdString = splitName[1];
    const auto streamEncoderName = splitName[2];
    const auto streamSchemaName = splitName[3];

    const auto streamTypeMaybe = streamTypeFromString(streamTypeName);
    const auto streamId = std::stoll(streamIdString);

    if (streamTypeMaybe.has_value()) {
      return Stream(brokerName, streamTypeMaybe.value(), streamId,
                    streamEncoderName, streamSchemaName);
    } else {
      return {};
    }
  }


} // namespace core
} // namespace nat
