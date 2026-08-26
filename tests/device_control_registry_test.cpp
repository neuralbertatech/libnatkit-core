// The device-control descriptor registry (TEC-NATKIT-10).
//
// What is protected here is an ARCHITECTURAL property, not a calculation: a
// third party must be able to describe controls for its own hardware without
// modifying any natKit source. So the test that matters is the one registering a
// descriptor natKit has never heard of and finding it again.
//
// ⚠️ AND WHAT THIS CLASS MUST *NOT* HOLD. The registry owns PROSE only. The
// contract — kind, read/write commands, args key, type, range — belongs to the
// device and travels in DeviceControlAdvertisement. An earlier draft carried
// both, which is two copies of one contract: they drift, and then the UI offers
// a command the device does not implement while both sides look correct.

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

  // --- natKit's own prose is there by default ------------------------------
  {
    auto &registry = DeviceControlDescriptorRegistry::getDefault();
    check(registry.registeredControlIds().size() >= 6,
          "the default registry carries natKit's controls");

    const auto identify = registry.findByControlId("identify");
    check(identify.has_value(), "identify is described");
    if (identify.has_value()) {
      check(identify.value()->getLabel() == "Identify", "with a label");
      check(!identify.value()->getDescription().empty(), "and a description");
    }

    const auto accel = registry.findByControlId("reports.accel");
    check(accel.has_value(), "reports.accel is described");
    if (accel.has_value()) {
      // ⚠️ The description must carry the not-a-rate-control warning, because the
      // obvious assumption is wrong and is otherwise made silently.
      const auto &text = accel.value()->getDescription();
      check(text.find("does NOT make the other reports faster") != std::string::npos,
            "and says turning it off is not a rate control");
    }

    const auto txPower = registry.findByControlId("tx_power");
    check(txPower.has_value(), "tx_power is described");
    if (txPower.has_value()) {
      check(txPower.value()->getUnit() == "quarter dBm",
            "with the unit the radio itself uses");
    }

    check(!registry.findByControlId("nope.not.a.control").has_value(),
          "an unknown control id resolves to nothing");
  }

  // --- ⚠️ THE POINT OF THE WHOLE DESIGN -----------------------------------
  {
    DeviceControlDescriptorRegistry registry;
    registerNatKitDeviceControls(registry);

    const auto thirdParty = std::make_shared<const DeviceControlDescriptor>(
        "acme.valve", "Valve", "Open or close the ACME valve");
    check(!registry.registerDescriptor(thirdParty),
          "a brand-new control id does not displace anything");
    const auto found = registry.findByControlId("acme.valve");
    check(found.has_value() && found.value()->getLabel() == "Valve",
          "a third-party control resolves, with its own words");
    check(registry.findByControlId("identify").has_value(),
          "injecting does not disturb the built-ins");
  }

  // --- overriding is allowed, and says so ---------------------------------
  //
  // Last-wins lets a library re-word or translate without forking. The return
  // value is how a caller that did NOT mean to override finds out.
  {
    DeviceControlDescriptorRegistry registry;
    registerNatKitDeviceControls(registry);
    check(registry.registerDescriptor(
              std::make_shared<const DeviceControlDescriptor>("identify", "Trouver")),
          "re-registering an existing id reports that it displaced one");
    check(registry.findByControlId("identify").value()->getLabel() == "Trouver",
          "and the last registration wins");
  }

  // --- a registry that refuses nonsense ------------------------------------
  {
    DeviceControlDescriptorRegistry registry;
    check(!registry.registerDescriptor(nullptr), "a null descriptor is refused");
    check(!registry.registerDescriptor(
              std::make_shared<const DeviceControlDescriptor>("", "Nameless")),
          "an empty control id is refused");
    check(registry.registeredControlIds().empty(), "and neither was stored");
  }

  // --- the kind vocabulary round-trips -------------------------------------
  //
  // Kind lives on the WIRE, not here, but the vocabulary is shared so the
  // advertisement decoder can reject a kind the UI could never draw.
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
