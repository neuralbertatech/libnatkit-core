# libnatkit-core C ABI Conventions

This is the single, written-down policy for the stable `extern "C"` ABI that
non-C++ languages bind against. It exists because retrofitting a policy after
Python / Java / Rust already depend on the symbols is expensive. Every new ABI
symbol follows these rules.

The ABI surface is declared in [`include/libnatkit-core-abi.h`](../include/libnatkit-core-abi.h),
a **pure C** header (no C++ syntax) so that `bindgen`, `jextract`, Panama FFM,
and plain C toolchains can parse it directly. `libnatkit-core.hpp` `#include`s it
but never redeclares these symbols.

## Why a C ABI (not per-language bindings)

We extend the shim pattern that already shipped in `StreamStitch.cpp` and is
consumed from Python via `ctypes` (`natVR/src/natvr/libnatkit_stitch.py`), rather
than wrapping C++ classes with pybind11/nanobind. A compiled extension module is
pinned to a specific CPython ABI (the dev venv runs 3.14, the ml-control-plane
image pins 3.12); `ctypes` loads a plain `.so` at runtime with no per-version
rebuild, and the same header serves Java and Rust later.

## Conventions

| Concern | Rule |
| --- | --- |
| Symbol naming | `nat_<module>_v1_<verb>` (e.g. `nat_core_v1_stream_id`). The `_v1_` segment is **frozen** once shipped; a breaking change ships as a new `_v2_` symbol *alongside* the old one. |
| Status codes | Every function returns `int`; `0` (`NAT_OK`) is success, nonzero is a documented error code. **No C++ exception ever crosses the boundary** — every `extern "C"` body wraps C++ calls in `try { ... } catch (...) { return NAT_ERR_INTERNAL; }`. |
| Struct layout | Fixed-width types only (`int64_t`, `uint32_t`, …), no packing pragmas. Fields are **append-only** after first release — never insert or reorder. |
| Fixed-size outputs | Caller allocates an output buffer at a size it already knows (e.g. one `int32_t` per input timestamp). |
| Variable-length outputs | **Two-call pattern.** First call with a `NULL` output buffer writes the required size (for strings, including the trailing `NUL`) through an in/out size pointer and returns `NAT_OK`; the caller allocates and calls again to fill. A too-small buffer on the fill call returns `NAT_ERR_BUFFER_TOO_SMALL` with the required size written back. Preferred over callee-allocates-plus-`nat_free()` because it avoids allocator-mismatch crashes across language runtimes and maps cleanly onto JNI/Panama/bindgen. |
| Opaque handles (Phase 4, shipped) | Opaque pointer typedef returned by an explicit `_create`, released by a matching `_destroy`. No handle is valid across a process boundary. Realized by the Kafka transport ABI — see below. |

## The Kafka transport ABI (separate library)

The Kafka transport (librdkafka-backed `nat::kafka::BrokerManager` /
`nat::core::TopicMessenger`) is exposed through its own header
`libnatkit/libnatkit/include/libnatkit-kafka-abi.h` and its own shared library
`liblibnatkit-kafka.so`, **separate from `liblibnatkit-core.so`**. This is
deliberate: `libnatkit-core` is the lean, embedded/ESP-IDF-friendly library and
must never take on a librdkafka dependency. Each tree owns a C ABI layer that
binds out to its own C++, and each is bound separately from Python
(`natvr.libnatkit_core` via `LIBNATKIT_CORE_PATH` vs `natvr.libnatkit_kafka` via
`LIBNATKIT_KAFKA_PATH`).

- **Symbols** follow `nat_kafka_v1_<verb>`; the same `NAT_OK` / `NAT_ERR_*`
  status registry is reused (the kafka header `#include`s the core header).
- **Opaque handles**: `nat_kafka_broker_t*` (from `nat_kafka_v1_broker_create`,
  released by `_broker_destroy`) and `nat_kafka_messenger_t*` (from
  `_messenger_create`, released by `_messenger_destroy`).
- **Two-call pattern** is used for `_broker_list_topics` (newline-joined string)
  and, with peek semantics so a too-small buffer never drops a message, for
  `_messenger_try_recv`.
- **Threading**: a broker handle is safe to share across threads (its
  registration state is now thread-safe — see the audit below); a *messenger*
  handle must not be used concurrently on the same instance because
  `_messenger_try_recv` holds a peeked message in the handle. Give each thread
  its own messenger, exactly as with a confluent-kafka `Consumer`.
- **Baseline**: `libnatkit/libnatkit/core/kafka/abi/symbols.txt` +
  `core/kafka/scripts/check_abi_symbols.sh`, mirroring the core lib's.
- **Build**: the lean `-DBUILD_KAFKA_ABI_ONLY=ON` CMake option builds only
  util + kafka-cxx + shim + librdkafka (no mosquitto/mqtt/bridge/tools/backend);
  used by `Dockerfile_natkit_ml_control_plane`.

### Status code registry

Values are append-only; existing codes keep their meaning forever
(`libnatkit-core-abi.h`):

| Code | Name | Meaning |
| --- | --- | --- |
| 0 | `NAT_OK` | Success |
| 1 | `NAT_ERR_NULL_ARGUMENT` | A required pointer argument was `NULL` |
| 2 | `NAT_ERR_INVALID_ARGUMENT` | An argument was malformed (bad topic-segment, unknown enum string, unparseable topic) |
| 3 | `NAT_ERR_BUFFER_TOO_SMALL` | Fill-call output buffer smaller than the required size |
| 4 | `NAT_ERR_DECODE_FAILED` | Payload could not be decoded |
| 5 | `NAT_ERR_ENCODE_FAILED` | Record could not be encoded |
| 6 | `NAT_ERR_ALLOCATION_FAILED` | Internal allocation failed |
| 7 | `NAT_ERR_INTERNAL` | Unexpected internal error (e.g. a caught C++ exception) |

> The pre-existing `nat_session_metadata_record_*` functions predate the two-call
> convention and use a callee-allocates + `nat_free_bytes()` ownership model.
> They are kept as-is for compatibility; **new** variable-length ABI must use the
> two-call pattern. The Phase 3a pilot established that pattern for schema
> transcoding via `nat_core_v1_marker_event_encode_json` /
> `_decode_json` (`MarkerEventV1.cpp`); Phase 3b rolled it out to
> `nat_core_v1_emg_frame_encode_json` / `_decode_json`
> (`ExgPillEmgDataSchemaV1.cpp`). SessionMetadataRecord keeps its existing
> callee-allocates ABI (already wired into `natvr.models`) rather than being
> duplicated into a second two-call symbol.

## Versioning / stability policy

- Exported symbols are **append-only**. Removing or renaming an exported symbol,
  or changing an existing symbol's signature, is a breaking change — do it by
  adding a new `_vN_` symbol and leaving the old one in place.
- `abi/symbols.txt` is the checked-in baseline of exported `nat_*` symbols.
  [`scripts/check_abi_symbols.sh`](../scripts/check_abi_symbols.sh) fails CI if any
  baseline symbol is missing from the built library, and lists new symbols that
  should be appended to the baseline in the same commit that adds them. This is
  wired into `.github/workflows/abi.yml`.
- When you add a symbol: implement it, add its declaration to
  `libnatkit-core-abi.h`, and append its name to `abi/symbols.txt` in the same
  change.

## Threading & thread-safety audit

`ctypes.CDLL` calls **release the Python GIL** for the duration of the call, so
concurrent calls from multiple Python threads genuinely execute inside the
library at the same time. Any shared mutable / lazily-initialized state reachable
from an ABI function must therefore be thread-safe, or the function must be
documented as single-threaded-caller-only. (Cross-process is not a concern — each
process loads its own copy of the `.so` with its own state.)

Audit of state reachable from the current ABI:

| Function(s) | Shared state reached | Verdict |
| --- | --- | --- |
| `nat_core_v1_stream_id` | none (pure FNV-1a over the inputs) | **Thread-safe.** |
| `nat_core_v1_topic_build`, `nat_core_v1_topic_parse` | `streamTypeFromString` / `serializationTypeFromString` read namespace-scope `static const` maps that are initialized during static init and never mutated afterward | **Thread-safe** (read-only after init). Writes to `std::cerr` on a malformed topic may interleave but are harmless. |
| `nat_sort_timestamps`, `nat_assign_*_to_timeline` | none (operate on caller buffers only) | **Thread-safe.** |
| `nat_session_metadata_record_encode_json` / `_decode_json`, plus any path that builds a default `Registry` (e.g. the Phase 4 `createBrokerManager`) | `ensureSessionMetadataRecordRegisteredForMetaRecord()` / `ensureTransformProvenanceRecordRegisteredForMetaRecord()` register into the shared function-local `metaRecordDecoders()` map | **Thread-safe** (fixed). Each `ensure*` guard is a `std::call_once`, and every read/write of `metaRecordDecoders()` is serialized by a dedicated `std::mutex` (`MetaRecord.cpp`), so concurrent first-calls — or a broker-create registering while another thread decodes — no longer race. |

**Gap closed (Phase 4 prerequisite):** the meta-record registration guards were
made thread-safe before the Kafka ABI (which reaches this state via
`createBrokerManager` → `Registry::createDefaultInitalizeRegistry()`) started
being called concurrently from GIL-released Python threads. The fix has two
parts: (1) `ensureSessionMetadataRecordRegisteredForMetaRecord` and
`ensureTransformProvenanceRecordRegisteredForMetaRecord` now use `std::call_once`
instead of a non-atomic `static bool`; (2) `metaRecordDecoders()` is guarded by a
`std::mutex` on **every** access — `registerMetaRecordType` and the three decode
read sites — so concurrent registration of different record types, and
read-during-registration, are both serialized. The decoder is copied out under
the lock and invoked outside it, so decode work does not run in the critical
section. This class of bug is easy to miss because it does not reproduce in
single-threaded testing.

## Verification

- **Golden vectors (stream identity):** `tests/stream_id_golden.json` is checked
  in once and read by *both* the C++ test binary (`tests/stream_id_abi_test.cpp`,
  built with `-DLIBNATKIT_CORE_BUILD_TESTS=ON`) and the Python ctypes test
  (`natVR/tests/test_libnatkit_core_abi.py`). It covers the `0 -> 1` /
  63-bit-mask post-condition and multibyte (UTF-8) identifiers.
- **Schema round trips (Phase 3a/3b):**
  `natVR/tests/test_libnatkit_marker_abi.py` round-trips the MarkerEventV1 ABI
  against REAL captured Kafka payloads
  (`natVR/tests/data/marker_events_real.jsonl`), and
  `natVR/tests/test_libnatkit_emg_abi.py` does the same for
  ExgPillEmgDataSchemaV1 (frames in the real producer wire format, seeded from
  recorded session parameters, since no raw EMG capture exists in-repo). Both
  assert byte-stable idempotency and field-for-field agreement with the
  hand-written Python decoders in `natvr.models`.
- **Symbol diff:** `scripts/check_abi_symbols.sh` in CI, tied to the append-only
  policy above.
