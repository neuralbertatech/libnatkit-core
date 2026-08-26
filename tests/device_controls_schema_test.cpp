// The controls advertisement (TEC-NATKIT-10, slice 2).
//
// This decodes something a DEVICE wrote, so the tests that matter are the ones
// about malformed and hostile input. A control that decodes into a plausible-but
// -wrong shape becomes a button on screen that does nothing, or worse, widens the
// server's allowlist with a command nobody advertised.

#include <cstdio>
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

std::vector<uint8_t> bytes(const std::string &text) {
  return std::vector<uint8_t>(text.begin(), text.end());
}

const char *kGood = R"({
  "schema_version": "nat.controls.v1",
  "device_id": 13793649671244,
  "controls": [
    {"id":"identify","kind":"button","write":"identify"},
    {"id":"reports.accel","kind":"toggle","group":"reports",
     "read":"get_reports","write":"set_reports","field":"accel","type":"bool"},
    {"id":"tx_power","kind":"input","read":"get_tx_power","write":"set_tx_power",
     "field":"quarter_dbm","type":"int16","min":8,"max":80,"unit":"quarter dBm"}
  ]
})";

}  // namespace

int main() {
  using namespace nat::core;

  // --- the happy path ------------------------------------------------------
  {
    const auto decoded = NatKitDeviceControlsV1Schema::decodeJson(bytes(kGood));
    check(decoded.has_value(), "a well-formed advertisement decodes");
    if (decoded.has_value()) {
      const auto &r = decoded.value();
      check(r.deviceId == 13793649671244ULL, "the device id");
      // Absent advertiser means the device spoke for itself.
      check(r.advertiserDeviceId == r.deviceId,
            "an absent advertiser defaults to the device itself");
      check(r.controls.size() == 3, "three controls");
      check(r.controls[0].kind == "button", "the button");
      check(r.controls[1].group == "reports", "the toggle's group");
      check(r.controls[2].ranged && r.controls[2].minValue == 8.0 &&
                r.controls[2].maxValue == 80.0,
            "the input's range");
      check(r.controls[2].unit == "quarter dBm",
            "a device may carry its own unit");

      // ⚠️ THE ALLOWLIST IS DERIVED, and includes read commands.
      const auto allowed = r.allowedCommands();
      check(allowed == std::vector<std::string>{"get_reports", "get_tx_power",
                                                "identify", "set_reports",
                                                "set_tx_power"},
            "the allowlist is every read and write, deduplicated and sorted");
    }
  }

  // --- a proxied advertisement --------------------------------------------
  {
    const auto decoded = NatKitDeviceControlsV1Schema::decodeJson(bytes(
        R"({"schema_version":"nat.controls.v1","device_id":7,
            "advertiser_device_id":99,
            "controls":[{"id":"a","kind":"button","write":"a"}]})"));
    check(decoded.has_value(), "a proxied advertisement decodes");
    if (decoded.has_value()) {
      check(decoded.value().deviceId == 7, "the end device");
      check(decoded.value().advertiserDeviceId == 99, "the bridge that spoke");
    }
  }

  // --- ⚠️ the version tag is checked, not assumed --------------------------
  //
  // Configuration is deliberately open to third-party record types, so "some
  // JSON arrived on a Configuration topic" does not make it an advertisement.
  {
    check(!NatKitDeviceControlsV1Schema::decodeJson(bytes(
              R"({"schema_version":"acme.hookups.v1","device_id":1,
                  "controls":[{"id":"a","kind":"button","write":"a"}]})"))
               .has_value(),
          "another record type on this channel is not decoded as ours");
    check(!NatKitDeviceControlsV1Schema::decodeJson(
              bytes(R"({"device_id":1,"controls":[]})"))
               .has_value(),
          "a missing version tag is refused");
  }

  // --- malformed input is refused, not guessed at --------------------------
  {
    check(!NatKitDeviceControlsV1Schema::decodeJson(bytes("not json at all"))
               .has_value(),
          "garbage is refused rather than throwing");
    check(!NatKitDeviceControlsV1Schema::decodeJson(bytes("[1,2,3]")).has_value(),
          "a non-object is refused");
    check(!NatKitDeviceControlsV1Schema::decodeJson(bytes(
              R"({"schema_version":"nat.controls.v1","device_id":1})"))
               .has_value(),
          "an advertisement with no controls array is refused");
  }

  // --- ⚠️ unusable entries are DROPPED, and cannot widen the allowlist ------
  {
    const auto decoded = NatKitDeviceControlsV1Schema::decodeJson(bytes(
        R"({"schema_version":"nat.controls.v1","device_id":1,"controls":[
             {"id":"","kind":"button","write":"ghost"},
             {"id":"noverb","kind":"button"},
             {"id":"weird","kind":"slider","write":"wat"},
             {"id":"real","kind":"button","write":"real_cmd"}
           ]})"));
    check(decoded.has_value(), "the advertisement still decodes");
    if (decoded.has_value()) {
      const auto &r = decoded.value();
      check(r.controls.size() == 1, "only the usable control survives");
      check(r.controls[0].controlId == "real", "and it is the right one");
      // The three dropped entries must not appear as permitted commands --
      // an id-less control would otherwise contribute an empty string, and an
      // unknown kind would contribute a command the UI can never render.
      check(r.allowedCommands() == std::vector<std::string>{"real_cmd"},
            "dropped controls do not widen the allowlist");
    }
  }

  // --- round trip ----------------------------------------------------------
  {
    const auto first = NatKitDeviceControlsV1Schema::decodeJson(bytes(kGood));
    check(first.has_value(), "decode for round trip");
    if (first.has_value()) {
      const auto encoded = first.value().toJson();
      const auto second =
          NatKitDeviceControlsV1Schema::decodeJson(bytes(encoded));
      check(second.has_value(), "re-decodes what it encoded");
      if (second.has_value()) {
        check(second.value().controls.size() == first.value().controls.size(),
              "the same controls survive");
        check(second.value().allowedCommands() == first.value().allowedCommands(),
              "and the same allowlist");
      }
    }
  }

  // --- reachable through the Registry, by topic ---------------------------
  {
    auto registry = Registry::createDefaultInitalizeRegistry();
    NatKitDeviceControlsV1Schema::registerWithRegistry(*registry);
    const auto topic = BasicTopicInformation::create(
        "Configuration-13793649671244-Json-NatKitDeviceControlsV1");
    check(topic.has_value(), "the Configuration topic name parses");
    if (topic.has_value()) {
      const auto decoded = registry->tryDecode(bytes(kGood), *topic.value());
      check(decoded.has_value(),
            "a log viewer resolves the advertisement by topic alone");
    }
  }

  if (g_failures > 0) {
    std::printf("device_controls_schema_test: %d failure(s)\n", g_failures);
    return 1;
  }
  std::printf("device_controls_schema_test: ok\n");
  return 0;
}
