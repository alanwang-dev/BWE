// Fitting runner: feed simulated (or recorded) traces through BWE estimators,
// compare estimated bitrate vs actual throughput, output metrics and CSV for plotting.

#include "src/api/bwe_api.h"
#include "network_simulator.h"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr double kBytesToBps = 8.0;

bwe::NetworkScenarioKind parse_scenario(const std::string& name) {
  if (name == "stable_wifi") return bwe::NetworkScenarioKind::kStableWifi;
  if (name == "mobile") return bwe::NetworkScenarioKind::kMobileFluctuating;
  if (name == "congestion") return bwe::NetworkScenarioKind::kSuddenCongestion;
  if (name == "high_rtt") return bwe::NetworkScenarioKind::kHighRttStableBw;
  if (name == "burst_loss") return bwe::NetworkScenarioKind::kBurstLoss;
  if (name == "uplink") return bwe::NetworkScenarioKind::kUplinkCongestion;
  return bwe::NetworkScenarioKind::kStableWifi;
}

std::string scenario_name(bwe::NetworkScenarioKind k) {
  switch (k) {
    case bwe::NetworkScenarioKind::kStableWifi: return "stable_wifi";
    case bwe::NetworkScenarioKind::kMobileFluctuating: return "mobile";
    case bwe::NetworkScenarioKind::kSuddenCongestion: return "congestion";
    case bwe::NetworkScenarioKind::kHighRttStableBw: return "high_rtt";
    case bwe::NetworkScenarioKind::kBurstLoss: return "burst_loss";
    case bwe::NetworkScenarioKind::kUplinkCongestion: return "uplink";
  }
  return "unknown";
}

struct FitRow {
  double time_s = 0.0;
  double actual_bps = 0.0;
  double estimate_bbrv2_bps = 0.0;
  double rtt_ms = 0.0;
  double jitter_ms = 0.0;
};

struct FitMetrics {
  double mae_bbrv2 = 0.0;
  double rmse_bbrv2 = 0.0;
  std::size_t n = 0;
};

FitMetrics compute_metrics(const std::vector<FitRow>& rows) {
  FitMetrics m;
  m.n = rows.size();
  if (m.n == 0) return m;

  double sum_ae_bbrv2 = 0.0, sum_se_bbrv2 = 0.0;

  for (const auto& r : rows) {
    double ae_bbrv2 = std::fabs(r.actual_bps - r.estimate_bbrv2_bps);
    sum_ae_bbrv2 += ae_bbrv2;
    sum_se_bbrv2 += ae_bbrv2 * ae_bbrv2;
  }

  m.mae_bbrv2 = sum_ae_bbrv2 / static_cast<double>(m.n);
  m.rmse_bbrv2 = std::sqrt(sum_se_bbrv2 / static_cast<double>(m.n));
  return m;
}

bool write_csv(const std::string& path, const std::vector<FitRow>& rows) {
  std::ofstream out(path);
  if (!out) return false;
  out << "time_s,actual_bps,estimate_bbrv2_bps,rtt_ms,jitter_ms\n";
  for (const auto& r : rows) {
    out << std::fixed << std::setprecision(2)
        << r.time_s << "," << r.actual_bps << ","
        << r.estimate_bbrv2_bps << ","
        << r.rtt_ms << "," << r.jitter_ms << "\n";
  }
  return true;
}

// Split a CSV line into fields (by comma).
std::vector<std::string> split_csv_line(const std::string& line) {
  std::vector<std::string> out;
  std::string field;
  for (char c : line) {
    if (c == ',') {
      out.push_back(field);
      field.clear();
    } else {
      field += c;
    }
  }
  out.push_back(field);
  return out;
}

// Read trace CSV: header must include time_s and (received_bytes or actual_bps); optional rtt_ms, jitter_ms.
bool read_trace_csv(const std::string& path,
                    std::vector<bwe::NetworkSample>* trace) {
  std::ifstream in(path);
  if (!in) return false;
  trace->clear();
  std::string line;
  if (!std::getline(in, line)) return false;
  std::vector<std::string> header = split_csv_line(line);
  int col_time = -1, col_bytes = -1, col_bps = -1, col_rtt = -1, col_jitter = -1;
  for (std::size_t i = 0; i < header.size(); ++i) {
    const std::string& col = header[i];
    if (col == "time_s") col_time = static_cast<int>(i);
    else if (col == "received_bytes") col_bytes = static_cast<int>(i);
    else if (col == "actual_bps") col_bps = static_cast<int>(i);
    else if (col == "rtt_ms") col_rtt = static_cast<int>(i);
    else if (col == "jitter_ms") col_jitter = static_cast<int>(i);
  }
  if (col_time < 0 || (col_bytes < 0 && col_bps < 0)) return false;

  int max_col = col_time;
  if (col_bytes >= 0) max_col = std::max(max_col, col_bytes);
  if (col_bps >= 0) max_col = std::max(max_col, col_bps);
  if (col_rtt >= 0) max_col = std::max(max_col, col_rtt);
  if (col_jitter >= 0) max_col = std::max(max_col, col_jitter);

  while (std::getline(in, line)) {
    std::vector<std::string> fields = split_csv_line(line);
    if (static_cast<int>(fields.size()) <= max_col) continue;
    double time_s = 0.0, bps_or_bytes = 0.0, rtt = 50.0, jitter = 5.0;
    try {
      time_s = std::stod(fields[col_time]);
      bps_or_bytes = std::stod(col_bytes >= 0 ? fields[col_bytes] : fields[col_bps]);
      if (col_rtt >= 0) rtt = std::stod(fields[col_rtt]);
      if (col_jitter >= 0) jitter = std::stod(fields[col_jitter]);
    } catch (...) {
      continue;
    }
    bwe::NetworkSample s;
    s.timestamp_ms = static_cast<std::int64_t>(time_s * 1000.0);
    s.received_bytes = col_bytes >= 0 ? static_cast<std::uint64_t>(bps_or_bytes)
                                      : static_cast<std::uint64_t>(bps_or_bytes / 8.0);
    s.rtt_ms = rtt;
    s.jitter_ms = jitter;
    trace->push_back(s);
  }
  return !trace->empty();
}

}  // namespace

int main(int argc, char* argv[]) {
  std::string scenario_str = "congestion";
  std::size_t duration_s = 90;
  std::string csv_path = "fit_results.csv";
  std::string input_csv;
  bool use_input_csv = false;

  if (argc >= 4 && std::string(argv[1]) == "--csv") {
    input_csv = argv[2];
    csv_path = argv[3];
    use_input_csv = true;
  } else {
    if (argc >= 2) scenario_str = argv[1];
    if (argc >= 3) duration_s = static_cast<std::size_t>(std::atoi(argv[2]));
    if (argc >= 4) csv_path = argv[3];
  }

  std::vector<bwe::NetworkSample> trace;
  if (use_input_csv) {
    if (!read_trace_csv(input_csv, &trace)) {
      std::cerr << "Failed to read trace from " << input_csv << "\n";
      return 1;
    }
    duration_s = trace.size();
  } else {
    bwe::NetworkScenarioKind scenario = parse_scenario(scenario_str);
    bwe::NetworkSimulator sim;
    trace = sim.generate(scenario, duration_s);
  }

  bwe::BweEstimatorConfig config_bbrv2;
  config_bbrv2.algorithm_type = bwe::BweAlgorithmType::kBbrv2Like;
  config_bbrv2.initial_bitrate_bps = 3'000'000.0;

  bwe::BweEstimator estimator_bbrv2(config_bbrv2);

  std::vector<FitRow> rows;
  rows.reserve(trace.size());

  for (const auto& sample : trace) {
    estimator_bbrv2.update(sample);

    double actual_bps = static_cast<double>(sample.received_bytes) * kBytesToBps;
    FitRow row;
    row.time_s = static_cast<double>(sample.timestamp_ms) / 1000.0;
    row.actual_bps = actual_bps;
    row.estimate_bbrv2_bps =
        estimator_bbrv2.last_output().bwe_result.estimated_bitrate_bps;
    row.rtt_ms = sample.rtt_ms;
    row.jitter_ms = sample.jitter_ms;
    rows.push_back(row);
  }

  FitMetrics metrics = compute_metrics(rows);

  std::cout << "=== BWE fit results ===\n";
  if (use_input_csv)
    std::cout << "source: " << input_csv << " (trace CSV)\n";
  else
    std::cout << "scenario: " << scenario_name(parse_scenario(scenario_str))
              << " duration: " << duration_s << " s\n";
  std::cout << "samples:  " << metrics.n << "\n";
  std::cout << "BBRv2-like: MAE=" << std::fixed << std::setprecision(0) << metrics.mae_bbrv2
            << " bps  RMSE=" << metrics.rmse_bbrv2 << " bps\n";
  std::cout << "CSV: " << csv_path << "\n";

  if (!write_csv(csv_path, rows)) {
    std::cerr << "Failed to write CSV to " << csv_path << "\n";
    return 1;
  }
  return 0;
}
