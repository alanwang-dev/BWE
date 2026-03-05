#include "src/api/bwe_api.h"
#include "network_simulator.h"

#include <cassert>
#include <iostream>
#include <string>

// Helper: build an estimator with default BBRv2 config.
static bwe::BweEstimator make_estimator() {
  bwe::BweEstimatorConfig cfg;
  cfg.algorithm_type = bwe::BweAlgorithmType::kBbrv2Like;
  return bwe::BweEstimator(cfg);
}

// ---- Test 1: basic smoke test ----
static void test_basic_output_nonzero() {
  auto est = make_estimator();
  bwe::NetworkSimulator sim;
  for (const auto& s : sim.generate(bwe::NetworkScenarioKind::kSuddenCongestion, 60))
    est.update(s);
  assert(est.last_output().bwe_result.estimated_bitrate_bps > 0.0);
  std::cout << "  [PASS] basic output nonzero\n";
}

// ---- Test 2: App-Limited detection ----
// Feed 20 s of high-BW baseline, then 10 s of low delivery + stable RTT +
// low jitter + healthy buffer. Estimate should NOT drop (freeze / app-limited).
static void test_app_limited_does_not_degrade() {
  auto est = make_estimator();

  // Warm up: 20 s at 5 Mbps, RTT 40 ms, jitter 5 ms, healthy buffer.
  for (int i = 0; i < 20; ++i) {
    bwe::NetworkSample s;
    s.timestamp_ms = static_cast<int64_t>(i * 1000);
    s.received_bytes = 5'000'000 / 8;  // 5 Mbps
    s.rtt_ms = 40.0;
    s.jitter_ms = 5.0;
    s.received_buffer_ms = 8000.0;
    est.update(s);
  }
  double est_after_warmup = est.last_output().bwe_result.estimated_bitrate_bps;

  // App-limited: sender drops to 0.5 Mbps, but RTT/jitter stays clean.
  std::string last_tag;
  for (int i = 20; i < 30; ++i) {
    bwe::NetworkSample s;
    s.timestamp_ms = static_cast<int64_t>(i * 1000);
    s.received_bytes = 500'000 / 8;  // 0.5 Mbps
    s.rtt_ms = 42.0;   // still close to baseline
    s.jitter_ms = 5.0;
    s.received_buffer_ms = 9000.0;  // buffer still healthy
    est.update(s);
    last_tag = est.last_output().bwe_result.debug_tag;
  }
  double est_after_app_limited = est.last_output().bwe_result.estimated_bitrate_bps;

  // Estimate should remain close to warmup value — app-limited freezes the BWE.
  assert(est_after_app_limited >= est_after_warmup * 0.7);
  assert(last_tag == "bbrv2_app_limited");
  std::cout << "  [PASS] app-limited: estimate frozen, tag=" << last_tag << "\n";
}

// ---- Test 3: Buffer Starvation panic ----
// When buffer < 300 ms and delivery rate is falling, estimate must drop sharply.
static void test_buffer_starvation_drops_estimate() {
  auto est = make_estimator();

  // Warm up at 4 Mbps.
  for (int i = 0; i < 15; ++i) {
    bwe::NetworkSample s;
    s.timestamp_ms = static_cast<int64_t>(i * 1000);
    s.received_bytes = 4'000'000 / 8;
    s.rtt_ms = 50.0;
    s.jitter_ms = 8.0;
    s.received_buffer_ms = 8000.0;
    est.update(s);
  }
  double est_before = est.last_output().bwe_result.estimated_bitrate_bps;

  // Starvation: buffer near zero, delivery crumbling.
  std::string last_tag;
  for (int i = 15; i < 20; ++i) {
    bwe::NetworkSample s;
    s.timestamp_ms = static_cast<int64_t>(i * 1000);
    s.received_bytes = 800'000 / 8;  // 0.8 Mbps, well below estimate
    s.rtt_ms = 55.0;
    s.jitter_ms = 10.0;
    s.received_buffer_ms = 150.0;  // < 300 ms threshold
    est.update(s);
    last_tag = est.last_output().bwe_result.debug_tag;
  }
  double est_after = est.last_output().bwe_result.estimated_bitrate_bps;

  assert(est_after < est_before * 0.5);  // must drop by at least half
  assert(last_tag == "bbrv2_buffer_starvation");
  std::cout << "  [PASS] buffer starvation: estimate dropped, tag=" << last_tag << "\n";
}

// ---- Test 4: Probe-RTT (bufferbloat detection) ----
// RTT rises well above baseline → probe_rtt penalty applied.
static void test_probe_rtt_tag() {
  auto est = make_estimator();

  // Warm up with low RTT to establish rtprop baseline.
  for (int i = 0; i < 15; ++i) {
    bwe::NetworkSample s;
    s.timestamp_ms = static_cast<int64_t>(i * 1000);
    s.received_bytes = 4'000'000 / 8;
    s.rtt_ms = 30.0;
    s.jitter_ms = 5.0;
    s.received_buffer_ms = 6000.0;
    est.update(s);
  }

  // RTT balloons to > 1.5 × rtprop.
  std::string last_tag;
  for (int i = 15; i < 22; ++i) {
    bwe::NetworkSample s;
    s.timestamp_ms = static_cast<int64_t>(i * 1000);
    s.received_bytes = 4'000'000 / 8;
    s.rtt_ms = 200.0;  // >> 1.5 * 30
    s.jitter_ms = 5.0;
    s.received_buffer_ms = 6000.0;
    est.update(s);
    last_tag = est.last_output().bwe_result.debug_tag;
  }
  assert(last_tag == "bbrv2_probe_rtt");
  std::cout << "  [PASS] probe_rtt: tag=" << last_tag << "\n";
}

int main() {
  std::cout << "test_bwe_core:\n";
  test_basic_output_nonzero();
  test_app_limited_does_not_degrade();
  test_buffer_starvation_drops_estimate();
  test_probe_rtt_tag();
  std::cout << "All test_bwe_core tests passed.\n";
  return 0;
}

