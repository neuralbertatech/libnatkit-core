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
    const std::string &description = std::string{},
    const std::string &unit = std::string{}) {
  return std::make_shared<const DeviceControlDescriptor>(controlId, label,
                                                         description, unit);
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

DeviceControlDescriptor::DeviceControlDescriptor() {}

DeviceControlDescriptor::DeviceControlDescriptor(const std::string &controlId,
                                                 const std::string &label,
                                                 const std::string &description,
                                                 const std::string &unit)
    : controlId(controlId),
      label(label),
      description(description),
      unit(unit) {}

const std::string &DeviceControlDescriptor::getControlId() const { return controlId; }
const std::string &DeviceControlDescriptor::getLabel() const { return label; }
const std::string &DeviceControlDescriptor::getDescription() const { return description; }
const std::string &DeviceControlDescriptor::getUnit() const { return unit; }

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
      "identify", "Identify",
      "Flash this board's LED so you can see which one on the bench it is"));

  // ⚠️ NOT A RATE CONTROL, and the description says so because the obvious
  // assumption is wrong and would otherwise be made silently: the sensor
  // delivers its reports in bursts at ~88 Hz regardless of how many are enabled
  // (TEC-NATKIT-41). The reasons to turn one off are airtime, power, and not
  // wanting the channel.
  const char *kNotARate =
      " Turning it off saves airtime and power; it does NOT make the other "
      "reports faster.";
  registry.registerDescriptor(make("reports.accel", "Accelerometer",
                                   std::string("Collect accelerometer samples.") + kNotARate));
  registry.registerDescriptor(make("reports.gyro", "Gyroscope",
                                   std::string("Collect gyroscope samples.") + kNotARate));
  registry.registerDescriptor(make("reports.mag", "Magnetometer",
                                   std::string("Collect magnetometer samples.") + kNotARate));
  registry.registerDescriptor(make("reports.rotation", "Rotation",
                                   std::string("Collect the fused rotation vector.") + kNotARate));

  // Quarter-dBm because that is the unit the radio itself uses; converting on
  // the way out would put two representations of one number in the system.
  // ⚠️ The RANGE is not here -- the device advertises it, because the device is
  // what enforces it.
  registry.registerDescriptor(make(
      "tx_power", "Transmit power",
      "ESP-NOW transmit power. Pinning it stops the per-boot sweep choosing a "
      "different level on each node (TEC-NATKIT-37).",
      "quarter dBm"));
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
