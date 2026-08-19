#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

#include <libnatkit-core.hpp>

namespace nat {
namespace core {

namespace {

std::vector<std::pair<int64_t, size_t>>
sortedTimestampOrder(const std::vector<int64_t> &timestamps) {
  std::vector<std::pair<int64_t, size_t>> ordered{};
  ordered.reserve(timestamps.size());
  for (size_t index = 0; index < timestamps.size(); ++index) {
    ordered.emplace_back(timestamps[index], index);
  }
  std::sort(ordered.begin(), ordered.end(),
            [](const std::pair<int64_t, size_t> &lhs,
               const std::pair<int64_t, size_t> &rhs) {
              if (lhs.first != rhs.first) {
                return lhs.first < rhs.first;
              }
              return lhs.second < rhs.second;
            });
  return ordered;
}

std::vector<TimelineInterval>
sortedIntervals(const std::vector<TimelineInterval> &intervals) {
  auto ordered = intervals;
  std::sort(ordered.begin(), ordered.end(),
            [](const TimelineInterval &lhs, const TimelineInterval &rhs) {
              if (lhs.start_time_us != rhs.start_time_us) {
                return lhs.start_time_us < rhs.start_time_us;
              }
              if (lhs.end_time_us != rhs.end_time_us) {
                return lhs.end_time_us < rhs.end_time_us;
              }
              return lhs.value < rhs.value;
            });
  return ordered;
}

std::vector<TimelinePoint>
sortedPoints(const std::vector<TimelinePoint> &points) {
  auto ordered = points;
  std::sort(ordered.begin(), ordered.end(),
            [](const TimelinePoint &lhs, const TimelinePoint &rhs) {
              if (lhs.time_us != rhs.time_us) {
                return lhs.time_us < rhs.time_us;
              }
              return lhs.value < rhs.value;
            });
  return ordered;
}

} // namespace

std::vector<uint32_t> sortTimestampOrder(
    const std::vector<int64_t> &timestamps) {
  const auto ordered = sortedTimestampOrder(timestamps);
  std::vector<uint32_t> output{};
  output.reserve(ordered.size());
  for (size_t index = 0; index < ordered.size(); ++index) {
    output.push_back(static_cast<uint32_t>(ordered[index].second));
  }
  return output;
}

std::vector<int32_t> assignIntervalsToTimeline(
    const std::vector<int64_t> &timestamps,
    const std::vector<TimelineInterval> &intervals,
    int32_t default_value) {
  std::vector<int32_t> output(timestamps.size(), default_value);
  if (timestamps.empty() || intervals.empty()) {
    return output;
  }

  const auto orderedTimestamps = sortedTimestampOrder(timestamps);
  const auto orderedIntervals = sortedIntervals(intervals);

  for (const auto &interval : orderedIntervals) {
    if (interval.end_time_us < interval.start_time_us) {
      throw std::invalid_argument(
          "Timeline intervals must have end_time_us >= start_time_us");
    }
  }

  std::vector<size_t> activeIntervals{};
  size_t intervalIndex = 0;
  for (const auto &timestampEntry : orderedTimestamps) {
    const int64_t timestamp = timestampEntry.first;
    const size_t originalIndex = timestampEntry.second;
    activeIntervals.erase(
        std::remove_if(activeIntervals.begin(), activeIntervals.end(),
                       [&](size_t activeIndex) {
                         return orderedIntervals[activeIndex].end_time_us <=
                                timestamp;
                       }),
        activeIntervals.end());

    while (intervalIndex < orderedIntervals.size() &&
           orderedIntervals[intervalIndex].start_time_us <= timestamp) {
      if (orderedIntervals[intervalIndex].end_time_us > timestamp) {
        activeIntervals.push_back(intervalIndex);
      }
      ++intervalIndex;
    }

    if (!activeIntervals.empty()) {
      output[originalIndex] = orderedIntervals[activeIntervals.back()].value;
    }
  }

  return output;
}

std::vector<int32_t> assignPointsToTimeline(
    const std::vector<int64_t> &timestamps,
    const std::vector<TimelinePoint> &points,
    int32_t default_value) {
  std::vector<int32_t> output(timestamps.size(), default_value);
  if (timestamps.empty() || points.empty()) {
    return output;
  }

  const auto orderedTimestamps = sortedTimestampOrder(timestamps);
  const auto orderedPoints = sortedPoints(points);

  size_t pointIndex = 0;
  bool hasActivePoint = false;
  int32_t activeValue = default_value;

  for (const auto &timestampEntry : orderedTimestamps) {
    const int64_t timestamp = timestampEntry.first;
    const size_t originalIndex = timestampEntry.second;

    while (pointIndex < orderedPoints.size() &&
           orderedPoints[pointIndex].time_us <= timestamp) {
      activeValue = orderedPoints[pointIndex].value;
      hasActivePoint = true;
      ++pointIndex;
    }

    if (hasActivePoint) {
      output[originalIndex] = activeValue;
    }
  }

  return output;
}

} // namespace core
} // namespace nat

extern "C" int nat_sort_timestamps(const int64_t *timestamps,
                                   size_t timestamp_count,
                                   uint32_t *out_indices) {
  if (out_indices == nullptr) {
    return 1;
  }
  if (timestamps == nullptr && timestamp_count != 0) {
    return 1;
  }

  std::vector<int64_t> timestampVector{};
  timestampVector.reserve(timestamp_count);
  for (size_t index = 0; index < timestamp_count; ++index) {
    timestampVector.push_back(timestamps[index]);
  }

  try {
    const auto ordered = nat::core::sortTimestampOrder(timestampVector);
    for (size_t index = 0; index < ordered.size(); ++index) {
      out_indices[index] = ordered[index];
    }
  } catch (...) {
    return 3;
  }

  return 0;
}

extern "C" int nat_assign_intervals_to_timeline(
    const int64_t *timestamps, size_t timestamp_count,
    const nat_timeline_interval_t *intervals, size_t interval_count,
    int32_t default_value, int32_t *out_values) {
  if (out_values == nullptr) {
    return 1;
  }
  if ((timestamps == nullptr && timestamp_count != 0) ||
      (intervals == nullptr && interval_count != 0)) {
    return 1;
  }

  std::vector<int64_t> timestampVector{};
  timestampVector.reserve(timestamp_count);
  for (size_t index = 0; index < timestamp_count; ++index) {
    timestampVector.push_back(timestamps[index]);
  }

  std::vector<nat::core::TimelineInterval> intervalVector{};
  intervalVector.reserve(interval_count);
  for (size_t index = 0; index < interval_count; ++index) {
    intervalVector.push_back(nat::core::TimelineInterval{
        intervals[index].start_time_us,
        intervals[index].end_time_us,
        intervals[index].value,
    });
  }

  try {
    const auto stitched = nat::core::assignIntervalsToTimeline(
        timestampVector, intervalVector, default_value);
    for (size_t index = 0; index < stitched.size(); ++index) {
      out_values[index] = stitched[index];
    }
  } catch (const std::invalid_argument &) {
    return 2;
  } catch (...) {
    return 3;
  }

  return 0;
}

extern "C" int nat_assign_points_to_timeline(
    const int64_t *timestamps, size_t timestamp_count,
    const nat_timeline_point_t *points, size_t point_count,
    int32_t default_value, int32_t *out_values) {
  if (out_values == nullptr) {
    return 1;
  }
  if ((timestamps == nullptr && timestamp_count != 0) ||
      (points == nullptr && point_count != 0)) {
    return 1;
  }

  std::vector<int64_t> timestampVector{};
  timestampVector.reserve(timestamp_count);
  for (size_t index = 0; index < timestamp_count; ++index) {
    timestampVector.push_back(timestamps[index]);
  }

  std::vector<nat::core::TimelinePoint> pointVector{};
  pointVector.reserve(point_count);
  for (size_t index = 0; index < point_count; ++index) {
    pointVector.push_back(nat::core::TimelinePoint{
        points[index].time_us,
        points[index].value,
    });
  }

  try {
    const auto stitched = nat::core::assignPointsToTimeline(
        timestampVector, pointVector, default_value);
    for (size_t index = 0; index < stitched.size(); ++index) {
      out_values[index] = stitched[index];
    }
  } catch (...) {
    return 3;
  }

  return 0;
}
