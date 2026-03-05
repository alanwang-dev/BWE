#!/usr/bin/env python3
"""
data.py — single entry point for all data operations.

Commands:
  dummy   [duration]          Generate dummy trace (default 300 s)
                              Written to data/dummy_trace/trace.csv
  list                        List available datasets from data/datasets.yaml
  download --all              Download / create all external datasets
  download --id <id>          Download / create one dataset by id
"""
import sys
import os
import argparse
import csv
import random
import yaml


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def load_config(path: str = "data/datasets.yaml"):
    if not os.path.exists(path):
        sys.exit(f"Error: {path} not found. Run from the project root.")
    with open(path) as f:
        return yaml.safe_load(f)


# ---------------------------------------------------------------------------
# dummy
# ---------------------------------------------------------------------------

def cmd_dummy(args):
    duration = args.duration
    out_dir = "data/dummy_trace"
    os.makedirs(out_dir, exist_ok=True)
    out_file = os.path.join(out_dir, "trace.csv")

    print(f"Generating dummy trace for {duration} s → {out_file}")

    base_bw  = 5_000_000   # 5 Mbps
    base_rtt = 40.0
    buffer   = 1500.0

    with open(out_file, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["time_s", "received_bytes", "rtt_ms", "jitter_ms",
                         "buffer_ms", "actual_bps"])
        for t in range(duration):
            rtt    = max(10.0, base_rtt + random.uniform(-5, 20))
            jitter = max(2.0,  random.uniform(2, 10))
            bps    = max(100_000, base_bw + random.uniform(-500_000, 500_000))

            # Simulate a network drop between t=30..45
            if 30 <= t <= 45:
                bps    *= 0.2
                rtt    += 100
                buffer  = max(100, buffer - 500)
            else:
                buffer  = min(3000, buffer + random.uniform(-100, 200))

            received_bytes = int(bps / 8)   # bytes per 1-second interval
            writer.writerow([t, received_bytes, round(rtt, 1),
                             round(jitter, 1), round(buffer, 1), int(bps)])

    print("Done.")


# ---------------------------------------------------------------------------
# list
# ---------------------------------------------------------------------------

def cmd_list(args):
    cfg      = load_config()
    external = cfg.get("external", [])

    print("Synthetic scenarios:")
    for s in cfg.get("synthetic", {}).get("scenarios", []):
        print(f"  {s['id']:<20} {s['name']}")

    print("\nExternal datasets:")
    for d in external:
        present = os.path.exists(f"data/{d['id']}/trace.csv")
        status  = "✓" if present else "✗"
        print(f"  {status} {d['id']:<22} {d['name']}")
        print(f"    {d['url']}")


# ---------------------------------------------------------------------------
# download
# ---------------------------------------------------------------------------

MOCK_ROWS = 300   # seconds of synthetic data used as placeholder

def _mock_trace(path: str):
    """Write a realistic-looking placeholder trace (same format as dummy)."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    base_bw  = 5_000_000
    base_rtt = 40.0
    buffer   = 1500.0
    with open(path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["time_s", "received_bytes", "rtt_ms", "jitter_ms",
                         "buffer_ms", "actual_bps"])
        for t in range(MOCK_ROWS):
            rtt    = max(10.0, base_rtt + random.uniform(-5, 20))
            jitter = max(2.0,  random.uniform(2, 10))
            bps    = max(100_000, base_bw + random.uniform(-500_000, 500_000))
            if 60 <= t <= 90:
                bps *= 0.25; rtt += 80
                buffer = max(100, buffer - 400)
            else:
                buffer = min(3000, buffer + random.uniform(-100, 200))
            writer.writerow([t, int(bps / 8), round(rtt, 1),
                             round(jitter, 1), round(buffer, 1), int(bps)])


def _download_one(ds: dict):
    out_dir   = f"data/{ds['id']}"
    out_trace = os.path.join(out_dir, "trace.csv")
    os.makedirs(out_dir, exist_ok=True)

    ds_type = ds.get("type", "url")
    print(f"[{ds['id']}]  {ds['name']}")
    print(f"  source : {ds['url']}  (type={ds_type})")

    if ds_type == "huggingface":
        # Attempt real download via huggingface_hub if installed
        try:
            from huggingface_hub import snapshot_download
            local = snapshot_download(repo_id=ds["url"].split("/datasets/")[-1],
                                      repo_type="dataset",
                                      local_dir=out_dir)
            print(f"  downloaded → {local}")
            return
        except ImportError:
            print("  huggingface_hub not installed — writing placeholder trace.")
        except Exception as e:
            print(f"  download failed ({e}) — writing placeholder trace.")

    elif ds_type in ("url", "github"):
        print("  Manual download required — see url above.")
        print("  Writing placeholder trace so the pipeline can run.")

    if not os.path.exists(out_trace):
        _mock_trace(out_trace)
        print(f"  placeholder → {out_trace}  ({MOCK_ROWS} rows)")
    else:
        print(f"  already present: {out_trace}")
    print()


def cmd_download(args):
    cfg      = load_config()
    external = cfg.get("external", [])

    if args.all:
        targets = external
    elif args.id:
        targets = [d for d in external if d["id"] == args.id]
        if not targets:
            ids = [d["id"] for d in external]
            sys.exit(f"Error: id '{args.id}' not found. Available: {ids}")
    else:
        print("Specify --all or --id <id>. Run 'data.sh list' to see available ids.")
        sys.exit(1)

    for ds in targets:
        _download_one(ds)


# ---------------------------------------------------------------------------
# CLI entry
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        prog="data.sh",
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    sub = parser.add_subparsers(dest="command", metavar="command")

    # dummy
    p_dummy = sub.add_parser("dummy", help="Generate dummy trace")
    p_dummy.add_argument("duration", nargs="?", type=int, default=300,
                         metavar="DURATION",
                         help="Trace length in seconds (default: 300)")

    # list
    sub.add_parser("list", help="List available datasets")

    # download
    p_dl = sub.add_parser("download", help="Download external dataset(s)")
    g = p_dl.add_mutually_exclusive_group(required=True)
    g.add_argument("--all", action="store_true", help="Download all datasets")
    g.add_argument("--id",  metavar="ID",        help="Download one dataset by id")

    args = parser.parse_args()

    if args.command == "dummy":
        cmd_dummy(args)
    elif args.command == "list":
        cmd_list(args)
    elif args.command == "download":
        cmd_download(args)
    else:
        parser.print_help()
        sys.exit(1)


if __name__ == "__main__":
    main()
