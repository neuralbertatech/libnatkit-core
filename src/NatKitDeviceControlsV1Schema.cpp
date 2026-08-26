#include <algorithm>
#include <libnatkit-core.hpp>
#include <sstream>

#ifdef SERVER
#include <nlohmann/json.hpp>
#endif

namespace nat {
namespace core {

const std::string NatKitDeviceControlsV1Schema::name =
    "NatKitDeviceControlsV1";
const std::string NatKitDeviceControlsV1Schema::schemaVersionTag =
    "nat.controls.v1";

bool NatKitDeviceControlsV1Schema::isSerializationTypeSupported(
    const SerializationType type) const {
  return type == SerializationType::Json;
}

std::string NatKitDeviceControlsV1Schema::getName() const { return name; }

std::vector<std::string> NatKitDeviceControlsV1Schema::allowedCommands() const {
  std::vector<std::string> out;
  const auto add = [&out](const std::string &command) {
    if (command.empty()) return;
    if (std::find(out.begin(), out.end(), command) != out.end()) return;
    out.push_back(command);
  };
  for (const auto &control : controls) {
    add(control.writeCommand);
    // ⚠️ The read as well. Permitting only the write would let the UI offer a
    // "Read from device" the server then refuses -- a control that silently
    // fails, which is the failure mode this whole design exists to remove.
    add(control.readCommand);
  }
  std::sort(out.begin(), out.end());
  return out;
}

std::string NatKitDeviceControlsV1Schema::toString() const {
  std::ostringstream out;
  out << name << ": {device=" << deviceId;
  if (advertiserDeviceId != 0 && advertiserDeviceId != deviceId) {
    out << ", advertised_by=" << advertiserDeviceId;
  }
  out << ", controls=" << controls.size() << "}";
  return out.str();
}

#ifdef SERVER

std::string NatKitDeviceControlsV1Schema::toJson() const {
  nlohmann::json doc;
  doc["schema_version"] = schemaVersionTag;
  doc["device_id"] = deviceId;
  if (advertiserDeviceId != 0) {
    doc["advertiser_device_id"] = advertiserDeviceId;
  }
  nlohmann::json list = nlohmann::json::array();
  for (const auto &control : controls) {
    nlohmann::json entry;
    entry["id"] = control.controlId;
    entry["kind"] = control.kind;
    entry["write"] = control.writeCommand;
    // Written only when they carry meaning, so the wire stays small -- this
    // document rides a radio packet with a 1470-byte ceiling.
    if (!control.readCommand.empty()) entry["read"] = control.readCommand;
    if (!control.group.empty()) entry["group"] = control.group;
    if (!control.field.empty()) entry["field"] = control.field;
    if (!control.valueType.empty()) entry["type"] = control.valueType;
    if (!control.label.empty()) entry["label"] = control.label;
    if (!control.description.empty()) entry["description"] = control.description;
    if (!control.unit.empty()) entry["unit"] = control.unit;
    if (control.ranged) {
      entry["min"] = control.minValue;
      entry["max"] = control.maxValue;
    }
    list.push_back(std::move(entry));
  }
  doc["controls"] = std::move(list);
  return doc.dump();
}

std::unique_ptr<message_t> NatKitDeviceControlsV1Schema::encodeToBytes(
    const SerializationType &type) const {
  if (type != SerializationType::Json) {
    return nullptr;
  }
  const auto text = toJson();
  return nat::core::make_unique<message_t>(std::begin(text), std::end(text));
}

Optional<NatKitDeviceControlsV1Schema>
NatKitDeviceControlsV1Schema::decodeJson(const std::vector<uint8_t> &message) {
  const std::string text(std::begin(message), std::end(message));
  const auto doc = nlohmann::json::parse(text, nullptr, false);
  if (doc.is_discarded() || !doc.is_object()) {
    return {};
  }
  // ⚠️ The version tag is checked, not assumed. This channel is deliberately
  // open to third-party record types, so "some JSON arrived on a Configuration
  // topic" does not make it a controls advertisement.
  const auto version = doc.value("schema_version", std::string{});
  if (version != schemaVersionTag) {
    return {};
  }
  if (!doc.contains("controls") || !doc["controls"].is_array()) {
    return {};
  }

  NatKitDeviceControlsV1Schema record;
  record.deviceId = doc.value("device_id", static_cast<uint64_t>(0));
  record.advertiserDeviceId =
      doc.value("advertiser_device_id", record.deviceId);

  for (const auto &entry : doc["controls"]) {
    if (!entry.is_object()) continue;
    DeviceControlAdvertisement control;
    control.controlId = entry.value("id", std::string{});
    control.kind = entry.value("kind", std::string{});
    control.writeCommand = entry.value("write", std::string{});
    control.readCommand = entry.value("read", std::string{});
    control.group = entry.value("group", std::string{});
    control.field = entry.value("field", std::string{});
    control.valueType = entry.value("type", std::string{});
    control.label = entry.value("label", std::string{});
    control.description = entry.value("description", std::string{});
    control.unit = entry.value("unit", std::string{});
    if (entry.contains("min") && entry.contains("max")) {
      control.ranged = true;
      control.minValue = entry.value("min", 0.0);
      control.maxValue = entry.value("max", 0.0);
    }
    // ⚠️ A control with no id or no write command is DROPPED rather than kept.
    // Keeping it would put a button on screen that can never do anything, and
    // would widen the allowlist with an empty string.
    if (control.controlId.empty() || control.writeCommand.empty()) {
      continue;
    }
    // An unknown kind is dropped for the same reason: the UI has no way to draw
    // it, and guessing "button" would make a toggle act as a one-shot.
    if (!deviceControlKindFromString(control.kind).has_value()) {
      continue;
    }
    record.controls.push_back(std::move(control));
  }
  return record;
}

#else  // !SERVER

// ⚠️ NOT IMPLEMENTED FOR THE EMBEDDED BUILD, and deliberately absent rather than
// stubbed to return something plausible. The ESP-IDF firmware does not link this
// library at all today (TEC-NATKIT-101), so nothing calls these; when it does,
// the cJSON path goes here alongside the nlohmann one, the way
// BasicMetaInfoSchema already carries both.

std::string NatKitDeviceControlsV1Schema::toJson() const { return std::string{}; }

std::unique_ptr<message_t> NatKitDeviceControlsV1Schema::encodeToBytes(
    const SerializationType &) const {
  return nullptr;
}

Optional<NatKitDeviceControlsV1Schema>
NatKitDeviceControlsV1Schema::decodeJson(const std::vector<uint8_t> &) {
  return {};
}

#endif

Optional<std::shared_ptr<Schema>> NatKitDeviceControlsV1Schema::tryDecode(
    const std::vector<uint8_t> &message, const SerializationType &type) const {
  if (type != SerializationType::Json) {
    return {};
  }
  auto decoded = decodeJson(message);
  if (!decoded.has_value()) {
    return {};
  }
  std::shared_ptr<Schema> shared(
      new NatKitDeviceControlsV1Schema(decoded.value()));
  return Optional<std::shared_ptr<Schema>>{shared};
}

void NatKitDeviceControlsV1Schema::registerWithRegistry(Registry &registry) {
  const decoder_t decoder = [](const message_t &message,
                               const SerializationType &type) {
    if (type != SerializationType::Json) {
      return Optional<std::unique_ptr<Schema>>{};
    }
    auto decoded = decodeJson(message);
    if (!decoded.has_value()) {
      return Optional<std::unique_ptr<Schema>>{};
    }
    std::unique_ptr<Schema> converted(
        new NatKitDeviceControlsV1Schema(decoded.value()));
    return Optional<std::unique_ptr<Schema>>{std::move(converted)};
  };
  registry.registerDecoder(name, SerializationType::Json, decoder);
}

}  // namespace core
}  // namespace nat
