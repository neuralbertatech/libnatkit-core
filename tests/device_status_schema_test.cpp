// Wire-layout test for the two device-health schemas, NatKitNodeStatusV1 and
// NatKitPrimaryStatusV1.
//
// What can go wrong here is not a crash. Both payloads are a memcpy of a struct
// the ESP-IDF firmware built for xtensa, decoded field by field at hard-coded
// offsets; if one offset is wrong the decode still SUCCEEDS and returns numbers
// that look like counters. So the fixture carries frames captured from the live
// four-leaf rig together with expected values decoded independently in Python
// from the firmware's struct declaration -- a second reading of the layout, so
// the C++ is checked against something other than itself.
//
// Two cross-checks in the fixture are worth more than any single field:
//
//   * the node's sync.epoch equals the primary's epoch. Two different structs,
//     two different offsets (56 and 16), one value the firmware puts in both.
//   * the node's last_seen_us sits just under the primary's uptime_us, both on
//     the hub's clock, ~20 ms apart -- which is what a status frame assembled
//     shortly after the last packet should look like, and nothing like what a
//     shifted read of an 8-byte field would give.
//
// ⚠️ The fixture's frames_sent EXCEEDS its frames_queued while frames_dropped is
// 0, which the firmware's code makes impossible. That is not a decode error; it
// is TEC-NATKIT-75, a lost-update race on frames_queued. The test asserts the
// bytes decode to those values, deliberately including the impossible pair: this
// test's job is the layout, and pinning the anomaly here is what stops a future
// reader from "fixing" the offset to make the arithmetic work.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <libnatkit-core.hpp>
#include <nlohmann/json.hpp>

namespace {

int g_failures = 0;

void fail(const std::string &message) {
  std::printf("  FAIL: %s\n", message.c_str());
  ++g_failures;
}

void expectTrue(const bool condition, const std::string &message) {
  if (!condition) {
    fail(message);
  }
}

std::vector<uint8_t> fromHex(const std::string &hex) {
  std::vector<uint8_t> bytes;
  bytes.reserve(hex.size() / 2);
  for (size_t i = 0; i + 1 < hex.size(); i += 2) {
    bytes.push_back(static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
  }
  return bytes;
}

// Every expectation goes through the DESCRIPTOR rather than the record's members,
// because the descriptor is what the backend and the frontend actually read. A
// member that decodes correctly but is unreachable by path is not usable.
void expectField(const nat::core::DataSchemaDescriptor &descriptor,
                 const nat::core::Schema &record, const std::string &path,
                 const nlohmann::json &expected) {
  const auto valueMaybe = descriptor.tryGetFieldValue(record, path);
  if (!valueMaybe.has_value()) {
    fail(path + ": no value (the descriptor cannot reach it)");
    return;
  }
  const auto &value = valueMaybe.value();
  std::ostringstream actual;
  // ⚠️ Every getter returns an Optional: it is empty when the stored type is not
  // the one asked for. Unwrapping without checking would print 0 for a field
  // whose declared type and stored type disagree -- the exact confusion this
  // test exists to catch -- so a type mismatch is a failure, not a zero.
  switch (value.getValueType()) {
    case nat::core::FieldValueType::Uint64: {
      const auto v = value.getUint64();
      if (!v.has_value()) { fail(path + ": declared Uint64, stored something else"); return; }
      actual << v.value();
      break;
    }
    case nat::core::FieldValueType::Uint32: {
      const auto v = value.getUint32();
      if (!v.has_value()) { fail(path + ": declared Uint32, stored something else"); return; }
      actual << v.value();
      break;
    }
    case nat::core::FieldValueType::Int16: {
      const auto v = value.getInt16();
      if (!v.has_value()) { fail(path + ": declared Int16, stored something else"); return; }
      actual << v.value();
      break;
    }
    case nat::core::FieldValueType::Float64: {
      const auto v = value.getFloat64();
      if (!v.has_value()) { fail(path + ": declared Float64, stored something else"); return; }
      actual << static_cast<int64_t>(v.value());
      break;
    }
    case nat::core::FieldValueType::Bool: {
      const auto v = value.getBool();
      if (!v.has_value()) { fail(path + ": declared Bool, stored something else"); return; }
      actual << (v.value() ? "true" : "false");
      break;
    }
    case nat::core::FieldValueType::String: {
      const auto v = value.getString();
      if (!v.has_value()) { fail(path + ": declared String, stored something else"); return; }
      actual << v.value();
      break;
    }
    default:
      fail(path + ": unexpected field type");
      return;
  }
  std::ostringstream want;
  if (expected.is_string()) {
    want << expected.get<std::string>();
  } else if (expected.is_boolean()) {
    want << (expected.get<bool>() ? "true" : "false");
  } else {
    want << expected.get<int64_t>();
  }
  if (actual.str() != want.str()) {
    fail(path + ": got " + actual.str() + ", expected " + want.str());
  }
}

}  // namespace

int main() {
  std::ifstream fixture(NAT_DEVICE_STATUS_FIXTURE_PATH);
  if (!fixture.is_open()) {
    std::printf("FAIL: cannot open %s\n", NAT_DEVICE_STATUS_FIXTURE_PATH);
    return 1;
  }
  nlohmann::json golden;
  fixture >> golden;

  auto &registry = nat::core::DataSchemaDescriptorRegistry::getDefault();

  // --- node status --------------------------------------------------------
  {
    const auto &entry = golden.at("node");
    const auto bytes = fromHex(entry.at("hex").get<std::string>());
    expectTrue(bytes.size() == entry.at("size").get<size_t>(), "node fixture size");

    const auto decoded = nat::core::NatKitNodeStatusV1Schema::decodeBinary(bytes);
    if (!decoded.has_value()) {
      std::printf("FAIL: node status did not decode\n");
      return 1;
    }
    const auto record = decoded.value();

    const auto descriptorMaybe =
        registry.findBySchemaName(nat::core::NatKitNodeStatusV1Schema::name);
    if (!descriptorMaybe.has_value()) {
      std::printf("FAIL: no descriptor registered for NatKitNodeStatusV1\n");
      return 1;
    }
    const auto &descriptor = *descriptorMaybe.value();
    for (auto it = entry.at("expect").begin(); it != entry.at("expect").end(); ++it) {
      expectField(descriptor, record, it.key(), it.value());
    }

    // A field name that does not exist must be unresolved, not zero. Reporting 0
    // for a typo is how a health panel shows a reassuring number for a counter
    // nobody is reading.
    expectTrue(!descriptor.tryGetFieldValue(record, "sync.not_a_field").has_value(),
               "node: an unknown sync field must be unresolved");

    expectTrue(record.encodeBinary() == bytes, "node: encode(decode(x)) == x");

    // ⚠️ Exact size or refuse. A short read would decode the leading fields fine
    // and invent the rest; the plausible half is what gets quoted.
    std::vector<uint8_t> shortBytes(bytes.begin(), bytes.end() - 1);
    expectTrue(!nat::core::NatKitNodeStatusV1Schema::decodeBinary(shortBytes).has_value(),
               "node: a short payload must be refused");
    std::vector<uint8_t> longBytes(bytes);
    longBytes.push_back(0);
    expectTrue(!nat::core::NatKitNodeStatusV1Schema::decodeBinary(longBytes).has_value(),
               "node: an over-long payload must be refused");
  }

  // --- primary status ------------------------------------------------------
  {
    const auto &entry = golden.at("primary");
    const auto bytes = fromHex(entry.at("hex").get<std::string>());
    expectTrue(bytes.size() == entry.at("size").get<size_t>(), "primary fixture size");

    const auto decoded = nat::core::NatKitPrimaryStatusV1Schema::decodeBinary(bytes);
    if (!decoded.has_value()) {
      std::printf("FAIL: primary status did not decode\n");
      return 1;
    }
    const auto record = decoded.value();

    const auto descriptorMaybe =
        registry.findBySchemaName(nat::core::NatKitPrimaryStatusV1Schema::name);
    if (!descriptorMaybe.has_value()) {
      std::printf("FAIL: no descriptor registered for NatKitPrimaryStatusV1\n");
      return 1;
    }
    const auto &descriptor = *descriptorMaybe.value();
    for (auto it = entry.at("expect").begin(); it != entry.at("expect").end(); ++it) {
      expectField(descriptor, record, it.key(), it.value());
    }

    expectTrue(!descriptor.tryGetFieldValue(record, "coherence_typical").has_value(),
               "primary: an unknown field must be unresolved");
    // Nothing nests in this schema, so a dotted path is a mistake and must not
    // resolve by accidentally matching its first segment.
    expectTrue(!descriptor.tryGetFieldValue(record, "sync.epoch").has_value(),
               "primary: a nested path must not resolve on a flat schema");

    expectTrue(record.encodeBinary() == bytes, "primary: encode(decode(x)) == x");

    std::vector<uint8_t> shortBytes(bytes.begin(), bytes.end() - 1);
    expectTrue(!nat::core::NatKitPrimaryStatusV1Schema::decodeBinary(shortBytes).has_value(),
               "primary: a short payload must be refused");
  }

  // --- the two cross-checks ------------------------------------------------
  {
    const auto node = nat::core::NatKitNodeStatusV1Schema::decodeBinary(
        fromHex(golden.at("node").at("hex").get<std::string>()));
    const auto primary = nat::core::NatKitPrimaryStatusV1Schema::decodeBinary(
        fromHex(golden.at("primary").at("hex").get<std::string>()));
    if (!node.has_value() || !primary.has_value()) {
      std::printf("FAIL: cross-check inputs did not decode\n");
      return 1;
    }
    expectTrue(node.value().syncEpoch == primary.value().epoch,
               "cross-check: the node's sync.epoch must equal the primary's epoch");
    // Both are the hub's monotonic clock. last_seen_us is when the hub last heard
    // this leaf, so it precedes the snapshot -- by tens of ms, not by hours.
    const uint64_t lag = primary.value().uptimeUs - node.value().lastSeenUs;
    expectTrue(primary.value().uptimeUs > node.value().lastSeenUs && lag < 1000000ULL,
               "cross-check: last_seen_us must sit just under uptime_us");
  }

  if (g_failures > 0) {
    std::printf("device_status_schema_test: %d failure(s)\n", g_failures);
    return 1;
  }
  std::printf("device_status_schema_test: ok\n");
  return 0;
}
