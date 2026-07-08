/*
 * libnatkit-core-abi.h -- stable C ABI for libnatkit-core.
 *
 * This header is intentionally PURE C (no C++ syntax anywhere). FFI generators
 * -- Rust `bindgen`, Java `jextract` / Panama FFM, and plain C consumers --
 * cannot parse C++ declarations even to skip past them, so every non-C++
 * language binds against this file, never against libnatkit-core.hpp. Do not
 * add C++ class declarations, templates, or `namespace` here.
 *
 * Conventions (full policy: docs/ABI_CONVENTIONS.md):
 *   - Symbols are named nat_<module>_v1_<verb>. The _v1_ segment is frozen once
 *     a symbol ships; a breaking change ships as a new _v2_ symbol *alongside*
 *     the old one, never by editing the existing signature in place.
 *   - Every function returns `int`: 0 (NAT_OK) == success, nonzero == error.
 *   - No C++ exception ever crosses this boundary.
 *   - Structs use fixed-width types only, no packing pragmas, and are
 *     append-only after first release -- never insert or reorder a field.
 *   - Fixed-size outputs are written into a caller-allocated buffer.
 *   - Variable-length outputs use the TWO-CALL pattern: call once with a NULL
 *     output buffer to learn the required size (written through an in/out size
 *     pointer, and for strings including the trailing NUL byte), allocate, then
 *     call again to fill. This avoids allocator-mismatch crashes across
 *     language runtimes that a callee-allocates-plus-free convention invites.
 */
#ifndef LIBNATKIT_CORE_ABI_H
#define LIBNATKIT_CORE_ABI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Status codes returned across the libnatkit-core C ABI. 0 is always success.
 * Values are append-only: existing codes keep their meaning forever.
 */
enum {
  NAT_OK = 0,
  NAT_ERR_NULL_ARGUMENT = 1,
  NAT_ERR_INVALID_ARGUMENT = 2,
  NAT_ERR_BUFFER_TOO_SMALL = 3,
  NAT_ERR_DECODE_FAILED = 4,
  NAT_ERR_ENCODE_FAILED = 5,
  NAT_ERR_ALLOCATION_FAILED = 6,
  NAT_ERR_INTERNAL = 7
};

/* -- Timeline stitching (StreamStitch.cpp) ------------------------------- */

typedef struct nat_timeline_interval_t {
  int64_t start_time_us;
  int64_t end_time_us;
  int32_t value;
} nat_timeline_interval_t;

typedef struct nat_timeline_point_t {
  int64_t time_us;
  int32_t value;
} nat_timeline_point_t;

int nat_sort_timestamps(const int64_t *timestamps,
                        size_t timestamp_count,
                        uint32_t *out_indices);

int nat_assign_intervals_to_timeline(const int64_t *timestamps,
                                     size_t timestamp_count,
                                     const nat_timeline_interval_t *intervals,
                                     size_t interval_count,
                                     int32_t default_value,
                                     int32_t *out_values);

int nat_assign_points_to_timeline(const int64_t *timestamps,
                                  size_t timestamp_count,
                                  const nat_timeline_point_t *points,
                                  size_t point_count,
                                  int32_t default_value,
                                  int32_t *out_values);

/* -- Stream identity & topic strings (BasicTopicInformation.cpp) --------- */

/*
 * Compute the stable stream id for a (namespace, identifier) pair: FNV-1a over
 * "<namespace>:<identifier>", masked to 63 bits, with a 0 result remapped to 1.
 * This is the single source of truth that C++ (StreamViewerWebSocket) and
 * Python (topics.py) previously each reimplemented.
 * Returns NAT_OK, or NAT_ERR_NULL_ARGUMENT if any pointer is NULL.
 */
int nat_core_v1_stream_id(const char *topic_namespace,
                          const char *identifier,
                          uint64_t *out_stream_id);

/*
 * Build the Kafka topic string "<Type>-<id>-<Serialization>-<Schema>", where id
 * is nat_core_v1_stream_id(topic_namespace, identifier). `stream_type` and
 * `serialization` are parsed case-insensitively and echoed back canonicalized
 * (e.g. "data" -> "Data"); an unknown value returns NAT_ERR_INVALID_ARGUMENT.
 * `identifier` must match [A-Za-z0-9][A-Za-z0-9_-]* or NAT_ERR_INVALID_ARGUMENT.
 *
 * Variable-length output (two-call pattern): with out_topic == NULL the
 * required buffer size (including the trailing NUL) is written to
 * *inout_topic_size and NAT_OK is returned. With a non-NULL out_topic that is
 * too small, NAT_ERR_BUFFER_TOO_SMALL is returned and the required size is
 * written to *inout_topic_size.
 */
int nat_core_v1_topic_build(const char *stream_type,
                            const char *topic_namespace,
                            const char *identifier,
                            const char *serialization,
                            const char *schema_name,
                            char *out_topic,
                            size_t *inout_topic_size);

/*
 * Parse a Kafka topic string into its components. On success out_stream_id (if
 * non-NULL) receives the numeric id, and the three string components use the
 * two-call pattern *together*: if any of out_stream_type / out_serialization /
 * out_schema_name is NULL this is treated as a sizing call -- every non-NULL
 * *inout_*_size receives that component's required size (including NUL) and
 * NAT_OK is returned without filling. When all three buffers are non-NULL each
 * is filled, or NAT_ERR_BUFFER_TOO_SMALL is returned if any is too small (with
 * required sizes written back). A malformed topic returns
 * NAT_ERR_INVALID_ARGUMENT. Type and serialization are canonicalized.
 */
int nat_core_v1_topic_parse(const char *topic,
                            uint64_t *out_stream_id,
                            char *out_stream_type,
                            size_t *inout_stream_type_size,
                            char *out_serialization,
                            size_t *inout_serialization_size,
                            char *out_schema_name,
                            size_t *inout_schema_name_size);

/* -- MarkerEventV1 JSON transcoding (MarkerEventV1.cpp) ------------------ *
 *
 * Phase 3a pilot of the two-call variable-length pattern for schema
 * transcoding. Both functions round-trip through the shared C++ MarkerEventV1
 * so the logic Python's natvr.models.MarkerEventV1 hand-decodes today has a
 * single source of truth.
 *
 * `encode` takes the logical field JSON
 * ({session_id, marker_type, marker_id, event, label, emitted_at_us,
 * attributes}) and returns the canonical wire message; `decode` takes a wire
 * message and returns the canonical field JSON. For MarkerEventV1 the JSON wire
 * format *is* the field set, so both are a canonicalization through the shared
 * struct; the two directions exist so call sites are insulated if the wire
 * format later changes (e.g. to Binary).
 *
 * Two-call pattern (unlike the callee-allocates session-metadata functions
 * below): pass out_* == NULL to write the exact required byte count (no NUL
 * terminator -- this is a byte buffer, not a C string) into *inout_*_size and
 * return NAT_OK; allocate and call again. A too-small buffer returns
 * NAT_ERR_BUFFER_TOO_SMALL with the required size written back. Malformed input
 * returns NAT_ERR_DECODE_FAILED.
 */

int nat_core_v1_marker_event_encode_json(const uint8_t *payload_json,
                                         size_t payload_json_size,
                                         uint8_t *out_message,
                                         size_t *inout_message_size);

int nat_core_v1_marker_event_decode_json(const uint8_t *message,
                                         size_t message_size,
                                         uint8_t *out_payload_json,
                                         size_t *inout_payload_json_size);

/* -- ExgPillEmgDataSchemaV1 JSON transcoding (ExgPillEmgDataSchemaV1.cpp) - *
 *
 * Phase 3b rollout of the same two-call convention to the EMG frame schema
 * (fields: schema_version, device_id, seq_no, device_ts_us, n_channels,
 * samples_per_channel, sample_rate_hz, channel_labels[], payload[][]). Same
 * contract and error codes as the MarkerEventV1 functions above. The C++
 * decoder is the strict validator (int16 sample range, channel/label/payload
 * count consistency) that Python's natvr.models.ExgPillEmgDataSchemaV1 mirrors.
 */

int nat_core_v1_emg_frame_encode_json(const uint8_t *payload_json,
                                      size_t payload_json_size,
                                      uint8_t *out_message,
                                      size_t *inout_message_size);

int nat_core_v1_emg_frame_decode_json(const uint8_t *message,
                                      size_t message_size,
                                      uint8_t *out_payload_json,
                                      size_t *inout_payload_json_size);

/* -- Session metadata record JSON transcoding (SessionMetadataRecord.cpp) -
 *
 * NOTE: these predate the two-call convention above and use a
 * callee-allocates-plus-nat_free_bytes() ownership model. New variable-length
 * ABI should prefer the two-call pattern; see docs/ABI_CONVENTIONS.md.
 */

int nat_session_metadata_record_encode_json(const uint8_t *payload_json,
                                            size_t payload_json_size,
                                            uint8_t **out_message,
                                            size_t *out_message_size);

int nat_session_metadata_record_decode_json(const uint8_t *message,
                                            size_t message_size,
                                            uint8_t **out_payload_json,
                                            size_t *out_payload_json_size);

/* Frees a buffer returned by a callee-allocates ABI function above. */
void nat_free_bytes(uint8_t *buffer);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LIBNATKIT_CORE_ABI_H */
