#include "src/bwe/quality_scorer.h"

#include <cassert>
#include <iostream>

int main() {
  bwe::BweResult bwe_result;
  bwe_result.estimated_bitrate_bps = 2'000'000.0;

  bwe::NetworkSample sample;
  sample.received_buffer_ms = 1000.0;
  sample.rtt_ms = 300.0;
  sample.jitter_ms = 60.0;
  sample.decode_fps = 25.0;
  sample.dropped_frames = 2;
  sample.requested_bitrate_bps = 3'000'000;
  sample.stream_bitrate_bps = 2'000'000;

  bwe::QualityScorer scorer;
  bwe::QualityResult result = scorer.score(bwe_result, sample);

  assert(result.score_0_to_100 >= 0.0);
  assert(result.score_0_to_100 <= 100.0);

  std::cout << "test_quality_scorer passed\n";
  return 0;
}

