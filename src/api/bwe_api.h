#ifndef SRC_API_BWE_API_H_
#define SRC_API_BWE_API_H_

#include <cstdint>
#include <memory>

#include "src/bwe/bwe_core.h"
#include "src/bwe/quality_scorer.h"
#include "src/bwe/signal_preprocessor.h"

namespace bwe {

struct BweEstimatorConfig {
  BweAlgorithmType algorithm_type = BweAlgorithmType::kBbrv2Like;
  double initial_bitrate_bps = 3'000'000.0;
  SignalPreprocessorConfig preprocessor_config;
  QualityConfig quality_config;  // sets StreamingMode (kLive or kVOD)
};

struct BweEstimatorOutput {
  BweResult bwe_result;
  QualityResult quality_result;
};

class BweEstimator {
 public:
  explicit BweEstimator(const BweEstimatorConfig& config);
  ~BweEstimator();

  void update(const NetworkSample& sample);

  BweEstimatorOutput last_output() const;

 private:
  BweEstimatorConfig config_;
  SignalPreprocessor preprocessor_;
  std::unique_ptr<BweCore> core_;
  QualityScorer scorer_;
  BweEstimatorOutput last_output_;
  int64_t last_timestamp_ms_ = 0;  // tracks previous sample time for interval
};

}  // namespace bwe

#endif  // SRC_API_BWE_API_H_

