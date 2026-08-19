#include <libnatkit-core.hpp>


namespace nat {
namespace core {

std::string toString(const StreamType &streamType) {
  switch (streamType) {
  case StreamType::DATA:
    return streamTypeToStringMapping.at(StreamType::DATA);
  case StreamType::META:
    return streamTypeToStringMapping.at(StreamType::META);
  case StreamType::MARKER:
    return streamTypeToStringMapping.at(StreamType::MARKER);
  case StreamType::EXECUTION_COMMAND:
    return streamTypeToStringMapping.at(StreamType::EXECUTION_COMMAND);
  case StreamType::HARDWARE_STATUS:
    return streamTypeToStringMapping.at(StreamType::HARDWARE_STATUS);
  case StreamType::HARDWARE_CONFIGURATION:
    return streamTypeToStringMapping.at(StreamType::HARDWARE_CONFIGURATION);
  case StreamType::LOGGING_LOG:
    return streamTypeToStringMapping.at(StreamType::LOGGING_LOG);
  case StreamType::LOGGING_HEARTBEAT:
    return streamTypeToStringMapping.at(StreamType::LOGGING_HEARTBEAT);
  default:
    assert(0);
  }
}

Optional<StreamType>
streamTypeFromString(const std::string &streamTypeString) {
  const auto lowercaseType = Strings::toLowercase(streamTypeString);
  const auto searchResult = lowercaseStringToStreamTypeMapping.find(lowercaseType);
  if (searchResult != lowercaseStringToStreamTypeMapping.end()) {
    return {searchResult->second};
  } else {
    return {};
  }
}

} // namespace core
} // namespace nat
