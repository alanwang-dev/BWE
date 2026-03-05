#include "src/bwe/quality_scorer.h"

#include <algorithm>

namespace bwe {

QualityScorer::QualityScorer(const QualityConfig& config) : config_(config) {}

QualityResult QualityScorer::score(const BweResult& bwe_result,
                                   const NetworkSample& sample) const {
  QualityResult result;
  const bool is_live = (config_.mode == StreamingMode::kLive);

  // Buffer thresholds differ between live and VOD.
  // Live: buffer target is 0.5–3 s; < 500 ms is critical, < 1000 ms is warning.
  // VOD:  buffer target is 8–20 s; < 2000 ms is critical, < 5000 ms is warning.
  const double kBufCritical = is_live ? 500.0  : 2000.0;
  const double kBufWarning  = is_live ? 1000.0 : 5000.0;

  double rebuffer_penalty = 0.0;
  if (sample.received_buffer_ms < kBufCritical) {
    rebuffer_penalty = 40.0;
  } else if (sample.received_buffer_ms < kBufWarning) {
    rebuffer_penalty = 20.0;
  }

  // Delay tolerance: live is tighter (interaction quality matters more).
  const double kRttCritical = is_live ? 250.0 : 400.0;
  const double kRttWarning  = is_live ? 150.0 : 250.0;

  double delay_penalty = 0.0;
  if (sample.rtt_ms > kRttCritical) {
    delay_penalty = 30.0;
  } else if (sample.rtt_ms > kRttWarning) {
    delay_penalty = 15.0;
  }

  double jitter_penalty = 0.0;
  if (sample.jitter_ms > 50.0) {
    jitter_penalty = 20.0;
  } else if (sample.jitter_ms > 20.0) {
    jitter_penalty = 10.0;
  }

  double player_penalty = 0.0;
  if (sample.decode_fps < 24.0 || sample.dropped_frames > 5) {
    player_penalty = 20.0;
  }

  // Base is 100; subtract penalties to get final score.
  double total_penalty =
      rebuffer_penalty + delay_penalty + jitter_penalty + player_penalty;
  double score = std::clamp(100.0 - total_penalty, 0.0, 100.0);
  result.score_0_to_100 = score;

  if (score >= 80.0) {
    result.quality_class = QualityClass::kGood;
  } else if (score >= 60.0) {
    result.quality_class = QualityClass::kFair;
  } else if (score >= 40.0) {
    result.quality_class = QualityClass::kPoor;
  } else {
    result.quality_class = QualityClass::kBad;
  }

  double bwe_bps = bwe_result.estimated_bitrate_bps;
  bool likely_network_limited =
      (rebuffer_penalty > 0.0 || delay_penalty > 0.0 ||
       jitter_penalty > 0.0) &&
      (sample.requested_bitrate_bps > 0 &&
       bwe_bps < 0.8 * static_cast<double>(sample.requested_bitrate_bps));

  bool likely_player_limited =
      (player_penalty > 0.0) &&
      (delay_penalty == 0.0 && jitter_penalty == 0.0);

  bool likely_uplink_limited = false;
  if (sample.requested_bitrate_bps > 0 && sample.stream_bitrate_bps > 0) {
    double ratio = static_cast<double>(sample.stream_bitrate_bps) /
                   static_cast<double>(sample.requested_bitrate_bps);
    if (ratio < 0.7 && delay_penalty == 0.0 && jitter_penalty == 0.0) {
      likely_uplink_limited = true;
    }
  }

  if (likely_network_limited) {
    result.limitation = LimitationKind::kNetworkLimited;
    result.reason = "network_limited";
  } else if (likely_player_limited) {
    result.limitation = LimitationKind::kPlayerLimited;
    result.reason = "player_limited";
  } else if (likely_uplink_limited) {
    result.limitation = LimitationKind::kUplinkLimited;
    result.reason = "uplink_limited";
  } else {
    result.limitation = LimitationKind::kUnknown;
    result.reason = "unknown";
  }

  return result;
}

}  // namespace bwe
