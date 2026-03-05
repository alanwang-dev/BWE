# Data sources for BWE validation

## Single source of truth: data/datasets.yaml

All data source **ids**, **names**, and **urls** are defined in [`data/datasets.yaml`](datasets.yaml). Two scripts read this config so they stay in sync:

- **scripts/download_datasets.py** — `--list`, `--id <id>`, `--all` to download external sources to `data/<id>/`.
- **scripts/run_all_fit_and_plot.py** and **validate.sh** — `--source all` or `--source <id>` to run fit on a given data source.

List valid ids: `python3 scripts/run_all_fit_and_plot.py --list`.

## Duration and synthetic data

- **`duration`** (e.g. in `validate.sh --duration 60`) is the **length of the generated trace in seconds**. The simulator produces **one sample per second** (1 Hz).
- **Synthetic (test-only)**: [`sim/network_simulator.cpp`](../sim/network_simulator.cpp). Scenario ids: see `synthetic.scenarios` in `data/datasets.yaml`.

## External datasets (defined in data/datasets.yaml)

| id | Description |
|----|-------------|
| bytedance_bwe | ByteDance Bandwidth Estimation Dataset (Douyin, 51-dim). Hugging Face. |
| mmsys2024_bwe | ACM MMSys 2024 BWE Challenge. |
| microsoft_rl4bwe | Microsoft RL4BandwidthEstimationChallenge (GitHub). |
| webrtc_testbed | WebRTC / C3Lab Testbed. |

To use an external source: run `python3 scripts/download_datasets.py --id <id>`, then place a trace CSV at `data/<id>/trace.csv` with columns `time_s`, `received_bytes` (or `actual_bps`), and optionally `rtt_ms`, `jitter_ms`. Then run validation with `--source <id>`.
