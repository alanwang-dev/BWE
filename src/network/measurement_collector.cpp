#include "src/network/measurement_collector.h"

namespace bwe {

MeasurementCollector::MeasurementCollector(std::size_t max_history_size)
    : max_history_size_(max_history_size) {}

void MeasurementCollector::add_sample(const NetworkSample& sample) {
  history_.push_back(sample);
  while (history_.size() > max_history_size_) {
    history_.pop_front();
  }
}

}  // namespace bwe

