#ifndef SRC_BWE_QUALITY_SCORER_H_
#define SRC_BWE_QUALITY_SCORER_H_

#include <string>

#include "src/bwe/bwe_core.h"
#include "src/network/network_stats.h"

namespace bwe {

enum class QualityClass {
  kGood = 0,
  kFair = 1,
  kPoor = 2,
  kBad = 3,
};

enum class LimitationKind {
  kUnknown = 0,
  kNetworkLimited = 1,
  kPlayerLimited = 2,
  kUplinkLimited = 3,
};

// Selects threshold presets appropriate for live vs. buffered VOD playback.
enum class StreamingMode {
  kLive = 0,  // Low-latency live: buffer target 0.5–3 s, tight delay tolerance
  kVOD  = 1,  // Buffered on-demand: buffer target 8–20 s, relaxed tolerance
};

struct QualityConfig {
  StreamingMode mode = StreamingMode::kLive;
};

struct QualityResult {
  double score_0_to_100 = 0.0;
  QualityClass quality_class = QualityClass::kGood;
  LimitationKind limitation = LimitationKind::kUnknown;
  std::string reason;
};

class QualityScorer {
 public:
  explicit QualityScorer(const QualityConfig& config = {});
  QualityResult score(const BweResult& bwe_result,
                      const NetworkSample& sample) const;
 private:
  QualityConfig config_;
};

}  // namespace bwe

#endif  // SRC_BWE_QUALITY_SCORER_H_

