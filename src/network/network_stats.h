#ifndef SRC_NETWORK_NETWORK_STATS_H_
#define SRC_NETWORK_NETWORK_STATS_H_

#include <cstdint>

namespace bwe {

// Represents a single per-second snapshot of player-side network statistics.
struct NetworkSample {
  std::int64_t timestamp_ms = 0;

  // Jitter statistics (in milliseconds).
  double jitter_ms = 0.0;
  double jitter_delay_ms = 0.0;

  // Round-trip time estimate (in milliseconds).
  double rtt_ms = 0.0;

  // Bytes received in the last interval.
  std::uint64_t received_bytes = 0;

  // Player buffer level in milliseconds of media.
  double received_buffer_ms = 0.0;

  // Packet-level statistics.
  std::uint32_t packets_received = 0;
  std::uint32_t packets_lost = 0;

  // Rendering / decode statistics.
  double decode_fps = 0.0;
  std::uint32_t dropped_frames = 0;

  // Optional: requested bitrate from ABR (bps).
  std::uint64_t requested_bitrate_bps = 0;

  // Optional: currently selected stream bitrate (bps).
  std::uint64_t stream_bitrate_bps = 0;
};

}  // namespace bwe

#endif  // SRC_NETWORK_NETWORK_STATS_H_

