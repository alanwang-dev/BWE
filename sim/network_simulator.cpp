#include "network_simulator.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace bwe {

namespace {

constexpr double kMinThroughputBps = 100'000.0;   // 100 kbps
constexpr double kMaxThroughputBps = 5'000'000.0;  // 5000 kbps
// Typical video segment ~1.2 KB; at 5 Mbps over 1s ~520 packets.
inline std::uint32_t packets_per_second_from_throughput_bps(double bps) {
  double bytes_per_s = bps / 8.0;
  double avg_packet_bytes = 1200.0;
  return static_cast<std::uint32_t>(std::max(10.0, bytes_per_s / avg_packet_bytes));
}

double clamp_double(double value, double min_value, double max_value) {
  return std::max(min_value, std::min(max_value, value));
}

}  // namespace

NetworkSimulator::NetworkSimulator() = default;

// Generates 1 sample per second (1 Hz), matching a 1s stats callback.
std::vector<NetworkSample> NetworkSimulator::generate(
    NetworkScenarioKind scenario, std::size_t seconds) const {
  std::vector<NetworkSample> result;
  result.reserve(seconds);

  std::mt19937 rng(42);
  std::normal_distribution<double> noise_throughput(0.0, 0.08);
  std::normal_distribution<double> noise_rtt(0.0, 4.0);
  std::normal_distribution<double> noise_jitter(0.0, 2.0);
  std::uniform_real_distribution<double> uniform(0.0, 1.0);
  // Random spikes: occasional throughput dip or RTT/jitter spike (realistic outliers).
  const double kSpikeProbThroughput = 0.025;
  const double kSpikeProbRtt = 0.03;
  const double kSpikeProbJitter = 0.03;

  // Realistic base values for video streaming (1s callback).
  double throughput_bps = 4'000'000.0;  // 4 Mbps
  double rtt_ms = 35.0;
  double jitter_ms = 6.0;
  double buffer_ms = 8000.0;  // 8 s
  double loss_rate = 0.0;
  double decode_fps = 30.0;
  std::uint32_t dropped_frames = 0;

  for (std::size_t i = 0; i < seconds; ++i) {
    double t = static_cast<double>(i);

    switch (scenario) {
      case NetworkScenarioKind::kStableWifi: {
        // Home Wi-Fi: 3–6 Mbps, RTT 20–50 ms, jitter 2–12 ms.
        throughput_bps = 4'500'000.0 * (1.0 + 0.12 * noise_throughput(rng));
        rtt_ms = 28.0 + 8.0 * (1.0 + 0.3 * noise_rtt(rng));
        jitter_ms = 4.0 + 3.0 * (1.0 + 0.4 * noise_jitter(rng));
        loss_rate = 0.002;
        buffer_ms = clamp_double(buffer_ms + 80.0, 2000.0, 20000.0);
        decode_fps = 30.0;
        dropped_frames = 0;
        break;
      }
      case NetworkScenarioKind::kMobileFluctuating: {
        // 4G/5G moving: 0.5–5 Mbps, RTT 50–150 ms, jitter 10–45 ms.
        double wave = 0.5 + 0.5 * std::sin(t / 8.0);
        throughput_bps = (500'000.0 + 4'500'000.0 * wave) * (1.0 + 0.15 * noise_throughput(rng));
        rtt_ms = 70.0 + 50.0 * std::sin(t / 12.0) + noise_rtt(rng);
        jitter_ms = 15.0 + 15.0 * (1.0 + 0.3 * noise_jitter(rng));
        loss_rate = 0.015;
        buffer_ms = clamp_double(buffer_ms - 50.0 + 100.0 * wave, 500.0, 15000.0);
        decode_fps = (throughput_bps < 1e6) ? 24.0 : 30.0;
        dropped_frames = (throughput_bps < 1e6) ? 2u : 0u;
        break;
      }
      case NetworkScenarioKind::kSuddenCongestion: {
        // Good -> congested (30–60s) -> good; RTT/jitter spike in middle.
        bool in_congestion = (i > seconds / 3 && i < 2 * seconds / 3);
        if (in_congestion) {
          throughput_bps = 1'200'000.0 * (1.0 + 0.2 * noise_throughput(rng));
          rtt_ms = 180.0 + 60.0 * (1.0 + 0.2 * noise_rtt(rng));
          jitter_ms = 35.0 + 15.0 * (1.0 + 0.3 * noise_jitter(rng));
          loss_rate = 0.04;
          buffer_ms = std::max(0.0, buffer_ms - 600.0);
          decode_fps = 20.0;
          dropped_frames = 8;
        } else {
          throughput_bps = 5'000'000.0 * (1.0 + 0.08 * noise_throughput(rng));
          rtt_ms = 40.0 + noise_rtt(rng);
          jitter_ms = 6.0 + noise_jitter(rng);
          loss_rate = 0.005;
          buffer_ms = clamp_double(buffer_ms + 120.0, 0.0, 18000.0);
          decode_fps = 30.0;
          dropped_frames = 0;
        }
        break;
      }
      case NetworkScenarioKind::kHighRttStableBw: {
        // Satellite or cross-continent: RTT 300–450 ms, bandwidth stable 3–5 Mbps.
        throughput_bps = 4'000'000.0 * (1.0 + 0.1 * noise_throughput(rng));
        rtt_ms = 380.0 + 40.0 * (1.0 + 0.15 * noise_rtt(rng));
        jitter_ms = 12.0 + 5.0 * (1.0 + 0.2 * noise_jitter(rng));
        loss_rate = 0.008;
        buffer_ms = clamp_double(buffer_ms + 50.0, 3000.0, 25000.0);
        decode_fps = 30.0;
        dropped_frames = 0;
        break;
      }
      case NetworkScenarioKind::kBurstLoss: {
        // Periodic loss bursts (e.g. wireless interference).
        bool in_burst = (i % 25 >= 20);
        throughput_bps = 4'500'000.0 * (1.0 + 0.1 * noise_throughput(rng));
        if (in_burst) {
          loss_rate = 0.18;
          rtt_ms = 95.0 + 25.0 * (1.0 + 0.2 * noise_rtt(rng));
          jitter_ms = 22.0 + 10.0 * (1.0 + 0.2 * noise_jitter(rng));
        } else {
          loss_rate = 0.008;
          rtt_ms = 65.0 + noise_rtt(rng);
          jitter_ms = 10.0 + noise_jitter(rng);
        }
        buffer_ms = clamp_double(buffer_ms + 60.0, 1000.0, 16000.0);
        decode_fps = 28.0;
        dropped_frames = in_burst ? 5u : 0u;
        break;
      }
      case NetworkScenarioKind::kUplinkCongestion: {
        // Downlink still fine (4–6 Mbps), but RTT elevated (ACK delay).
        throughput_bps = 5'000'000.0 * (1.0 + 0.08 * noise_throughput(rng));
        rtt_ms = 140.0 + 40.0 * (1.0 + 0.2 * noise_rtt(rng));
        jitter_ms = 18.0 + 8.0 * (1.0 + 0.2 * noise_jitter(rng));
        loss_rate = 0.01;
        buffer_ms = clamp_double(buffer_ms + 60.0, 2000.0, 14000.0);
        decode_fps = 30.0;
        dropped_frames = 0;
        break;
      }
    }

    // Inject random spikes (throughput dips, RTT/jitter spikes) for realistic validation.
    if (uniform(rng) < kSpikeProbThroughput)
      throughput_bps *= 0.35 + 0.25 * uniform(rng);
    if (uniform(rng) < kSpikeProbRtt)
      rtt_ms += 80.0 + 60.0 * uniform(rng);
    if (uniform(rng) < kSpikeProbJitter)
      jitter_ms += 25.0 + 20.0 * uniform(rng);

    throughput_bps = clamp_double(throughput_bps, kMinThroughputBps, kMaxThroughputBps);
    rtt_ms = clamp_double(rtt_ms, 8.0, 800.0);
    jitter_ms = clamp_double(jitter_ms, 0.5, 120.0);
    double loss_rate_clamped = clamp_double(loss_rate, 0.0, 1.0);

    std::uint32_t packets_sent = packets_per_second_from_throughput_bps(throughput_bps);
    std::uint32_t packets_lost =
        static_cast<std::uint32_t>(std::round(loss_rate_clamped * packets_sent));
    if (packets_lost > packets_sent) packets_lost = packets_sent;

    NetworkSample sample = make_sample(
        static_cast<std::int64_t>(i * 1000), throughput_bps, rtt_ms,
        jitter_ms, buffer_ms, loss_rate_clamped, decode_fps, dropped_frames);
    sample.packets_received = packets_sent - packets_lost;
    sample.packets_lost = packets_lost;
    result.push_back(sample);
  }

  return result;
}

NetworkSample NetworkSimulator::make_sample(
    std::int64_t timestamp_ms, double throughput_bps, double rtt_ms,
    double jitter_ms, double buffer_ms, double loss_rate, double decode_fps,
    std::uint32_t dropped_frames) const {
  NetworkSample sample;
  sample.timestamp_ms = timestamp_ms;
  sample.received_bytes = static_cast<std::uint64_t>(throughput_bps / 8.0);
  sample.rtt_ms = rtt_ms;
  sample.jitter_ms = jitter_ms;
  sample.jitter_delay_ms = jitter_ms * 1.2;  // Typical relation for playout delay.
  sample.received_buffer_ms = buffer_ms;
  sample.decode_fps = decode_fps;
  sample.dropped_frames = dropped_frames;
  (void)loss_rate;
  return sample;
}

}  // namespace bwe
