#ifndef SRC_BWE_SIGNAL_PREPROCESSOR_H_
#define SRC_BWE_SIGNAL_PREPROCESSOR_H_

#include <cstddef>
#include <deque>

namespace bwe {

struct EwmaConfig {
  double alpha = 0.3;  // Smoothing factor in (0, 1].
};

struct MedianFilterConfig {
  std::size_t window_size = 5;
};

struct OutlierConfig {
  double max_z_score = 3.0;
};

struct SignalPreprocessorConfig {
  EwmaConfig throughput_ewma;
  EwmaConfig rtt_ewma;
  EwmaConfig jitter_ewma;
  MedianFilterConfig throughput_median;  // window for throughput spike filtering
  MedianFilterConfig rtt_median;
  MedianFilterConfig jitter_median;
  OutlierConfig outlier;
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
  double apply_ewma(double previous, double current,
                    const EwmaConfig& config) const;

  double apply_median(std::deque<double>* window,
                      std::size_t max_window_size) const;

  bool is_outlier(double value, const std::deque<double>& window,
                  const OutlierConfig& config) const;

  SignalPreprocessorConfig config_;
  PreprocessedSignals signals_;

  std::deque<double> throughput_history_;
  std::deque<double> rtt_history_;
  std::deque<double> jitter_history_;
};

}  // namespace bwe

#endif  // SRC_BWE_SIGNAL_PREPROCESSOR_H_

