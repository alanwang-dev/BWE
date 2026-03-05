#!/usr/bin/env python3
import sys
import yaml
import os
import argparse

def main():
    parser = argparse.ArgumentParser(description="Download external datasets config.")
    parser.add_argument("--all", action="store_true", help="Download all datasets")
    parser.add_argument("--id", type=str, help="Dataset ID to download")
    args, unknown = parser.parse_known_args()

    config_path = "config/datasets.yaml"
    if not os.path.exists(config_path):
        print(f"Error: {config_path} not found.")
        sys.exit(1)

    with open(config_path, "r") as f:
        config = yaml.safe_load(f)

    externals = config.get("external", [])
    
    if args.all:
        targets = externals
    elif args.id:
        targets = [d for d in externals if d["id"] == args.id]
        if not targets:
            print(f"Error: Dataset ID '{args.id}' not found.")
            sys.exit(1)
    elif unknown and len(unknown) > 0:
        # Fallback to positional argument from older script versions
        target_id = unknown[0]
        targets = [d for d in externals if d["id"] == target_id]
        if not targets:
           # Maybe they meant all
           if target_id == "all":
               targets = externals
           else:
               print(f"Error: Dataset ID '{target_id}' not found.")
               sys.exit(1)
    else:
        print("Usage: ./download.sh [source_id_or_all]")
        print("Available datasets:")
        for d in externals:
            print(f"  - {d['id']} ({d['name']})")
        sys.exit(0)

    for ds in targets:
        print(f"Mock Downloading: {ds['name']} from {ds['url']} ...")
        # In a real setup, handle url / github / huggingface appropriately
        out_dir = f"data/{ds['id']}"
        os.makedirs(out_dir, exist_ok=True)
        # Mock file creation
        mock_file = os.path.join(out_dir, "trace.csv")
        if not os.path.exists(mock_file):
            with open(mock_file, "w") as f:
                f.write("time_s,received_bytes,rtt_ms,jitter_ms\n")
                f.write("0.0,500000,40,5\n")
                f.write("1.0,520000,42,4\n")
            print(f"Created mock trace at {mock_file}")
        print(f"Done downloading {ds['id']}.\n")

if __name__ == "__main__":
    main()
