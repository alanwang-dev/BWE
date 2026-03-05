#include "src/bwe/signal_preprocessor.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

namespace bwe {

namespace {

double clamp_alpha(double alpha) {
  if (alpha <= 0.0) return 0.01;
  if (alpha > 1.0) return 1.0;
  return alpha;
}

}  // namespace

SignalPreprocessor::SignalPreprocessor(
    const SignalPreprocessorConfig& config)
    : config_(config) {}

void SignalPreprocessor::update(double raw_throughput_bps, double raw_rtt_ms,
                                double raw_jitter_ms) {
  throughput_history_.push_back(raw_throughput_bps);
  rtt_history_.push_back(raw_rtt_ms);
  jitter_history_.push_back(raw_jitter_ms);

  if (throughput_history_.size() > config_.throughput_median_window) {
    throughput_history_.pop_front();
  }
  if (rtt_history_.size() > config_.rtt_median_window) {
    rtt_history_.pop_front();
  }
  if (jitter_history_.size() > config_.jitter_median_window) {
    jitter_history_.pop_front();
  }

  double filtered_throughput = raw_throughput_bps;
  if (!is_outlier(raw_throughput_bps, throughput_history_, config_.outlier_max_z_score)) {
    double previous = signals_.throughput_bps;
    signals_.throughput_bps =
        apply_ewma(previous, filtered_throughput, config_.throughput_ewma_alpha);
  }

  double median_rtt =
      apply_median(&rtt_history_, config_.rtt_median_window);
  double median_jitter =
      apply_median(&jitter_history_, config_.jitter_median_window);

  if (!is_outlier(median_rtt, rtt_history_, config_.outlier_max_z_score)) {
    double previous = signals_.rtt_ms;
    signals_.rtt_ms = apply_ewma(previous, median_rtt, config_.rtt_ewma_alpha);
  }

  if (!is_outlier(median_jitter, jitter_history_, config_.outlier_max_z_score)) {
    double previous = signals_.jitter_ms;
    signals_.jitter_ms =
        apply_ewma(previous, median_jitter, config_.jitter_ewma_alpha);
  }
}

double SignalPreprocessor::apply_ewma(double previous, double current,
                                      double alpha) const {
  alpha = clamp_alpha(alpha);
  if (previous <= 0.0) return current;
  return alpha * current + (1.0 - alpha) * previous;
}

double SignalPreprocessor::apply_median(std::deque<double>* window,
                                        std::size_t max_window_size) const {
  if (window->empty()) return 0.0;
  if (window->size() > max_window_size) {
    while (window->size() > max_window_size) {
      window->pop_front();
    }
  }
  std::vector<double> sorted(window->begin(), window->end());
  std::sort(sorted.begin(), sorted.end());
  std::size_t mid = sorted.size() / 2;
  if (sorted.size() % 2 == 0) {
    return 0.5 * (sorted[mid - 1] + sorted[mid]);
  }
  return sorted[mid];
}

bool SignalPreprocessor::is_outlier(double value,
                                    const std::deque<double>& window,
                                    double max_z_score) const {
  if (window.size() < 3) return false;

  double sum =
      std::accumulate(window.begin(), window.end(), 0.0);
  double mean = sum / static_cast<double>(window.size());

  double variance_sum = 0.0;
  for (double v : window) {
    double diff = v - mean;
    variance_sum += diff * diff;
  }
  double variance = variance_sum / static_cast<double>(window.size());
  double stddev = std::sqrt(variance);
  if (stddev <= 0.0) return false;

  double z_score = std::fabs((value - mean) / stddev);
  return z_score > max_z_score;
}

}  // namespace bwe

