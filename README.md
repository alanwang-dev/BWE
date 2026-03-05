## BBR-based Bandwidth Estimation Demo

This project provides a cross-platform C++ bandwidth and network-quality estimation
module for mobile players (iOS and Android). It is inspired by GCC and BBRv2 and
is designed to consume per-second player statistics (jitter, RTT, received bytes,
buffer level, etc.) and produce bandwidth and quality estimates for ABR and QoE.

### Layout

- `src/network`: Data models and measurement collection.
- `src/bwe`: Signal preprocessing, BWE core algorithms, quality scoring.
- `test_common`: **Test-only synthetic data** — [`network_simulator.cpp`](test_common/network_simulator.cpp) generates 1 Hz traces (with random spikes) for validation and unit tests. Not part of the production lib. Validation `--duration` = length of trace in seconds. See [docs/datasets.md](docs/datasets.md) for external datasets.
- `src/api`: Public API for integration with players.
- `tests`: Unit tests.
- `tools`: **Fitting runner** — runs BWE on simulated (or recorded) data and compares estimates to actual throughput.
- `scripts`: **Plot script** and **validation script** — run full validation (build, tests, fit, plot).

### Workflows & Scripts

All primary workflows are managed through simplified scripts in the `scripts/` directory:

1. **Download Datasets**
   Download required dataset files (if any):
   ```bash
   ./scripts/download.sh [source]
   ```

2. **Generate Dummy Data**
   Generate synthetic local test data (specify duration in seconds):
   ```bash
   ./scripts/generate_dummy.sh 300
   ```

3. **Run Tests & Plot**
   Run the BWE simulator with a given data source and automatically generate the plot (`bbr_results_plot.png`):
   ```bash
   ./scripts/test.sh [source] [additional_params...]
   ```

*(Note: `scripts/plot.py` is called automatically by `test.sh` to render the 3-pane output plot, you don't need to run it manually).*

### Building (CMake)

This repo is intended to be built with CMake and a C++17-compatible compiler:

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
ctest
```

You will need GoogleTest available. The simplest approach is to let CMake
download it as a dependency (to be wired in a future step), or you can
point CMake to an existing installation.

