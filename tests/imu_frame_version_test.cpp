// Frame-version compatibility test for NatImuBulkDataSchema's binary codec.
//
// Version 2 added the magnetometer, widening a sample from 50 to 62 bytes. The
// risk that carries is not that new frames break -- it is that OLD ONES DO, and
// silently: every recording made before 2026-08 is version 1, and a decoder that
// validates length against a single hard-coded sample size rejects all of them
// with nothing but a size mismatch to explain why.
//
// So the load-bearing case here is decodeV1Frame, not the v2 round trip. The v1
// byte vector below is built BY HAND rather than by calling the encoder, because
// an encoder-built fixture would follow the encoder wherever it went and prove
// only that it agrees with itself.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <libnatkit-core.hpp>

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

void expectNear(const float actual, const float expected, const std::string &what) {
  // Exact equality is the right expectation: the codec memcpys bit patterns and
  // never does arithmetic on them, so anything but equality is corruption.
  if (actual != expected) {
    fail(what + ": expected " + std::to_string(expected) + ", got " +
         std::to_string(actual));
  }
}

void appendU16(std::vector<uint8_t> &out, const uint16_t value) {
  out.push_back(static_cast<uint8_t>(value & 0xFF));
  out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void appendU32(std::vector<uint8_t> &out, const uint32_t value) {
  for (int i = 0; i < 4; ++i) {
    out.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xFF));
  }
}

void appendU64(std::vector<uint8_t> &out, const uint64_t value) {
  for (int i = 0; i < 8; ++i) {
    out.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xFF));
  }
}

void appendFloat(std::vector<uint8_t> &out, const float value) {
  uint8_t raw[sizeof(float)];
  std::memcpy(raw, &value, sizeof(float));
  for (size_t i = 0; i < sizeof(float); ++i) {
    out.push_back(raw[i]);
  }
}

// A hand-built version 1 frame: 24-byte header, then `sampleCount` 50-byte
// samples of ten floats each. This is the exact shape every pre-2026-08
// recording has on disk.
std::vector<uint8_t> buildV1Frame(const uint16_t sampleCount) {
  std::vector<uint8_t> frame;
  appendU16(frame, 1);            // schemaVersion
  appendU16(frame, sampleCount);  // sampleCount
  appendU32(frame, 100);          // sampleRateHz
  appendU64(frame, 4242);         // seqNo
  appendU64(frame, 1234567890);   // deviceTsUs

  for (uint16_t s = 0; s < sampleCount; ++s) {
    appendU64(frame, 1000 + s);
    for (int f = 0; f < 10; ++f) {
      appendFloat(frame, static_cast<float>(s * 100 + f));
    }
    // accuracies: accel 3, gyro 2, rotation 1. Bits 7-6 are the pair a v1
    // writer left unused, and they are deliberately set here to prove the
    // decoder does not read them as a magnetometer accuracy.
    appendU16(frame, 0);
    frame.pop_back();
    frame.back() = static_cast<uint8_t>(0xC0 | (3 << 4) | (2 << 2) | 1);
    // has_data: accel|gyro|rotation set, and bit 3 -- the one v2 uses for the
    // magnetometer -- also set, for the same reason.
    frame.push_back(static_cast<uint8_t>(0x0F));
  }
  return frame;
}

void decodeV1Frame() {
  std::printf("decodeV1Frame (a pre-magnetometer recording must still decode)\n");
  const uint16_t sampleCount = 10;
  const std::vector<uint8_t> frame = buildV1Frame(sampleCount);

  expectTrue(frame.size() == 24 + 50 * static_cast<size_t>(sampleCount),
             "hand-built v1 frame should be 524 bytes, got " +
                 std::to_string(frame.size()));

  auto decoded = nat::core::NatImuBulkDataSchema::decodeBinary(frame);
  if (!decoded.has_value()) {
    fail("a version 1 frame failed to decode at all -- every existing recording "
         "is this shape");
    return;
  }

  const auto &bulk = *decoded.value();
  expectTrue(bulk.getSampleCount() == sampleCount,
             "v1 sample count should survive the round trip");

  const auto &first = bulk.getSamples()[0];
  const float *values = first.getData();
  for (int f = 0; f < 10; ++f) {
    expectNear(values[f], static_cast<float>(f), "v1 float " + std::to_string(f));
  }

  // ⚠️ THE POINT OF THE TEST. The three magnetometer floats must read as absent,
  // not as zero-valued readings, and the has_data bit the v1 writer left set for
  // its own reasons must NOT be mistaken for a magnetometer claim.
  for (int f = 10; f < 13; ++f) {
    expectNear(values[f], 0.0F,
               "v1 frame must leave magnetometer float " + std::to_string(f) +
                   " zeroed");
  }
  expectTrue(!first.wasDataSetForMagnetometer(),
             "a v1 frame must report NO magnetometer, even though its spare "
             "has_data bit was set -- otherwise every old recording claims a "
             "magnetometer reading of (0,0,0)");
  expectTrue(static_cast<int>(first.getMagnetometerAccuracy()) == 0,
             "a v1 frame's spare accuracy bits 7-6 must be cleared, not reported "
             "as a magnetometer accuracy");
  expectTrue(static_cast<int>(first.getAccelerationAccuracy()) == 3 &&
                 static_cast<int>(first.getGyroscopeAccuracy()) == 2 &&
                 static_cast<int>(first.getRotationAccuracy()) == 1,
             "v1 accuracies for the three real sensors must survive the mask");
  expectTrue(first.wasDataSetForAcceleration() && first.wasDataSetForGryoscope() &&
                 first.wasDataSetForRotation(),
             "v1 has_data bits for the three real sensors must survive");
}

void roundTripV2Frame() {
  std::printf("roundTripV2Frame (the magnetometer must survive encode/decode)\n");

  nat::core::NatImuBulkDataSchema bulk;
  const uint16_t sampleCount = 10;
  for (uint16_t s = 0; s < sampleCount; ++s) {
    float values[13];
    for (int f = 0; f < 13; ++f) {
      values[f] = static_cast<float>(s * 100 + f);
    }
    // accuracies: mag 3 in bits 7-6, accel 3, gyro 2, rotation 1.
    // has_data: all four set.
    bulk.add(nat::core::NatImuDataSchema(
        1000 + s, static_cast<uint8_t>((3 << 6) | (3 << 4) | (2 << 2) | 1),
        static_cast<uint8_t>(0x0F), values, 13));
  }
  bulk.setFrameHeader(4242, 1234567890, 100);

  const auto encoded =
      bulk.encodeToBytes(nat::core::SerializationType::Binary);
  expectTrue(encoded->size() == 24 + 62 * static_cast<size_t>(sampleCount),
             "a v2 frame of 10 samples should be 644 bytes, got " +
                 std::to_string(encoded->size()));
  expectTrue((*encoded)[0] == 2 && (*encoded)[1] == 0,
             "the encoder must stamp version 2 in the header");

  auto decoded = nat::core::NatImuBulkDataSchema::decodeBinary(*encoded);
  if (!decoded.has_value()) {
    fail("a v2 frame failed to decode");
    return;
  }

  const auto &first = decoded.value()->getSamples()[0];
  const float *values = first.getData();
  for (int f = 0; f < 13; ++f) {
    expectNear(values[f], static_cast<float>(f), "v2 float " + std::to_string(f));
  }
  expectTrue(first.wasDataSetForMagnetometer(),
             "v2 must carry the magnetometer has_data bit");
  expectTrue(static_cast<int>(first.getMagnetometerAccuracy()) == 3,
             "v2 must carry the magnetometer accuracy from bits 7-6");
}

void rejectFutureVersion() {
  std::printf("rejectFutureVersion (a newer writer must be refused, not "
              "misparsed)\n");
  std::vector<uint8_t> frame = buildV1Frame(1);
  frame[0] = 99;  // schemaVersion = 99
  auto decoded = nat::core::NatImuBulkDataSchema::decodeBinary(frame);
  expectTrue(!decoded.has_value(),
             "a frame claiming version 99 must be rejected rather than decoded "
             "with whatever sample size this build happens to use");
}

void legacyHeaderlessStillWorks() {
  std::printf("legacyHeaderlessStillWorks (the 5000-byte sentinel)\n");
  // The headerless format is recognised only by being exactly 5000 bytes, which
  // is unambiguous while neither 24 + n*50 nor 24 + n*62 equals 5000. Adding the
  // 62-byte sample size did not break that, and this pins it.
  const std::vector<uint8_t> frame(5000, 0);
  auto decoded = nat::core::NatImuBulkDataSchema::decodeBinary(frame);
  expectTrue(decoded.has_value(),
             "a 5000-byte headerless legacy frame must still decode");
  if (decoded.has_value()) {
    expectTrue(!decoded.value()->getSamples()[0].wasDataSetForMagnetometer(),
               "a legacy frame has no magnetometer either");
  }
}

}  // namespace

int main() {
  std::printf("NatImuBulkDataSchema frame-version tests\n");
  decodeV1Frame();
  roundTripV2Frame();
  rejectFutureVersion();
  legacyHeaderlessStillWorks();

  if (g_failures == 0) {
    std::printf("all frame-version tests passed\n");
    return 0;
  }
  std::printf("%d frame-version check(s) failed\n", g_failures);
  return 1;
}
