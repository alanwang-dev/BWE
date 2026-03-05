#!/bin/bash
# test.sh
# 
# Usage: ./test.sh [dataset] [duration]
#    or: ./test.sh [duration]

if [ -f ".venv/bin/activate" ]; then
    source .venv/bin/activate
fi

if [ "$1" == "-h" ] || [ "$1" == "--help" ] || [ "$1" == "help" ]; then
    echo "Usage: ./scripts/test.sh <source> [duration]"
    echo ""
    echo "Arguments:"
    echo "  source:    Required. Either:"
    echo "               - a duration in seconds to run ALL synthetic scenarios (e.g., 90)"
    echo "               - an external dataset ID (e.g., mmsys2024_bwe)"
    echo "  duration:  (Optional) Seconds to run, only when source is a dataset ID."
    echo "             Default: 90."
    echo ""
    echo "Examples:"
    echo "  ./scripts/test.sh 60                # Run ALL synthetic scenarios for 60s"
    echo "  ./scripts/test.sh mmsys2024_bwe     # Run specific external dataset"
    echo "  ./scripts/test.sh mmsys2024_bwe 120 # Run specific external dataset for 120s"
    exit 0
fi

if [ -z "$1" ]; then
    echo "Error: source is required. Run './scripts/test.sh --help' for usage."
    exit 1
fi

# Determine if first argument is a number (duration -> run all) or a dataset ID
if [[ "$1" =~ ^[0-9]+$ ]]; then
    SOURCE="all"
    DURATION=$1
else
    SOURCE=$1
    DURATION=${2:-90}
fi

# Ensure it's built
if [ ! -f "build/tools/bwe_fit_runner" ]; then
    echo "Building project first..."
    mkdir -p build && cd build && cmake .. && make -j4 && cd ..
fi

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_DIR="out/result_${TIMESTAMP}"
mkdir -p "$RESULT_DIR"

run_single_scenario() {
    local SCEN=$1
    local DUR=$2
    local OUTPUT_CSV="${RESULT_DIR}/fit_results_${SCEN}.csv"

    echo "----------------------------------------"
    echo "Running data source: $SCEN (Duration: ${DUR}s)"

    if [ -f "data/$SCEN/trace.csv" ]; then
        ./build/tools/bwe_fit_runner --csv "data/$SCEN/trace.csv" "$OUTPUT_CSV"
    elif [ -f "$SCEN" ]; then
        ./build/tools/bwe_fit_runner --csv "$SCEN" "$OUTPUT_CSV"
    else
        ./build/tools/bwe_fit_runner "$SCEN" "$DUR" "$OUTPUT_CSV"
    fi
}

RESULT_PNG="${RESULT_DIR}/fit_all_results.png"

if [ "$SOURCE" == "all" ] || [ "$SOURCE" == "--all" ]; then
    echo "Running ALL default synthetic scenarios..."
    SCENARIOS=("stable_wifi" "mobile" "congestion" "high_rtt" "burst_loss" "uplink")
    for s in "${SCENARIOS[@]}"; do
        run_single_scenario "$s" "$DURATION"
    done
    echo "----------------------------------------"

    echo "Generating combined fit_all_results.png..."
    python3 scripts/generate_plot.py "$RESULT_DIR" "$RESULT_PNG"
else
    run_single_scenario "$SOURCE" "$DURATION"
    python3 scripts/generate_plot.py "$RESULT_DIR" "$RESULT_PNG"
fi

cp "$RESULT_PNG" "out/latest_result.png"
echo "Done! Results in $RESULT_DIR"
echo "      Latest:  out/latest_result.png"
