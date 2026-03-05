#include "src/api/bwe_api.h"

namespace bwe {

BweEstimator::BweEstimator(const BweEstimatorConfig& config)
    : config_(config),
      preprocessor_(config.preprocessor_config),
      core_(create_bwe_core(config.algorithm_type,
                            config.initial_bitrate_bps)),
      scorer_(config.quality_config) {}

BweEstimator::~BweEstimator() = default;

void BweEstimator::update(const NetworkSample& sample) {
  // Compute the actual interval since last callback.
  // Protects against OS timer drift where "1-second" callbacks arrive early/late.
  double interval_s = 1.0;
  if (last_timestamp_ms_ > 0 && sample.timestamp_ms > last_timestamp_ms_) {
    interval_s = static_cast<double>(sample.timestamp_ms - last_timestamp_ms_) / 1000.0;
  }
  last_timestamp_ms_ = sample.timestamp_ms;

  // throughput_bps = delivery rate at receiver = received bytes * 8 / actual interval.
  // This is the raw input to the preprocessor's EWMA+median spike filter.
  double throughput_bps =
      static_cast<double>(sample.received_bytes) * 8.0 / interval_s;

  preprocessor_.update(throughput_bps, sample.rtt_ms, sample.jitter_ms);
  PreprocessedSignals signals = preprocessor_.signals();

  if (core_) {
    core_->update(sample, signals);
    last_output_.bwe_result = core_->estimate();
  }

  last_output_.quality_result =
      scorer_.score(last_output_.bwe_result, sample);
}

BweEstimatorOutput BweEstimator::last_output() const {
  return last_output_;
}

}  // namespace bwe

