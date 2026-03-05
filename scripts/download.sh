#!/bin/bash
# 
# Usage: ./scripts/download.sh [--all | --id <source>]

if [ -f ".venv/bin/activate" ]; then
    source .venv/bin/activate
fi

python3 scripts/download_datasets.py "$@"
