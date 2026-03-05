#include "src/bwe/bwe_core.h"

#include <algorithm>
#include <deque>

namespace bwe {

namespace {

constexpr double kIncreaseFactor = 1.08;
constexpr double kDecreaseFactor = 0.85;
constexpr double kMinBitrateBps = 100'000.0;
constexpr double kMaxBitrateBps = 50'000'000.0;

struct WindowSample {
  double value;
  int64_t timestamp_ms;
};

class SlidingWindowMax {
 public:
  SlidingWindowMax(int64_t window_len_ms) : window_len_ms_(window_len_ms) {}

  void update(double value, int64_t timestamp_ms) {
    while (!q_.empty() && q_.back().value <= value) {
      q_.pop_back();
    }
    q_.push_back({value, timestamp_ms});
    while (!q_.empty() && q_.front().timestamp_ms < timestamp_ms - window_len_ms_) {
      q_.pop_front();
    }
  }

  double get_max() const {
    if (q_.empty()) return 0.0;
    return q_.front().value;
  }
 private:
  int64_t window_len_ms_;
  std::deque<WindowSample> q_;
};

class SlidingWindowMin {
 public:
  SlidingWindowMin(int64_t window_len_ms) : window_len_ms_(window_len_ms) {}

  void update(double value, int64_t timestamp_ms) {
    while (!q_.empty() && q_.back().value >= value) {
      q_.pop_back();
    }
    q_.push_back({value, timestamp_ms});
    while (!q_.empty() && q_.front().timestamp_ms < timestamp_ms - window_len_ms_) {
      q_.pop_front();
    }
  }

  double get_min() const {
    if (q_.empty()) return 0.0;
    return q_.front().value;
  }
 private:
  int64_t window_len_ms_;
  std::deque<WindowSample> q_;
};

class Bbrv2BweCore : public BweCore {
 public:
  explicit Bbrv2BweCore(double initial_bitrate_bps)
      : max_bw_filter_(10'000),  // 10 sec window
        min_rtt_filter_(10'000), // 10 sec window
        current_bitrate_bps_(initial_bitrate_bps),
        last_timestamp_ms_(0) {}

  void update(const NetworkSample& sample,
              const PreprocessedSignals& signals) override {
    // 1. Calculate precise interval to avoid timer jitter artifacts
    double interval_s = 1.0;
    if (last_timestamp_ms_ > 0 && sample.timestamp_ms > last_timestamp_ms_) {
      interval_s = static_cast<double>(sample.timestamp_ms - last_timestamp_ms_) / 1000.0;
    }
    last_timestamp_ms_ = sample.timestamp_ms;
    
    // Safety clamp: ignore huge gaps (e.g., app went to background)
    if (interval_s > 5.0 || interval_s < 0.1) {
      return; 
    }

    // 2. Calculate Delivery Rate.
    // Prefer signals.throughput_bps (EWMA-smoothed by preprocessor — spike-filtered)
    // so single-packet-storm outliers don't pollute the BtlBw sliding-window max.
    // Fall back to raw bytes if preprocessor hasn't warmed up yet.
    double delivery_rate_bps = 0.0;
    if (signals.throughput_bps > 0.0) {
      delivery_rate_bps = signals.throughput_bps;
    } else if (sample.received_bytes > 0) {
      delivery_rate_bps = static_cast<double>(sample.received_bytes) * 8.0 / interval_s;
    }

    // Raw (un-smoothed) delivery rate for emergency checks — EWMA lags by design
    // and would mask sudden starvation events if used directly.
    double raw_delivery_rate_bps = (sample.received_bytes > 0)
        ? static_cast<double>(sample.received_bytes) * 8.0 / interval_s
        : delivery_rate_bps;
    
    // 3. Identify App-Limited state (Downlink underuse but no congestion signs)
    double rtt_ms = signals.rtt_ms > 0.0 ? signals.rtt_ms : sample.rtt_ms;
    double rtprop_ms = min_rtt_filter_.get_min();
    
    bool is_app_limited = false;
    double prev_btl_bw = max_bw_filter_.get_max();

    if (rtprop_ms > 0.0 && rtt_ms > 0.0) {
      bool stable_rtt = rtt_ms <= rtprop_ms * 1.2 + 20.0; // RTT is close to min RTT
      bool stable_jitter = signals.jitter_ms < 30.0;      // Low jitter indicates no queue build-up
      bool healthy_buffer = sample.received_buffer_ms > 500.0; // Buffer is not starving

      if (delivery_rate_bps < prev_btl_bw * 0.8 && stable_rtt && stable_jitter && healthy_buffer) {
        // Bandwidth decreased purely because sender stopped sending as much data
        // Freeze BWE estimate instead of degrading it.
        is_app_limited = true;
      }
    }

    // Update filters
    if (rtt_ms > 0.0) {
      min_rtt_filter_.update(rtt_ms, sample.timestamp_ms);
      rtprop_ms = min_rtt_filter_.get_min(); // refresh min RTT
    }

    if (delivery_rate_bps > 0.0 && !is_app_limited) {
      max_bw_filter_.update(delivery_rate_bps, sample.timestamp_ms);
    }
    
    double btl_bw_bps = max_bw_filter_.get_max();
    if (btl_bw_bps <= 0.0) btl_bw_bps = current_bitrate_bps_;

    // 3. Compute safe bitrate with bounds
    double headroom_factor = 0.85;
    double safe_rate_bps = btl_bw_bps * headroom_factor;

    // Clamp: do not let the max-filter lag cause a massive overestimate.
    // Cap at delivery_rate * 1.10 — 10% headroom covers single-sample jitter
    // without letting estimate overshoot actual during sinusoidal BW drops.
    if (delivery_rate_bps > 0.0) {
      double delivery_ceil = delivery_rate_bps * 1.10;
      safe_rate_bps = std::min(safe_rate_bps, delivery_ceil);
    }

    // Detect if we are standing in queue / bufferbloat (increase in RTT)
    if (sample.received_buffer_ms > 0 && sample.received_buffer_ms < 300.0 && raw_delivery_rate_bps < current_bitrate_bps_) {
      // 🚨 CRITICAL: Player buffer is starving (e.g., < 300ms) and we are not receiving enough data.
      // This means the network is hard-failing (possibly deep packet loss, not just queuing).
      // We must aggressively back off to force ABR to drop quality and save the playback.
      // Use raw delivery rate (not EWMA) — EWMA lags too much to detect sudden starvation.
      safe_rate_bps = raw_delivery_rate_bps * 0.8;
      debug_tag_ = "bbrv2_buffer_starvation";
      current_bitrate_bps_ = std::clamp(safe_rate_bps, kMinBitrateBps, kMaxBitrateBps);
    } else if (rtt_ms > 0.0 && rtprop_ms > 0.0 && rtt_ms > 1.5 * rtprop_ms) {
      safe_rate_bps *= 0.85; // Drain Queue
      debug_tag_ = "bbrv2_probe_rtt";
      current_bitrate_bps_ = std::clamp(safe_rate_bps, kMinBitrateBps, kMaxBitrateBps);
    } else if (is_app_limited) {
      // Sender stopped sending — do NOT update the BWE estimate.
      // Freeze current_bitrate_bps_ so ABR isn't punished for sender underuse.
      debug_tag_ = "bbrv2_app_limited";
    } else {
      debug_tag_ = "bbrv2_bandwidth";
      current_bitrate_bps_ = std::clamp(safe_rate_bps, kMinBitrateBps, kMaxBitrateBps);
    }

    // Trend reporting for Player
    if (delivery_rate_bps > current_bitrate_bps_) {
      trend_ = BweTrend::kIncreasing;
    } else if (delivery_rate_bps < current_bitrate_bps_ * 0.9) {
      trend_ = BweTrend::kDecreasing;
    } else {
      trend_ = BweTrend::kStable;
    }
  }

  BweResult estimate() const override {
    BweResult result;
    result.estimated_bitrate_bps = current_bitrate_bps_;
    result.trend = trend_;
    result.debug_tag = debug_tag_;
    return result;
  }

 private:
  SlidingWindowMax max_bw_filter_;
  SlidingWindowMin min_rtt_filter_;
  double current_bitrate_bps_;
  BweTrend trend_ = BweTrend::kStable;
  std::string debug_tag_ = "bbrv2_init";
  int64_t last_timestamp_ms_;
};

}  // namespace

std::unique_ptr<BweCore> create_bwe_core(BweAlgorithmType type,
                                         double initial_bitrate_bps) {
  if (type == BweAlgorithmType::kBbrv2Like) {
    return std::make_unique<Bbrv2BweCore>(initial_bitrate_bps);
  }
  return nullptr;
}

}  // namespace bwe

