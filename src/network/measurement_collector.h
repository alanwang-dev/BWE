#ifndef SRC_NETWORK_MEASUREMENT_COLLECTOR_H_
#define SRC_NETWORK_MEASUREMENT_COLLECTOR_H_

#include <cstddef>
#include <deque>

#include "src/network/network_stats.h"

namespace bwe {

// MeasurementCollector keeps a rolling history of recent NetworkSample values.
class MeasurementCollector {
 public:
  explicit MeasurementCollector(std::size_t max_history_size);

  void add_sample(const NetworkSample& sample);

  const std::deque<NetworkSample>& history() const { return history_; }

  bool empty() const { return history_.empty(); }

  std::size_t size() const { return history_.size(); }

  std::size_t max_history_size() const { return max_history_size_; }

 private:
  std::size_t max_history_size_;
  std::deque<NetworkSample> history_;
};

}  // namespace bwe

#endif  // SRC_NETWORK_MEASUREMENT_COLLECTOR_H_

