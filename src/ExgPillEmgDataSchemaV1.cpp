#include <libnatkit-core.hpp>

#ifdef SERVER
#include <nlohmann/json.hpp>
#else
#include <cJSON.h>
#endif

#include <cassert>
#include <iostream>
#include <sstream>

namespace nat {
namespace core {

const std::string ExgPillEmgDataSchemaV1::name = "ExgPillEmgDataSchemaV1";
const std::string ExgPillEmgDataSchemaV1::schemaVersion =
    "exg.pill.emg.data.v1";

ExgPillEmgDataSchemaV1::ExgPillEmgDataSchemaV1()
    : seqNo(0), deviceTsUs(0), sampleRateHz(0), samplesPerChannel(0) {}

ExgPillEmgDataSchemaV1::ExgPillEmgDataSchemaV1(
    const std::string &deviceId,
    uint64_t seqNo,
    uint64_t deviceTsUs,
    uint32_t sampleRateHz,
    const std::vector<std::string> &channelLabels,
    const std::vector<int16_t> &samples,
    uint32_t samplesPerChannel)
    : deviceId(deviceId),
      seqNo(seqNo),
      deviceTsUs(deviceTsUs),
      sampleRateHz(sampleRateHz),
      channelLabels(channelLabels),
      samples(samples),
      samplesPerChannel(samplesPerChannel) {}

#ifdef SERVER
Optional<std::shared_ptr<ExgPillEmgDataSchemaV1>>
ExgPillEmgDataSchemaV1::tryCreateFromSchema(
    const Optional<const std::shared_ptr<Schema>> &messageMaybe) {
  if (!messageMaybe.has_value() || messageMaybe.value() == nullptr) {
    return {};
  }
  if (messageMaybe.value()->getName() == ExgPillEmgDataSchemaV1::name) {
    return std::dynamic_pointer_cast<ExgPillEmgDataSchemaV1>(
        messageMaybe.value());
  }
  return {};
}
#endif

std::unique_ptr<std::vector<uint8_t>>
ExgPillEmgDataSchemaV1::encodeToBytes(const SerializationType &type) const {
  switch (type) {
  case SerializationType::Json: {
#ifdef SERVER
    nlohmann::json root;
    root["schema_version"] = schemaVersion;
    root["device_id"] = deviceId;
    root["seq_no"] = seqNo;
    root["device_ts_us"] = deviceTsUs;
    root["n_channels"] = getChannelCount();
    root["samples_per_channel"] = samplesPerChannel;
    root["sample_rate_hz"] = sampleRateHz;
    root["channel_labels"] = channelLabels;
    root["payload"] = nlohmann::json::array();
    for (uint32_t channelIndex = 0; channelIndex < getChannelCount();
         ++channelIndex) {
      nlohmann::json channel = nlohmann::json::array();
      const size_t offset = static_cast<size_t>(channelIndex) * samplesPerChannel;
      for (uint32_t sampleIndex = 0; sampleIndex < samplesPerChannel;
           ++sampleIndex) {
        channel.push_back(samples[offset + sampleIndex]);
      }
      root["payload"].push_back(channel);
    }
    const auto jsonString = root.dump();
    return nat::core::make_unique<std::vector<uint8_t>>(
        jsonString.begin(), jsonString.end());
#else
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "schema_version", schemaVersion.c_str());
    cJSON_AddStringToObject(root, "device_id", deviceId.c_str());
    cJSON_AddNumberToObject(root, "seq_no", static_cast<double>(seqNo));
    cJSON_AddNumberToObject(root, "device_ts_us", static_cast<double>(deviceTsUs));
    cJSON_AddNumberToObject(root, "n_channels", static_cast<double>(getChannelCount()));
    cJSON_AddNumberToObject(root, "samples_per_channel", static_cast<double>(samplesPerChannel));
    cJSON_AddNumberToObject(root, "sample_rate_hz", static_cast<double>(sampleRateHz));
    cJSON *labels = cJSON_CreateArray();
    for (const auto &label : channelLabels) {
      cJSON_AddItemToArray(labels, cJSON_CreateString(label.c_str()));
    }
    cJSON_AddItemToObject(root, "channel_labels", labels);
    cJSON *payload = cJSON_CreateArray();
    for (uint32_t channelIndex = 0; channelIndex < getChannelCount();
         ++channelIndex) {
      cJSON *channel = cJSON_CreateArray();
      const size_t offset = static_cast<size_t>(channelIndex) * samplesPerChannel;
      for (uint32_t sampleIndex = 0; sampleIndex < samplesPerChannel;
           ++sampleIndex) {
        cJSON_AddItemToArray(
            channel, cJSON_CreateNumber(samples[offset + sampleIndex]));
      }
      cJSON_AddItemToArray(payload, channel);
    }
    cJSON_AddItemToObject(root, "payload", payload);
    const auto jsonString = std::string(cJSON_PrintUnformatted(root));
    cJSON_Delete(root);
    return nat::core::make_unique<std::vector<uint8_t>>(
        jsonString.begin(), jsonString.end());
#endif
  }
  default:
    assert(0);
    return nat::core::make_unique<std::vector<uint8_t>>();
  }
}

bool ExgPillEmgDataSchemaV1::isSerializationTypeSupported(
    const SerializationType type) const {
  switch (type) {
  case SerializationType::Json:
    return true;
  default:
    return false;
  }
}

std::string ExgPillEmgDataSchemaV1::getName() const { return name; }

std::string ExgPillEmgDataSchemaV1::toString() const {
  std::ostringstream builder;
  builder << "ExgPillEmgDataSchemaV1{device_id=\"" << deviceId
          << "\", seq_no=" << seqNo << ", device_ts_us=" << deviceTsUs
          << ", sample_rate_hz=" << sampleRateHz
          << ", n_channels=" << getChannelCount()
          << ", samples_per_channel=" << samplesPerChannel << "}";
  return builder.str();
}

const std::string &ExgPillEmgDataSchemaV1::getDeviceId() const {
  return deviceId;
}

uint64_t ExgPillEmgDataSchemaV1::getSeqNo() const { return seqNo; }

uint64_t ExgPillEmgDataSchemaV1::getDeviceTsUs() const { return deviceTsUs; }

uint32_t ExgPillEmgDataSchemaV1::getSampleRateHz() const { return sampleRateHz; }

uint32_t ExgPillEmgDataSchemaV1::getChannelCount() const {
  return static_cast<uint32_t>(channelLabels.size());
}

uint32_t ExgPillEmgDataSchemaV1::getSamplesPerChannel() const {
  return samplesPerChannel;
}

const std::vector<std::string> &ExgPillEmgDataSchemaV1::getChannelLabels() const {
  return channelLabels;
}

const std::vector<int16_t> &ExgPillEmgDataSchemaV1::getSamples() const {
  return samples;
}

Optional<std::unique_ptr<ExgPillEmgDataSchemaV1>>
ExgPillEmgDataSchemaV1::decodeJson(const std::vector<uint8_t> &message) {
  const auto rawJson =
      std::string(reinterpret_cast<const char *>(message.data()), message.size());

#ifdef SERVER
  try {
    const auto root = nlohmann::json::parse(rawJson);
    const auto schemaVersionIt = root.find("schema_version");
    if (schemaVersionIt != root.end()) {
      if (!schemaVersionIt->is_string()) {
        return {};
      }
      if (schemaVersionIt->get<std::string>() != schemaVersion) {
        // The topic schema name is already authoritative in the Kafka path.
        // Accept structurally valid EMG frames here so a stale producer-side
        // schema_version string does not black-hole live viewing.
        std::cerr << "ExgPillEmgDataSchemaV1::decodeJson warning: expected "
                  << schemaVersion << " but received "
                  << schemaVersionIt->get<std::string>()
                  << "; continuing because payload shape matched the topic schema"
                  << std::endl;
      }
    }
    const auto labelsJson = root.at("channel_labels");
    const auto payloadJson = root.at("payload");
    if (!labelsJson.is_array() || !payloadJson.is_array()) {
      return {};
    }

    std::vector<std::string> labels{};
    labels.reserve(labelsJson.size());
    for (const auto &labelJson : labelsJson) {
      if (!labelJson.is_string()) {
        return {};
      }
      labels.push_back(labelJson.get<std::string>());
    }

    const uint32_t declaredChannelCount = root.value("n_channels", 0U);
    const uint32_t declaredSamplesPerChannel =
        root.value("samples_per_channel", 0U);
    if (declaredChannelCount == 0 || declaredSamplesPerChannel == 0 ||
        declaredChannelCount != labels.size() ||
        payloadJson.size() != labels.size()) {
      return {};
    }

    std::vector<int16_t> flattenedSamples{};
    flattenedSamples.reserve(
        static_cast<size_t>(declaredChannelCount) * declaredSamplesPerChannel);
    for (const auto &channelJson : payloadJson) {
      if (!channelJson.is_array() ||
          channelJson.size() != declaredSamplesPerChannel) {
        return {};
      }
      for (const auto &sampleJson : channelJson) {
        if (!sampleJson.is_number_integer()) {
          return {};
        }
        const auto sample = sampleJson.get<int>();
        if (sample < -32768 || sample > 32767) {
          return {};
        }
        flattenedSamples.push_back(static_cast<int16_t>(sample));
      }
    }

    return nat::core::make_unique<ExgPillEmgDataSchemaV1>(
        root.value("device_id", std::string{}),
        root.value("seq_no", static_cast<uint64_t>(0)),
        root.value("device_ts_us", static_cast<uint64_t>(0)),
        root.value("sample_rate_hz", static_cast<uint32_t>(0)),
        labels,
        flattenedSamples,
        declaredSamplesPerChannel);
  } catch (const std::exception &) {
    return {};
  }
#else
  cJSON *root = cJSON_Parse(rawJson.c_str());
  if (root == nullptr) {
    return {};
  }

  cJSON *schemaVersionJson =
      cJSON_GetObjectItemCaseSensitive(root, "schema_version");
  cJSON *deviceIdJson = cJSON_GetObjectItemCaseSensitive(root, "device_id");
  cJSON *seqNoJson = cJSON_GetObjectItemCaseSensitive(root, "seq_no");
  cJSON *deviceTsUsJson = cJSON_GetObjectItemCaseSensitive(root, "device_ts_us");
  cJSON *sampleRateHzJson = cJSON_GetObjectItemCaseSensitive(root, "sample_rate_hz");
  cJSON *labelsJson = cJSON_GetObjectItemCaseSensitive(root, "channel_labels");
  cJSON *payloadJson = cJSON_GetObjectItemCaseSensitive(root, "payload");
  cJSON *nChannelsJson = cJSON_GetObjectItemCaseSensitive(root, "n_channels");
  cJSON *samplesPerChannelJson =
      cJSON_GetObjectItemCaseSensitive(root, "samples_per_channel");

  if ((schemaVersionJson != nullptr &&
       (!cJSON_IsString(schemaVersionJson) ||
        schemaVersionJson->valuestring == nullptr)) ||
      !cJSON_IsString(deviceIdJson) || !cJSON_IsNumber(seqNoJson) ||
      !cJSON_IsNumber(deviceTsUsJson) || !cJSON_IsNumber(sampleRateHzJson) ||
      !cJSON_IsArray(labelsJson) || !cJSON_IsArray(payloadJson) ||
      !cJSON_IsNumber(nChannelsJson) || !cJSON_IsNumber(samplesPerChannelJson)) {
    cJSON_Delete(root);
    return {};
  }

  if (schemaVersionJson != nullptr &&
      std::string(schemaVersionJson->valuestring) != schemaVersion) {
    std::cerr << "ExgPillEmgDataSchemaV1::decodeJson warning: expected "
              << schemaVersion << " but received "
              << schemaVersionJson->valuestring
              << "; continuing because payload shape matched the topic schema"
              << std::endl;
  }

  const uint32_t channelCount = static_cast<uint32_t>(nChannelsJson->valueint);
  const uint32_t sampleCount =
      static_cast<uint32_t>(samplesPerChannelJson->valueint);
  std::vector<std::string> labels{};
  std::vector<int16_t> flattenedSamples{};
  for (cJSON *labelJson = labelsJson->child; labelJson != nullptr;
       labelJson = labelJson->next) {
    if (!cJSON_IsString(labelJson) || labelJson->valuestring == nullptr) {
      cJSON_Delete(root);
      return {};
    }
    labels.push_back(labelJson->valuestring);
  }

  if (labels.size() != channelCount ||
      static_cast<uint32_t>(cJSON_GetArraySize(payloadJson)) != channelCount) {
    cJSON_Delete(root);
    return {};
  }

  flattenedSamples.reserve(static_cast<size_t>(channelCount) * sampleCount);
  for (cJSON *channelJson = payloadJson->child; channelJson != nullptr;
       channelJson = channelJson->next) {
    if (!cJSON_IsArray(channelJson) ||
        static_cast<uint32_t>(cJSON_GetArraySize(channelJson)) != sampleCount) {
      cJSON_Delete(root);
      return {};
    }
    for (cJSON *sampleJson = channelJson->child; sampleJson != nullptr;
         sampleJson = sampleJson->next) {
      if (!cJSON_IsNumber(sampleJson) || sampleJson->valueint < -32768 ||
          sampleJson->valueint > 32767) {
        cJSON_Delete(root);
        return {};
      }
      flattenedSamples.push_back(static_cast<int16_t>(sampleJson->valueint));
    }
  }

  auto decoded = nat::core::make_unique<ExgPillEmgDataSchemaV1>(
      deviceIdJson->valuestring,
      static_cast<uint64_t>(seqNoJson->valuedouble),
      static_cast<uint64_t>(deviceTsUsJson->valuedouble),
      static_cast<uint32_t>(sampleRateHzJson->valuedouble),
      labels,
      flattenedSamples,
      sampleCount);
  cJSON_Delete(root);
  return decoded;
#endif
}

Optional<std::unique_ptr<ExgPillEmgDataSchemaV1>>
ExgPillEmgDataSchemaV1::decodeAll(const std::vector<uint8_t> &message,
                                  const SerializationType &type) {
  switch (type) {
  case SerializationType::Json:
    return decodeJson(message);
  default:
    return {};
  }
}

Optional<std::shared_ptr<Schema>> ExgPillEmgDataSchemaV1::tryDecode(
    const std::vector<uint8_t> &message,
    const SerializationType &type) const {
  return sharedDecodeAll(message, type);
}

void ExgPillEmgDataSchemaV1::registerWithRegistry(Registry &registry) {
  registry.registerDecoder(name, SerializationType::Json, uniqueDecodeAll);
}

Optional<std::shared_ptr<Schema>> ExgPillEmgDataSchemaV1::sharedDecodeAll(
    const std::vector<uint8_t> &message,
    const SerializationType &type) {
  auto decodedMaybe = decodeAll(message, type);
  if (!decodedMaybe.has_value()) {
    return {};
  }

  std::shared_ptr<Schema> shared(std::move(decodedMaybe.value()));
  return shared;
}

Optional<std::unique_ptr<Schema>> ExgPillEmgDataSchemaV1::uniqueDecodeAll(
    const std::vector<uint8_t> &message,
    const SerializationType &type) {
  auto decodedMaybe = decodeAll(message, type);
  if (!decodedMaybe.has_value()) {
    return {};
  }

  std::unique_ptr<Schema> schema(std::move(decodedMaybe.value()));
  return Optional<std::unique_ptr<Schema>>{std::move(schema)};
}

} // namespace core
} // namespace nat

namespace {

// Two-call byte-buffer output helper (see libnatkit-core-abi.h). Kept local to
// this translation unit, matching MarkerEventV1.cpp.
int writeEmgBytesOutput(const std::vector<uint8_t> &value, uint8_t *out,
                        size_t *inout_size) {
  if (inout_size == nullptr) {
    return NAT_ERR_NULL_ARGUMENT;
  }
  const size_t required = value.size();
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
  return NAT_OK;
}

int emgFrameCanonicalizeJson(const uint8_t *input, size_t input_size,
                             uint8_t *out, size_t *inout_size) {
  if (input == nullptr || inout_size == nullptr) {
    return NAT_ERR_NULL_ARGUMENT;
  }
  try {
    const std::vector<uint8_t> message(input, input + input_size);
    auto recordMaybe = nat::core::ExgPillEmgDataSchemaV1::decodeJson(message);
    if (!recordMaybe.has_value()) {
      return NAT_ERR_DECODE_FAILED;
    }
    auto encoded =
        recordMaybe.value()->encodeToBytes(nat::core::SerializationType::Json);
    if (!encoded) {
      return NAT_ERR_ENCODE_FAILED;
    }
    return writeEmgBytesOutput(*encoded, out, inout_size);
  } catch (...) {
    return NAT_ERR_INTERNAL;
  }
}

} // namespace

extern "C" int nat_core_v1_emg_frame_encode_json(const uint8_t *payload_json,
                                                 size_t payload_json_size,
                                                 uint8_t *out_message,
                                                 size_t *inout_message_size) {
  return emgFrameCanonicalizeJson(payload_json, payload_json_size, out_message,
                                  inout_message_size);
}

extern "C" int nat_core_v1_emg_frame_decode_json(
    const uint8_t *message, size_t message_size, uint8_t *out_payload_json,
    size_t *inout_payload_json_size) {
  return emgFrameCanonicalizeJson(message, message_size, out_payload_json,
                                  inout_payload_json_size);
}
