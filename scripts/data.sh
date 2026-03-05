#!/bin/bash
# data.sh — all data operations in one place.
#
# Usage:
#   ./scripts/data.sh dummy [duration]       Generate dummy trace (default 300 s)
#   ./scripts/data.sh list                   List available datasets
#   ./scripts/data.sh download --all         Download all external datasets
#   ./scripts/data.sh download --id <id>     Download one dataset by id

[ -f ".venv/bin/activate" ] && source .venv/bin/activate
python3 scripts/data.py "$@"
