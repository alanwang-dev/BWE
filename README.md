---
# BBR-based Bandwidth Estimation Demo

BWE (Bandwidth Estimation) is a cross-platform C++ module for mobile devices (iOS and Android) to estimate bandwidth and network quality. Inspired by GCC and BBRv2, it processes per-second player statistics (e.g., jitter, RTT, received bytes, buffer level, etc.) to provide bandwidth and quality estimates for ABR (adaptive bitrate) and QoE (quality of experience).

## Directory Structure

- **`src/network`**: Data models and measurement collection.
- **`src/bwe`**: Signal preprocessing, core BWE algorithms, and quality scoring.
- **`test_common`**: **Test-specific synthetic data generation tools**, such as [`network_simulator.cpp`](test_common/network_simulator.cpp), which generates 1 Hz trace data for validation and unit tests.
- **`src/api`**: Public APIs for integration with media players.
- **`tests`**: Unit tests.
- **`tools`**: Includes **fitting runners** to compare simulated or recorded data against estimation results.
- **`scripts`**: Variety of tools for validation and plotting.

## Getting Started

### Downloading Datasets
Run the following command to fetch required datasets:
```bash
./scripts/download.sh [source]
```

### Generating Synthetic Data
Generate local test data for a specified duration (in seconds):
```bash
./scripts/generate_dummy.sh 300
```

### Running Tests and Plotting Results
Run the BWE simulator with specific data sources and generate output plots (`bbr_results_plot.png`):
```bash
./scripts/test.sh [source] [additional_params...]
```
Note: `scripts/plot.py` will be automatically called, so no need to run it separately.

## Compilation Instructions

The project builds with CMake and a C++17-compatible compiler:
```bash
mkdir -p build
cd build
cmake ..
cmake --build .
ctest
```
Dependencies include GoogleTest. CMake can download it automatically, or you can use a pre-installed version.

## Example Outputs

After running `test.sh`, you will find:
- **Estimated Outputs**: Bandwidth estimation graphs in PNG.
- **Log Files**: Detailed runtime logs.

## Contribution Guide

We welcome contributions! Please refer to [CONTRIBUTING.md](CONTRIBUTING.md) for details on how to contribute.
---