// Golden-vector test for the libnatkit-core stream-identity C ABI.
//
// Reads the SAME checked-in fixture (tests/stream_id_golden.json) that the
// Python ctypes test reads, so both language bindings are held to one shared
// source of truth rather than to independently-regenerated expectations.
//
// Usage: stream_id_abi_test <path-to-stream_id_golden.json>
// (falls back to the compile-time NAT_GOLDEN_FIXTURE_PATH define if no arg).

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <libnatkit-core-abi.h>

namespace {

constexpr uint64_t kInt64Max = (1ULL << 63) - 1ULL;
int g_failures = 0;

void fail(const std::string &message) {
  std::fprintf(stderr, "FAIL: %s\n", message.c_str());
  ++g_failures;
}

std::string buildTopic(const std::string &streamType,
                       const std::string &ns,
                       const std::string &identifier,
                       const std::string &serialization,
                       const std::string &schema) {
  size_t size = 0;
  int status = nat_core_v1_topic_build(streamType.c_str(), ns.c_str(),
                                       identifier.c_str(), serialization.c_str(),
                                       schema.c_str(), nullptr, &size);
  if (status != NAT_OK || size == 0) {
    fail("topic_build sizing call failed for " + identifier);
    return {};
  }
  std::vector<char> buffer(size);
  status = nat_core_v1_topic_build(streamType.c_str(), ns.c_str(),
                                   identifier.c_str(), serialization.c_str(),
                                   schema.c_str(), buffer.data(), &size);
  if (status != NAT_OK) {
    fail("topic_build fill call failed for " + identifier);
    return {};
  }
  return std::string(buffer.data());
}

} // namespace

int main(int argc, char **argv) {
  std::string fixturePath;
  if (argc > 1) {
    fixturePath = argv[1];
  } else {
#ifdef NAT_GOLDEN_FIXTURE_PATH
    fixturePath = NAT_GOLDEN_FIXTURE_PATH;
#endif
  }
  if (fixturePath.empty()) {
    std::fprintf(stderr, "usage: %s <stream_id_golden.json>\n", argv[0]);
    return 2;
  }

  std::ifstream file(fixturePath);
  if (!file) {
    std::fprintf(stderr, "cannot open fixture: %s\n", fixturePath.c_str());
    return 2;
  }
  nlohmann::json fixture;
  file >> fixture;

  for (const auto &vector : fixture.at("stream_id_vectors")) {
    const auto ns = vector.at("namespace").get<std::string>();
    const auto identifier = vector.at("identifier").get<std::string>();
    const auto expected = vector.at("stream_id").get<uint64_t>();

    uint64_t actual = 0;
    const int status =
        nat_core_v1_stream_id(ns.c_str(), identifier.c_str(), &actual);
    if (status != NAT_OK) {
      fail("stream_id status " + std::to_string(status) + " for " + identifier);
      continue;
    }
    if (actual != expected) {
      fail("stream_id mismatch for " + ns + ":" + identifier + " expected " +
           std::to_string(expected) + " got " + std::to_string(actual));
    }
    // Post-conditions of the mask + 0->1 fallback: never 0, never > INT64_MAX.
    if (actual == 0 || actual > kInt64Max) {
      fail("stream_id out of range for " + identifier);
    }
  }

  for (const auto &vector : fixture.at("topic_vectors")) {
    const auto streamType = vector.at("stream_type").get<std::string>();
    const auto ns = vector.at("namespace").get<std::string>();
    const auto identifier = vector.at("identifier").get<std::string>();
    const auto serialization = vector.at("serialization").get<std::string>();
    const auto schema = vector.at("schema_name").get<std::string>();
    const auto expectedTopic = vector.at("topic").get<std::string>();

    const std::string topic =
        buildTopic(streamType, ns, identifier, serialization, schema);
    if (topic != expectedTopic) {
      fail("topic_build mismatch expected '" + expectedTopic + "' got '" +
           topic + "'");
      continue;
    }

    // Round-trip: parse the topic we just built and confirm the components.
    uint64_t parsedId = 0;
    size_t typeSize = 0, serSize = 0, schemaSize = 0;
    if (nat_core_v1_topic_parse(topic.c_str(), &parsedId, nullptr, &typeSize,
                                nullptr, &serSize, nullptr,
                                &schemaSize) != NAT_OK) {
      fail("topic_parse sizing failed for '" + topic + "'");
      continue;
    }
    std::vector<char> typeBuf(typeSize), serBuf(serSize), schemaBuf(schemaSize);
    const int parseStatus = nat_core_v1_topic_parse(
        topic.c_str(), &parsedId, typeBuf.data(), &typeSize, serBuf.data(),
        &serSize, schemaBuf.data(), &schemaSize);
    if (parseStatus != NAT_OK) {
      fail("topic_parse fill failed for '" + topic + "'");
      continue;
    }
    uint64_t expectedId = 0;
    nat_core_v1_stream_id(ns.c_str(), identifier.c_str(), &expectedId);
    if (parsedId != expectedId || std::string(typeBuf.data()) != streamType ||
        std::string(serBuf.data()) != serialization ||
        std::string(schemaBuf.data()) != schema) {
      fail("topic_parse round-trip mismatch for '" + topic + "'");
    }
  }

  if (g_failures == 0) {
    std::printf("PASS: stream-id ABI golden vectors\n");
    return 0;
  }
  std::fprintf(stderr, "%d failure(s)\n", g_failures);
  return 1;
}
