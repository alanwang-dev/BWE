#ifndef TEST_COMMON_NETWORK_SIMULATOR_H_
#define TEST_COMMON_NETWORK_SIMULATOR_H_

#include <cstddef>
#include <vector>

#include "src/network/network_stats.h"

namespace bwe {

enum class NetworkScenarioKind {
  kStableWifi = 0,
  kMobileFluctuating = 1,
  kSuddenCongestion = 2,
  kHighRttStableBw = 3,
  kBurstLoss = 4,
  kUplinkCongestion = 5,
};

// Test/validation only: generates synthetic network stats at 1 Hz.
// Used by tests and tools (fit runner). See docs/datasets.md for external traces.
class NetworkSimulator {
 public:
  NetworkSimulator();

  // Generates `seconds` samples (duration of the trace in seconds).
  std::vector<NetworkSample> generate(NetworkScenarioKind scenario,
                                      std::size_t seconds) const;

 private:
  NetworkSample make_sample(std::int64_t timestamp_ms,
                            double throughput_bps, double rtt_ms,
                            double jitter_ms, double buffer_ms,
                            double loss_rate, double decode_fps,
                            std::uint32_t dropped_frames) const;
};

}  // namespace bwe

#endif  // TEST_COMMON_NETWORK_SIMULATOR_H_
