#!/usr/bin/env python3
# generate_plot.py
# Combines all fit_results_*.csv in a directory into a single evaluation image.

import os
import sys
import glob
import pandas as pd
import matplotlib.pyplot as plt

def main():
    in_dir = sys.argv[1] if len(sys.argv) > 1 else "out"
    out_file = sys.argv[2] if len(sys.argv) > 2 else "out/fit_all_results.png"

    csv_files = sorted(glob.glob(os.path.join(in_dir, "fit_results_*.csv")))
    if not csv_files:
        print(f"No CSVs found in {in_dir}.")
        return

    n = len(csv_files)
    # 2 charts per scenario, 4 cols total (2 scenarios per row)
    n_rows = (n + 1) // 2
    fig, axes = plt.subplots(n_rows, 4, figsize=(20, 5 * n_rows), squeeze=False)

    for idx, csv_path in enumerate(csv_files):
        df = pd.read_csv(csv_path)
        scenario = os.path.basename(csv_path).replace("fit_results_", "").replace(".csv", "")
        x = df['time_s'] if 'time_s' in df.columns else range(len(df))

        row = idx // 2
        col_offset = (idx % 2) * 2  # 0 or 2

        # Compute evaluation metrics
        bbrv2_col = next((c for c in ['estimate_bbrv2_bps', 'estimated_bw_bps'] if c in df.columns), None)
        if bbrv2_col and 'actual_bps' in df.columns:
            est = df[bbrv2_col]
            act = df['actual_bps']
            utilization = (est / act).mean()
            overestimate_rate = (est > act).mean() * 100
            subtitle = f"Utilization: {utilization:.2f}  |  Overestimate: {overestimate_rate:.1f}%"
        else:
            subtitle = ""

        # Column 0: Bandwidth (converted to kbps)
        ax0 = axes[row][col_offset]
        if 'actual_bps' in df.columns:
            ax0.plot(x, df['actual_bps'] / 1000, label='Actual', color='gray', linestyle='--')
        bbrv2_col = next((c for c in ['estimate_bbrv2_bps', 'estimated_bw_bps'] if c in df.columns), None)
        if bbrv2_col:
            ax0.plot(x, df[bbrv2_col] / 1000, label='BWE (BBRv2)', color='blue')
        ax0.set_title(f"{scenario} — Bandwidth\n{subtitle}", fontsize=9)
        ax0.set_ylabel('kbps')
        ax0.legend(fontsize=7)

        # Column 1: RTT / Jitter
        ax1 = axes[row][col_offset + 1]
        if 'rtt_ms' in df.columns:
            ax1.plot(x, df['rtt_ms'], label='RTT (ms)', color='orange')
        if 'jitter_ms' in df.columns:
            ax1.plot(x, df['jitter_ms'], label='Jitter (ms)', color='red')
        ax1.set_title(f"{scenario} — RTT / Jitter")
        ax1.set_ylabel('ms')
        if ax1.get_lines():
            ax1.legend(fontsize=7)

        for ax in [ax0, ax1]:
            ax.set_xlabel('time (s)')

    plt.tight_layout()
    plt.savefig(out_file, dpi=120)
    print(f"Combined plot saved as {out_file}")

if __name__ == "__main__":
    main()