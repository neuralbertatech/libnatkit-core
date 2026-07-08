#include <libnatkit-core.hpp>
#include <iostream>

namespace nat {
namespace core {

  Optional<std::unique_ptr<BasicTopicInformation>>
  BasicTopicInformation::create(const std::string &kafkaTopicString) {
    const auto splitName = Strings::split(kafkaTopicString, '-');
    if (splitName.size() != 4) {
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

  uint64_t stableStreamId(const std::string &topic_namespace,
                          const std::string &identifier) {
    static const uint64_t kFnvOffsetBasis64 = 14695981039346656037ULL;
    static const uint64_t kFnvPrime64 = 1099511628211ULL;
    static const uint64_t kInt64Max = (1ULL << 63) - 1ULL;

    const std::string key = topic_namespace + ":" + identifier;
    uint64_t digest = kFnvOffsetBasis64;
    for (unsigned char byte : key) {
      digest ^= static_cast<uint64_t>(byte);
      digest *= kFnvPrime64;
    }

    const uint64_t stream_id = digest & kInt64Max;
    return stream_id == 0 ? 1 : stream_id;
  }

} // namespace core
} // namespace nat

namespace {

// Matches the topic-segment grammar ^[A-Za-z0-9][A-Za-z0-9_-]*$ that topics.py
// enforced before this logic moved into the shared library.
bool isValidTopicSegment(const std::string &value) {
  if (value.empty()) {
    return false;
  }
  auto isAlnum = [](char character) {
    return (character >= 'A' && character <= 'Z') ||
           (character >= 'a' && character <= 'z') ||
           (character >= '0' && character <= '9');
  };
  if (!isAlnum(value.front())) {
    return false;
  }
  for (char character : value) {
    if (!isAlnum(character) && character != '_' && character != '-') {
      return false;
    }
  }
  return true;
}

// Shared two-call string-output helper: writes the required size (including the
// trailing NUL) through inout_size, and fills out when it is non-NULL and large
// enough. Returns NAT_OK, NAT_ERR_NULL_ARGUMENT, or NAT_ERR_BUFFER_TOO_SMALL.
int writeStringOutput(const std::string &value, char *out, size_t *inout_size) {
  if (inout_size == nullptr) {
    return NAT_ERR_NULL_ARGUMENT;
  }
  const size_t required = value.size() + 1;
  const size_t provided = *inout_size;
  *inout_size = required;
  if (out == nullptr) {
    return NAT_OK;
  }
  if (provided < required) {
    return NAT_ERR_BUFFER_TOO_SMALL;
  }
  if (!value.empty()) {
    std::memcpy(out, value.data(), value.size());
  }
  out[value.size()] = '\0';
  return NAT_OK;
}

} // namespace

extern "C" int nat_core_v1_stream_id(const char *topic_namespace,
                                     const char *identifier,
                                     uint64_t *out_stream_id) {
  if (topic_namespace == nullptr || identifier == nullptr ||
      out_stream_id == nullptr) {
    return NAT_ERR_NULL_ARGUMENT;
  }
  *out_stream_id = nat::core::stableStreamId(topic_namespace, identifier);
  return NAT_OK;
}

extern "C" int nat_core_v1_topic_build(const char *stream_type,
                                       const char *topic_namespace,
                                       const char *identifier,
                                       const char *serialization,
                                       const char *schema_name,
                                       char *out_topic,
                                       size_t *inout_topic_size) {
  if (stream_type == nullptr || topic_namespace == nullptr ||
      identifier == nullptr || serialization == nullptr ||
      schema_name == nullptr || inout_topic_size == nullptr) {
    return NAT_ERR_NULL_ARGUMENT;
  }

  const std::string identifierString{identifier};
  if (!isValidTopicSegment(identifierString)) {
    return NAT_ERR_INVALID_ARGUMENT;
  }

  try {
    const auto streamTypeMaybe = nat::core::streamTypeFromString(stream_type);
    const auto serializationMaybe =
        nat::core::serializationTypeFromString(serialization);
    if (!streamTypeMaybe.has_value() || !serializationMaybe.has_value()) {
      return NAT_ERR_INVALID_ARGUMENT;
    }

    const uint64_t streamId =
        nat::core::stableStreamId(topic_namespace, identifierString);
    const std::string topic = nat::core::toString(streamTypeMaybe.value()) +
                              "-" + std::to_string(streamId) + "-" +
                              nat::core::toString(serializationMaybe.value()) +
                              "-" + std::string{schema_name};
    return writeStringOutput(topic, out_topic, inout_topic_size);
  } catch (...) {
    return NAT_ERR_INTERNAL;
  }
}

extern "C" int nat_core_v1_topic_parse(const char *topic,
                                       uint64_t *out_stream_id,
                                       char *out_stream_type,
                                       size_t *inout_stream_type_size,
                                       char *out_serialization,
                                       size_t *inout_serialization_size,
                                       char *out_schema_name,
                                       size_t *inout_schema_name_size) {
  if (topic == nullptr) {
    return NAT_ERR_NULL_ARGUMENT;
  }

  std::string typeString;
  std::string serializationString;
  std::string schemaString;
  uint64_t streamId = 0;
  try {
    auto topicInfoMaybe = nat::core::BasicTopicInformation::create(topic);
    if (!topicInfoMaybe.has_value()) {
      return NAT_ERR_INVALID_ARGUMENT;
    }
    const auto &topicInfo = *topicInfoMaybe.value();
    typeString = nat::core::toString(topicInfo.type);
    serializationString = nat::core::toString(topicInfo.serializationType);
    schemaString = topicInfo.schemaName;
    streamId = topicInfo.id;
  } catch (...) {
    return NAT_ERR_INVALID_ARGUMENT;
  }

  if (out_stream_id != nullptr) {
    *out_stream_id = streamId;
  }

  // Sizing call: any NULL string buffer means "just report sizes".
  const bool sizingOnly = out_stream_type == nullptr ||
                          out_serialization == nullptr ||
                          out_schema_name == nullptr;
  if (sizingOnly) {
    if (inout_stream_type_size != nullptr) {
      *inout_stream_type_size = typeString.size() + 1;
    }
    if (inout_serialization_size != nullptr) {
      *inout_serialization_size = serializationString.size() + 1;
    }
    if (inout_schema_name_size != nullptr) {
      *inout_schema_name_size = schemaString.size() + 1;
    }
    return NAT_OK;
  }

  // Fill call: report every required size, then fail if any buffer is short.
  const int typeStatus =
      writeStringOutput(typeString, out_stream_type, inout_stream_type_size);
  const int serializationStatus = writeStringOutput(
      serializationString, out_serialization, inout_serialization_size);
  const int schemaStatus =
      writeStringOutput(schemaString, out_schema_name, inout_schema_name_size);
  if (typeStatus != NAT_OK) {
    return typeStatus;
  }
  if (serializationStatus != NAT_OK) {
    return serializationStatus;
  }
  return schemaStatus;
}
