// The device-control descriptor registry (TEC-NATKIT-10).
//
// What is being protected here is an ARCHITECTURAL property, not a calculation:
// a third party must be able to add controls for its own hardware without
// modifying any natKit source. So the important test is the one that registers a
// descriptor natKit has never heard of and finds it again.

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include <libnatkit-core.hpp>

namespace {

int g_failures = 0;

void check(const bool condition, const std::string &what) {
  if (!condition) {
    std::printf("  FAIL: %s\n", what.c_str());
    ++g_failures;
  }
}

}  // namespace

int main() {
  using namespace nat::core;

  // --- natKit's own controls are there by default -------------------------
  {
    auto &registry = DeviceControlDescriptorRegistry::getDefault();
    const auto ids = registry.registeredControlIds();
    check(ids.size() >= 6, "the default registry carries natKit's controls");

    const auto identify = registry.findByControlId("identify");
    check(identify.has_value(), "identify is registered");
    if (identify.has_value()) {
      const auto &d = *identify.value();
      check(d.getKind() == DeviceControlKind::Button, "identify is a button");
      check(d.getWriteCommand() == "identify", "identify's command");
      // ⚠️ A button has nothing to read. If this ever gains a read command the
      // UI would render a pointless "refresh" next to a one-shot action.
      check(d.getReadCommand().empty(), "a button has no read command");
      check(d.commands() == std::vector<std::string>{"identify"},
            "identify permits exactly one command");
    }

    const auto accel = registry.findByControlId("reports.accel");
    check(accel.has_value(), "reports.accel is registered");
    if (accel.has_value()) {
      const auto &d = *accel.value();
      check(d.getKind() == DeviceControlKind::Toggle, "a report is a toggle");
      check(d.getGroup() == "reports", "reports share one group");
      check(d.getReadCommand() == "get_reports", "the group's read command");
      check(d.getWriteCommand() == "set_reports", "the group's write command");
      // The wire key, not the label -- this is what goes in the args object.
      check(d.getField() == "accel", "the field is the wire key");
      // ⚠️ BOTH commands are permitted. Gating only on the write would let the UI
      // offer a "Read from device" the server then refuses.
      check(d.commands().size() == 2, "a toggle permits its read AND its write");
    }

    const auto txPower = registry.findByControlId("tx_power");
    check(txPower.has_value(), "tx_power is registered");
    if (txPower.has_value()) {
      const auto &d = *txPower.value();
      check(d.getKind() == DeviceControlKind::Input, "tx_power is an input");
      // Typed with the EXISTING vocabulary rather than a parallel one.
      check(d.getValueType() == FieldValueType::Int16, "tx_power is an Int16");
      check(d.isRanged() && d.getMinValue() == 8.0 && d.getMaxValue() == 80.0,
            "tx_power carries its range");
      check(d.getUnit() == "quarter dBm", "tx_power states its unit");
    }

    check(!registry.findByControlId("nope.not.a.control").has_value(),
          "an unknown control id resolves to nothing");
  }

  // --- ⚠️ THE POINT OF THE WHOLE DESIGN -----------------------------------
  //
  // A library natKit knows nothing about registers a control for its own
  // hardware. No natKit source is touched, and it resolves like any other.
  {
    DeviceControlDescriptorRegistry registry;
    registerNatKitDeviceControls(registry);

    const auto thirdParty = std::make_shared<const DeviceControlDescriptor>(
        "acme.valve", "Valve", DeviceControlKind::Toggle, "acme_set_valve",
        "acme_get_valve", "open", FieldValueType::Bool,
        "Open or close the ACME valve", std::string{}, "acme");
    const bool replaced = registry.registerDescriptor(thirdParty);
    check(!replaced, "a brand-new control id does not displace anything");

    const auto found = registry.findByControlId("acme.valve");
    check(found.has_value(), "a third-party control resolves");
    if (found.has_value()) {
      check(found.value()->getLabel() == "Valve", "with its own label");
      check(found.value()->commands().size() == 2,
            "and its own commands are permitted");
    }
    // natKit's are untouched by the injection.
    check(registry.findByControlId("identify").has_value(),
          "injecting does not disturb the built-ins");
  }

  // --- overriding is allowed, and says so ---------------------------------
  //
  // Last-wins lets a library re-word or translate a control without forking it.
  // The return value is how a caller that did NOT mean to override finds out.
  {
    DeviceControlDescriptorRegistry registry;
    registerNatKitDeviceControls(registry);

    const auto override_ = std::make_shared<const DeviceControlDescriptor>(
        "identify", "Trouver", DeviceControlKind::Button, "identify");
    const bool replaced = registry.registerDescriptor(override_);
    check(replaced, "re-registering an existing id reports that it displaced one");
    check(registry.findByControlId("identify").value()->getLabel() == "Trouver",
          "and the last registration wins");
  }

  // --- a registry that refuses nonsense ------------------------------------
  {
    DeviceControlDescriptorRegistry registry;
    check(!registry.registerDescriptor(nullptr), "a null descriptor is refused");
    const auto blank = std::make_shared<const DeviceControlDescriptor>(
        "", "Nameless", DeviceControlKind::Button, "x");
    check(!registry.registerDescriptor(blank), "an empty control id is refused");
    check(registry.registeredControlIds().empty(), "and neither was stored");
  }

  // --- the kind vocabulary round-trips -------------------------------------
  {
    for (const auto kind : {DeviceControlKind::Button, DeviceControlKind::Toggle,
                            DeviceControlKind::Input}) {
      const auto name = toString(kind);
      check(!name.empty(), "every kind has a name");
      const auto back = deviceControlKindFromString(name);
      check(back.has_value() && back.value() == kind,
            "kind survives a string round trip: " + name);
    }
    check(!deviceControlKindFromString("slider").has_value(),
          "an unknown kind is not invented");
  }

  if (g_failures > 0) {
    std::printf("device_control_registry_test: %d failure(s)\n", g_failures);
    return 1;
  }
  std::printf("device_control_registry_test: ok\n");
  return 0;
}
