#include <algorithm>
#include <libnatkit-core.hpp>

namespace nat {
namespace core {

namespace {

const std::unordered_map<DeviceControlKind, std::string> kKindNames = {
    {DeviceControlKind::Button, "button"},
    {DeviceControlKind::Toggle, "toggle"},
    {DeviceControlKind::Input, "input"},
};

std::shared_ptr<const DeviceControlDescriptor> make(
    const std::string &controlId, const std::string &label,
    DeviceControlKind kind, const std::string &writeCommand,
    const std::string &readCommand = std::string{},
    const std::string &field = std::string{},
    FieldValueType valueType = FieldValueType::Bool,
    const std::string &description = std::string{},
    const std::string &unit = std::string{},
    const std::string &group = std::string{}, bool ranged = false,
    double minValue = 0.0, double maxValue = 0.0) {
  return std::make_shared<const DeviceControlDescriptor>(
      controlId, label, kind, writeCommand, readCommand, field, valueType,
      description, unit, group, ranged, minValue, maxValue);
}

}  // namespace

std::string toString(const DeviceControlKind kind) {
  const auto found = kKindNames.find(kind);
  return found == kKindNames.end() ? std::string{} : found->second;
}

Optional<DeviceControlKind> deviceControlKindFromString(
    const std::string &name) {
  for (const auto &entry : kKindNames) {
    if (entry.second == name) {
      return entry.first;
    }
  }
  return {};
}

DeviceControlDescriptor::DeviceControlDescriptor()
    : kind(DeviceControlKind::Button),
      valueType(FieldValueType::Bool),
      ranged(false),
      minValue(0.0),
      maxValue(0.0) {}

DeviceControlDescriptor::DeviceControlDescriptor(
    const std::string &controlId, const std::string &label,
    const DeviceControlKind kind, const std::string &writeCommand,
    const std::string &readCommand, const std::string &field,
    const FieldValueType valueType, const std::string &description,
    const std::string &unit, const std::string &group, const bool ranged,
    const double minValue, const double maxValue,
    const std::vector<std::string> &enumValues)
    : controlId(controlId),
      label(label),
      description(description),
      unit(unit),
      kind(kind),
      valueType(valueType),
      group(group),
      readCommand(readCommand),
      writeCommand(writeCommand),
      field(field),
      ranged(ranged),
      minValue(minValue),
      maxValue(maxValue),
      enumValues(enumValues) {}

const std::string &DeviceControlDescriptor::getControlId() const { return controlId; }
const std::string &DeviceControlDescriptor::getLabel() const { return label; }
const std::string &DeviceControlDescriptor::getDescription() const { return description; }
const std::string &DeviceControlDescriptor::getUnit() const { return unit; }
DeviceControlKind DeviceControlDescriptor::getKind() const { return kind; }
FieldValueType DeviceControlDescriptor::getValueType() const { return valueType; }
const std::string &DeviceControlDescriptor::getGroup() const { return group; }
const std::string &DeviceControlDescriptor::getReadCommand() const { return readCommand; }
const std::string &DeviceControlDescriptor::getWriteCommand() const { return writeCommand; }
const std::string &DeviceControlDescriptor::getField() const { return field; }
bool DeviceControlDescriptor::isRanged() const { return ranged; }
double DeviceControlDescriptor::getMinValue() const { return minValue; }
double DeviceControlDescriptor::getMaxValue() const { return maxValue; }
const std::vector<std::string> &DeviceControlDescriptor::getEnumValues() const {
  return enumValues;
}

std::vector<std::string> DeviceControlDescriptor::commands() const {
  std::vector<std::string> out;
  if (!writeCommand.empty()) {
    out.push_back(writeCommand);
  }
  // ⚠️ The read command counts too. Gating only on the write would let the UI
  // offer a "Read from device" that the server then refuses, which is the exact
  // shape of control-that-silently-fails this design exists to remove.
  if (!readCommand.empty() && readCommand != writeCommand) {
    out.push_back(readCommand);
  }
  return out;
}

bool DeviceControlDescriptorRegistry::registerDescriptor(
    const std::shared_ptr<const DeviceControlDescriptor> &descriptor) {
  if (!descriptor || descriptor->getControlId().empty()) {
    return false;
  }
  const auto controlId = descriptor->getControlId();
  const bool replaced = descriptorsByControlId.count(controlId) > 0;
  descriptorsByControlId[controlId] = descriptor;
  return replaced;
}

Optional<std::shared_ptr<const DeviceControlDescriptor>>
DeviceControlDescriptorRegistry::findByControlId(
    const std::string &controlId) const {
  const auto found = descriptorsByControlId.find(controlId);
  if (found == descriptorsByControlId.end()) {
    return {};
  }
  return found->second;
}

std::vector<std::string> DeviceControlDescriptorRegistry::registeredControlIds()
    const {
  std::vector<std::string> ids;
  ids.reserve(descriptorsByControlId.size());
  for (const auto &entry : descriptorsByControlId) {
    ids.push_back(entry.first);
  }
  std::sort(ids.begin(), ids.end());
  return ids;
}

void registerNatKitDeviceControls(DeviceControlDescriptorRegistry &registry) {
  registry.registerDescriptor(make(
      "identify", "Identify", DeviceControlKind::Button, "identify",
      /*readCommand=*/std::string{}, /*field=*/std::string{},
      FieldValueType::Bool,
      "Flash this board's LED so you can see which one on the bench it is"));

  // ⚠️ ONE GROUP, read whole and written one field at a time. The device starts
  // from its current mask and applies only the field it was sent, so two toggles
  // in flight cannot clobber each other -- confirmed against a board 2026-08-26.
  //
  // ⚠️ AND THIS IS NOT A RATE CONTROL. The sensor delivers its reports in bursts
  // at ~88 Hz regardless of how many are enabled (TEC-NATKIT-41). The reasons to
  // turn one off are airtime, power and not wanting the channel. Said here
  // because the obvious assumption is wrong and would otherwise be made silently.
  const auto report = [&](const std::string &field, const std::string &label) {
    registry.registerDescriptor(make(
        "reports." + field, label, DeviceControlKind::Toggle, "set_reports",
        "get_reports", field, FieldValueType::Bool,
        "Whether this device collects " + label.substr(0, 1) +
            [&label] {
              std::string rest = label.substr(1);
              std::transform(rest.begin(), rest.end(), rest.begin(),
                             [](unsigned char c) { return std::tolower(c); });
              return rest;
            }() +
            " samples. Turning it off saves airtime and power; it does NOT make "
            "the other reports faster.",
        std::string{}, "reports"));
  };
  report("accel", "Accelerometer");
  report("gyro", "Gyroscope");
  report("mag", "Magnetometer");
  report("rotation", "Rotation");

  // Quarter-dBm because that is the unit the radio itself uses; converting on
  // the way out would put two representations of one number in the system.
  registry.registerDescriptor(make(
      "tx_power", "Transmit power", DeviceControlKind::Input, "set_tx_power",
      "get_tx_power", "quarter_dbm", FieldValueType::Int16,
      "ESP-NOW transmit power. Pinning it stops the per-boot sweep choosing a "
      "different level on each node (TEC-NATKIT-37).",
      "quarter dBm", std::string{}, /*ranged=*/true, 8.0, 80.0));
}

DeviceControlDescriptorRegistry &DeviceControlDescriptorRegistry::getDefault() {
  static DeviceControlDescriptorRegistry registry{};
  static bool initialized = false;
  if (!initialized) {
    registerNatKitDeviceControls(registry);
    initialized = true;
  }
  return registry;
}

}  // namespace core
}  // namespace nat
