#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

RUNS="${1:-3}"
REPORT_PATH="${2:-docs/STRESS_METRICS.md}"

if [[ ! "$RUNS" =~ ^[0-9]+$ ]] || [[ "$RUNS" -lt 1 ]]; then
    echo "error: RUNS must be a positive integer" >&2
    exit 1
fi

if [[ ! -x "bin/x86_64-asm" ]]; then
    echo "error: missing binary bin/x86_64-asm; run 'make' first" >&2
    exit 1
fi

mkdir -p "$(dirname "$REPORT_PATH")"

mapfile -t fixtures < <(find examples -maxdepth 1 -type f -name 'stress_*.asm' | sort)
if [[ "${#fixtures[@]}" -eq 0 ]]; then
    echo "error: no stress fixtures found in examples/" >&2
    exit 1
fi

report_tmp="$(mktemp /tmp/iariaboot_stress_report_XXXXXX)"
trap 'rm -f "$report_tmp"' EXIT

{
    echo "# Stress Metrics Report"
    echo
    echo "Date: $(date -u +"%Y-%m-%d %H:%M:%SZ")"
    echo "Platform: $(uname -srm)"
    echo "Runner: bin/x86_64-asm"
    echo "Runs per fixture: $RUNS"
    echo
    echo "| Fixture | Source Size (bytes) | Output Size (bytes) | Avg Time (ms) | Min Time (ms) | Max Time (ms) |"
    echo "|---|---:|---:|---:|---:|---:|"

    for fixture in "${fixtures[@]}"; do
        src_size=$(stat -c%s "$fixture")
        total_ns=0
        min_ns=0
        max_ns=0
        out_size=0

        for ((i = 1; i <= RUNS; i++)); do
            out_file="$(mktemp /tmp/iariaboot_stress_bin_XXXXXX)"

            start_ns=$(date +%s%N)
            bin/x86_64-asm "$fixture" -o "$out_file" >/dev/null
            end_ns=$(date +%s%N)

            run_ns=$((end_ns - start_ns))
            total_ns=$((total_ns + run_ns))
            if [[ "$i" -eq 1 ]]; then
                min_ns=$run_ns
                max_ns=$run_ns
            else
                if [[ "$run_ns" -lt "$min_ns" ]]; then
                    min_ns=$run_ns
                fi
                if [[ "$run_ns" -gt "$max_ns" ]]; then
                    max_ns=$run_ns
                fi
            fi

            out_size=$(stat -c%s "$out_file")
            rm -f "$out_file"
        done

        avg_ns=$((total_ns / RUNS))
        avg_ms=$((avg_ns / 1000000))
        min_ms=$((min_ns / 1000000))
        max_ms=$((max_ns / 1000000))

        printf '| %s | %d | %d | %d | %d | %d |\n' \
            "$(basename "$fixture")" "$src_size" "$out_size" "$avg_ms" "$min_ms" "$max_ms"
    done
} >"$report_tmp"

mv "$report_tmp" "$REPORT_PATH"
echo "Wrote stress metrics report: $REPORT_PATH"