#ifndef SRC_BWE_BWE_CORE_H_
#define SRC_BWE_BWE_CORE_H_

#include <memory>
#include <string>

#include "src/network/network_stats.h"
#include "src/bwe/signal_preprocessor.h"

namespace bwe {

enum class BweTrend {
  kStable = 0,
  kIncreasing = 1,
  kDecreasing = 2,
};

struct BweResult {
  double estimated_bitrate_bps = 0.0;
  BweTrend trend = BweTrend::kStable;
  std::string debug_tag;
};

class BweCore {
 public:
  virtual ~BweCore() = default;

  virtual void update(const NetworkSample& sample,
                      const PreprocessedSignals& signals) = 0;

  virtual BweResult estimate() const = 0;
};

enum class BweAlgorithmType {
  kBbrv2Like = 0,
};

std::unique_ptr<BweCore> create_bwe_core(BweAlgorithmType type,
                                         double initial_bitrate_bps);

}  // namespace bwe

#endif  // SRC_BWE_BWE_CORE_H_

