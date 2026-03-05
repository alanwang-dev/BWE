#ifndef SRC_BWE_SIGNAL_PREPROCESSOR_H_
#define SRC_BWE_SIGNAL_PREPROCESSOR_H_

#include <cstddef>
#include <deque>

namespace bwe {

struct SignalPreprocessorConfig {
  double      throughput_ewma_alpha    = 0.3;
  double      rtt_ewma_alpha           = 0.3;
  double      jitter_ewma_alpha        = 0.3;
  std::size_t throughput_median_window = 5;
  std::size_t rtt_median_window        = 5;
  std::size_t jitter_median_window     = 5;
  double      outlier_max_z_score      = 3.0;
};

struct PreprocessedSignals {
  double throughput_bps = 0.0;
  double rtt_ms = 0.0;
  double jitter_ms = 0.0;
};

class SignalPreprocessor {
 public:
  explicit SignalPreprocessor(const SignalPreprocessorConfig& config);

  // Update internal state with raw measurements.
  void update(double raw_throughput_bps, double raw_rtt_ms,
              double raw_jitter_ms);

  PreprocessedSignals signals() const { return signals_; }

 private:
  double apply_ewma(double previous, double current, double alpha) const;

  double apply_median(std::deque<double>* window,
                      std::size_t max_window_size) const;

  bool is_outlier(double value, const std::deque<double>& window,
                  double max_z_score) const;

  SignalPreprocessorConfig config_;
  PreprocessedSignals signals_;

  std::deque<double> throughput_history_;
  std::deque<double> rtt_history_;
  std::deque<double> jitter_history_;
};

}  // namespace bwe

#endif  // SRC_BWE_SIGNAL_PREPROCESSOR_H_

