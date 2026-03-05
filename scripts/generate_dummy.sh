#!/bin/bash
# generate_dummy.sh
# 
# Usage: ./generate_dummy.sh [max_duration]

if [ -f ".venv/bin/activate" ]; then
    source .venv/bin/activate
fi

MAX_DURATION=${1:-300}

echo "Generating dummy data for $MAX_DURATION seconds..."
python3 scripts/generate_dummy_data.py "$MAX_DURATION"
