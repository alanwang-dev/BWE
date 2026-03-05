#include "network_simulator.h"

#include <cassert>
#include <iostream>

int main() {
  bwe::NetworkSimulator simulator;

  // ----- Test every scenario -----
  const bwe::NetworkScenarioKind scenarios[] = {
    bwe::NetworkScenarioKind::kStableWifi,
    bwe::NetworkScenarioKind::kMobileFluctuating,
    bwe::NetworkScenarioKind::kSuddenCongestion,
    bwe::NetworkScenarioKind::kHighRttStableBw,
    bwe::NetworkScenarioKind::kBurstLoss,
    bwe::NetworkScenarioKind::kUplinkCongestion,
  };

  for (auto scenario : scenarios) {
    constexpr std::size_t kDuration = 30;
    auto samples = simulator.generate(scenario, kDuration);

    // Correct sample count.
    assert(samples.size() == kDuration);

    int64_t prev_ts = -1;
    for (const auto& s : samples) {
      // Timestamps must be strictly increasing.
      assert(s.timestamp_ms > prev_ts);
      prev_ts = s.timestamp_ms;

      // Must deliver some bytes every second.
      assert(s.received_bytes > 0);

      // RTT must be a physically plausible value [1, 800] ms.
      assert(s.rtt_ms >= 1.0 && s.rtt_ms <= 800.0);

      // Jitter must be non-negative.
      assert(s.jitter_ms >= 0.0);

      // Buffer level must be non-negative.
      assert(s.received_buffer_ms >= 0.0);

      // Packet stats must be consistent: lost ≤ received+lost.
      assert(s.packets_lost <= s.packets_received + s.packets_lost);
    }
  }

  std::cout << "test_network_simulator passed (all scenarios, content validated)\n";
  return 0;
}

