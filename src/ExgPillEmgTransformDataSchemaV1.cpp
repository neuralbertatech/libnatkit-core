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

const std::string ExgPillEmgTransformDataSchemaV1::name =
    "ExgPillEmgTransformDataSchemaV1";
const std::string ExgPillEmgTransformDataSchemaV1::schemaVersion =
    "exg.pill.emg.transform.data.v1";

ExgPillEmgTransformDataSchemaV1::ExgPillEmgTransformDataSchemaV1()
    : seqNo(0), deviceTsUs(0), sampleRateHz(0), samplesPerChannel(0) {}

ExgPillEmgTransformDataSchemaV1::ExgPillEmgTransformDataSchemaV1(
    const std::string &deviceId,
    uint64_t seqNo,
    uint64_t deviceTsUs,
    uint32_t sampleRateHz,
    const std::vector<std::string> &channelLabels,
    const std::vector<float> &samples,
    uint32_t samplesPerChannel)
    : deviceId(deviceId),
      seqNo(seqNo),
      deviceTsUs(deviceTsUs),
      sampleRateHz(sampleRateHz),
      channelLabels(channelLabels),
      samples(samples),
      samplesPerChannel(samplesPerChannel) {}

#ifdef SERVER
Optional<std::shared_ptr<ExgPillEmgTransformDataSchemaV1>>
ExgPillEmgTransformDataSchemaV1::tryCreateFromSchema(
    const Optional<const std::shared_ptr<Schema>> &messageMaybe) {
  if (!messageMaybe.has_value() || messageMaybe.value() == nullptr) {
    return {};
  }
  if (messageMaybe.value()->getName() ==
      ExgPillEmgTransformDataSchemaV1::name) {
    return std::dynamic_pointer_cast<ExgPillEmgTransformDataSchemaV1>(
        messageMaybe.value());
  }
  return {};
}
#endif

std::unique_ptr<std::vector<uint8_t>>
ExgPillEmgTransformDataSchemaV1::encodeToBytes(
    const SerializationType &type) const {
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
      const size_t offset =
          static_cast<size_t>(channelIndex) * samplesPerChannel;
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
      const size_t offset =
          static_cast<size_t>(channelIndex) * samplesPerChannel;
      for (uint32_t sampleIndex = 0; sampleIndex < samplesPerChannel;
           ++sampleIndex) {
        cJSON_AddItemToArray(
            channel, cJSON_CreateNumber(static_cast<double>(samples[offset + sampleIndex])));
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

bool ExgPillEmgTransformDataSchemaV1::isSerializationTypeSupported(
    const SerializationType type) const {
  switch (type) {
  case SerializationType::Json:
    return true;
  default:
    return false;
  }
}

std::string ExgPillEmgTransformDataSchemaV1::getName() const { return name; }

std::string ExgPillEmgTransformDataSchemaV1::toString() const {
  std::ostringstream builder;
  builder << "ExgPillEmgTransformDataSchemaV1{device_id=\"" << deviceId
          << "\", seq_no=" << seqNo << ", device_ts_us=" << deviceTsUs
          << ", sample_rate_hz=" << sampleRateHz
          << ", n_channels=" << getChannelCount()
          << ", samples_per_channel=" << samplesPerChannel << "}";
  return builder.str();
}

const std::string &ExgPillEmgTransformDataSchemaV1::getDeviceId() const {
  return deviceId;
}

uint64_t ExgPillEmgTransformDataSchemaV1::getSeqNo() const { return seqNo; }

uint64_t ExgPillEmgTransformDataSchemaV1::getDeviceTsUs() const {
  return deviceTsUs;
}

uint32_t ExgPillEmgTransformDataSchemaV1::getSampleRateHz() const {
  return sampleRateHz;
}

uint32_t ExgPillEmgTransformDataSchemaV1::getChannelCount() const {
  return static_cast<uint32_t>(channelLabels.size());
}

uint32_t ExgPillEmgTransformDataSchemaV1::getSamplesPerChannel() const {
  return samplesPerChannel;
}

const std::vector<std::string> &
ExgPillEmgTransformDataSchemaV1::getChannelLabels() const {
  return channelLabels;
}

const std::vector<float> &ExgPillEmgTransformDataSchemaV1::getSamples() const {
  return samples;
}

Optional<std::unique_ptr<ExgPillEmgTransformDataSchemaV1>>
ExgPillEmgTransformDataSchemaV1::decodeJson(
    const std::vector<uint8_t> &message) {
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
        std::cerr
            << "ExgPillEmgTransformDataSchemaV1::decodeJson warning: expected "
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

    std::vector<float> flattenedSamples{};
    flattenedSamples.reserve(
        static_cast<size_t>(declaredChannelCount) * declaredSamplesPerChannel);
    for (const auto &channelJson : payloadJson) {
      if (!channelJson.is_array() ||
          channelJson.size() != declaredSamplesPerChannel) {
        return {};
      }
      for (const auto &sampleJson : channelJson) {
        if (!sampleJson.is_number()) {
          return {};
        }
        flattenedSamples.push_back(sampleJson.get<float>());
      }
    }

    return nat::core::make_unique<ExgPillEmgTransformDataSchemaV1>(
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
  cJSON *sampleRateHzJson =
      cJSON_GetObjectItemCaseSensitive(root, "sample_rate_hz");
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
    std::cerr
        << "ExgPillEmgTransformDataSchemaV1::decodeJson warning: expected "
        << schemaVersion << " but received " << schemaVersionJson->valuestring
        << "; continuing because payload shape matched the topic schema"
        << std::endl;
  }

  const uint32_t channelCount = static_cast<uint32_t>(nChannelsJson->valueint);
  const uint32_t sampleCount =
      static_cast<uint32_t>(samplesPerChannelJson->valueint);
  std::vector<std::string> labels{};
  std::vector<float> flattenedSamples{};
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
      if (!cJSON_IsNumber(sampleJson)) {
        cJSON_Delete(root);
        return {};
      }
      flattenedSamples.push_back(static_cast<float>(sampleJson->valuedouble));
    }
  }

  auto decoded = nat::core::make_unique<ExgPillEmgTransformDataSchemaV1>(
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

Optional<std::unique_ptr<ExgPillEmgTransformDataSchemaV1>>
ExgPillEmgTransformDataSchemaV1::decodeAll(
    const std::vector<uint8_t> &message,
    const SerializationType &type) {
  switch (type) {
  case SerializationType::Json:
    return decodeJson(message);
  default:
    return {};
  }
}

Optional<std::shared_ptr<Schema>> ExgPillEmgTransformDataSchemaV1::tryDecode(
    const std::vector<uint8_t> &message,
    const SerializationType &type) const {
  return sharedDecodeAll(message, type);
}

void ExgPillEmgTransformDataSchemaV1::registerWithRegistry(Registry &registry) {
  registry.registerDecoder(name, SerializationType::Json, uniqueDecodeAll);
}

Optional<std::shared_ptr<Schema>>
ExgPillEmgTransformDataSchemaV1::sharedDecodeAll(
    const std::vector<uint8_t> &message,
    const SerializationType &type) {
  auto decodedMaybe = decodeAll(message, type);
  if (!decodedMaybe.has_value()) {
    return {};
  }

  std::shared_ptr<Schema> shared(std::move(decodedMaybe.value()));
  return shared;
}

Optional<std::unique_ptr<Schema>>
ExgPillEmgTransformDataSchemaV1::uniqueDecodeAll(
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
