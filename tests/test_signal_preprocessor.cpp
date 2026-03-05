#include "src/bwe/signal_preprocessor.h"

#include <cassert>
#include <iostream>

int main() {
  bwe::SignalPreprocessorConfig config;
  config.throughput_ewma_alpha    = 0.5;
  config.rtt_ewma_alpha           = 0.5;
  config.jitter_ewma_alpha        = 0.5;
  config.rtt_median_window        = 5;
  config.jitter_median_window     = 5;

  bwe::SignalPreprocessor preprocessor(config);

  for (int i = 0; i < 10; ++i) {
    preprocessor.update(1'000'000.0, 50.0, 5.0);
  }
  bwe::PreprocessedSignals signals = preprocessor.signals();
  assert(signals.throughput_bps > 0.0);
  assert(signals.rtt_ms > 0.0);
  assert(signals.jitter_ms > 0.0);

  std::cout << "test_signal_preprocessor passed\n";
  return 0;
}

