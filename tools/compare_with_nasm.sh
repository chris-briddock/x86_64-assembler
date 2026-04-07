#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

REPORT_PATH="${1:-docs/NASM_COMPARISON_REPORT.md}"

if [[ ! -x "bin/x86_64-asm" ]]; then
    echo "error: missing bin/x86_64-asm; run 'make' first" >&2
    exit 1
fi

if ! command -v nasm >/dev/null 2>&1; then
    echo "error: nasm not found in PATH" >&2
    exit 1
fi

if ! command -v objcopy >/dev/null 2>&1; then
    echo "error: objcopy not found in PATH" >&2
    exit 1
fi

if ! command -v readelf >/dev/null 2>&1; then
    echo "error: readelf not found in PATH" >&2
    exit 1
fi

mkdir -p "$(dirname "$REPORT_PATH")"
mapfile -t fixtures < <(find examples -maxdepth 1 -type f -name 'reference_nasm_*.asm' | sort)

if [[ "${#fixtures[@]}" -eq 0 ]]; then
    echo "error: no reference fixtures found (examples/reference_nasm_*.asm)" >&2
    exit 1
fi

report_tmp="$(mktemp /tmp/iariaboot_nasm_compare_XXXXXX)"
analysis_tmp="$(mktemp /tmp/iariaboot_nasm_analysis_XXXXXX)"
trap 'rm -f "$report_tmp" "$analysis_tmp"' EXIT

mismatch_count=0
match_count=0
error_count=0

classify_family() {
    local base="$1"

    case "$base" in
        reference_nasm_arith|reference_nasm_logic|reference_nasm_high_regs)
            echo "gpr-core"
            ;;
        reference_nasm_controlflow|reference_nasm_stack)
            echo "control-flow"
            ;;
        reference_nasm_memory|reference_nasm_data)
            echo "memory-addressing"
            ;;
        reference_nasm_shifts|reference_nasm_bitops)
            echo "bit-ops"
            ;;
        reference_nasm_sse)
            echo "sse"
            ;;
        *)
            echo "other"
            ;;
    esac
}

extract_exec_segment() {
    local elf_path="$1"
    local out_path="$2"
    local offset_hex
    local size_hex
    local vaddr_hex
    local entry_hex
    local memsz_hex
    local chosen=0
    local offset_dec
    local size_dec
    local vaddr_dec
    local entry_dec
    local memsz_dec
    local entry_off_dec
    local entry_size_dec

    entry_hex="$(readelf -h "$elf_path" | awk -F: '/Entry point address/ { gsub(/^[[:space:]]+/, "", $2); print $2; exit }')"

    while read -r offset_hex vaddr_hex size_hex memsz_hex; do
        entry_dec=$((16#${entry_hex#0x}))
        vaddr_dec=$((16#${vaddr_hex#0x}))
        memsz_dec=$((16#${memsz_hex#0x}))
        if (( entry_dec >= vaddr_dec && entry_dec < (vaddr_dec + memsz_dec) )); then
            chosen=1
            break
        fi
    done < <(readelf -lW "$elf_path" | awk '$1 == "LOAD" { print $2, $3, $5, $6 }')

    if [[ "$chosen" -ne 1 || -z "${offset_hex:-}" || -z "${vaddr_hex:-}" || -z "${size_hex:-}" || -z "${entry_hex:-}" ]]; then
        return 1
    fi

    offset_dec=$((16#${offset_hex#0x}))
    vaddr_dec=$((16#${vaddr_hex#0x}))
    size_dec=$((16#${size_hex#0x}))
    entry_dec=$((16#${entry_hex#0x}))

    if (( entry_dec < vaddr_dec )); then
        return 1
    fi

    entry_off_dec=$((offset_dec + (entry_dec - vaddr_dec)))
    entry_size_dec=$((size_dec - (entry_off_dec - offset_dec)))

    if (( entry_size_dec <= 0 )); then
        return 1
    fi

    dd if="$elf_path" of="$out_path" bs=1 skip="$entry_off_dec" count="$entry_size_dec" status=none
}

{
    echo "# NASM Comparison Report"
    echo
    echo "Date: $(date -u +"%Y-%m-%d %H:%M:%SZ")"
    echo "Platform: $(uname -srm)"
    echo "Our assembler: bin/x86_64-asm"
    echo "Reference assembler: nasm"
    echo
    echo "| Fixture | Result | Category | Our .text bytes | NASM .text bytes | First Diff | Notes |"
    echo "|---|---|---|---:|---:|---|---|"

    for fixture in "${fixtures[@]}"; do
        base="$(basename "$fixture" .asm)"
        ours_elf="$(mktemp /tmp/${base}_ours_XXXXXX)"
        nasm_obj="$(mktemp /tmp/${base}_nasm_obj_XXXXXX.o)"
        nasm_elf="$(mktemp /tmp/${base}_nasm_elf_XXXXXX)"
        nasm_ld_script="$(mktemp /tmp/${base}_nasm_ld_XXXXXX.ld)"
        ours_text="$(mktemp /tmp/${base}_ours_text_XXXXXX.bin)"
        nasm_text="$(mktemp /tmp/${base}_nasm_text_XXXXXX.bin)"

        note=""
        category=""
        first_diff="-"
        family="$(classify_family "$base")"
        first_sig="none"
        result="MATCH"

        cat >"$nasm_ld_script" <<'EOF'
ENTRY(_start)
SECTIONS {
    . = 0x400000;
    .data : {
        *(.data)
        *(.rodata)
        *(.bss)
        *(COMMON)
    }
    .text : {
        *(.text)
    }
}
EOF

        if ! bin/x86_64-asm "$fixture" -o "$ours_elf" >/dev/null 2>&1; then
            result="ERROR"
            category="tool failure"
            note="our assembler failed"
        elif ! nasm -f elf64 "$fixture" -o "$nasm_obj" >/dev/null 2>&1; then
            result="ERROR"
            category="tool failure"
            note="nasm failed"
        elif ! extract_exec_segment "$ours_elf" "$ours_text"; then
            result="ERROR"
            category="tool failure"
            note="failed to extract executable segment from our output"
        elif ! ld -e _start -T "$nasm_ld_script" -o "$nasm_elf" "$nasm_obj" >/dev/null 2>&1; then
            result="ERROR"
            category="tool failure"
            note="ld failed on nasm output"
        elif ! objcopy --only-section=.text -O binary "$nasm_elf" "$nasm_text" >/dev/null 2>&1; then
            result="ERROR"
            category="tool failure"
            note="objcopy failed on linked nasm output"
        else
            ours_size=$(stat -c%s "$ours_text")
            nasm_size=$(stat -c%s "$nasm_text")

            if cmp -s "$ours_text" "$nasm_text"; then
                result="MATCH"
                category="byte-identical"
                note="byte-identical"
                match_count=$((match_count + 1))
            else
                result="MISMATCH"
                if [[ "$ours_size" -ne "$nasm_size" ]]; then
                    category="size mismatch"
                else
                    category="byte mismatch"
                fi

                diff_line=$( (cmp -l "$ours_text" "$nasm_text" | head -n1) 2>/dev/null || true )
                if [[ -n "$diff_line" ]]; then
                    set -- $diff_line
                    diff_pos="$1"
                    ours_oct="$2"
                    nasm_oct="$3"
                    ours_hex=$(printf '0x%02x' "$((8#$ours_oct))")
                    nasm_hex=$(printf '0x%02x' "$((8#$nasm_oct))")
                    first_diff="+$((diff_pos - 1)): ${ours_hex} vs ${nasm_hex}"
                    first_sig="${ours_hex}->${nasm_hex}"
                fi

                note="different machine-code bytes"
                mismatch_count=$((mismatch_count + 1))
            fi

            printf '| %s | %s | %s | %d | %d | %s | %s |\n' \
                "$base" "$result" "$category" "$ours_size" "$nasm_size" "$first_diff" "$note"

            if [[ "$result" == "MISMATCH" ]]; then
                printf '%s|%s|%s|%s|%d|%d\n' \
                    "$base" "$family" "$category" "$first_sig" "$ours_size" "$nasm_size" >>"$analysis_tmp"
            fi

            rm -f "$ours_elf" "$nasm_obj" "$nasm_elf" "$nasm_ld_script" "$ours_text" "$nasm_text"
            continue
        fi

        ours_size=0
        nasm_size=0
        [[ -f "$ours_text" ]] && ours_size=$(stat -c%s "$ours_text")
        [[ -f "$nasm_text" ]] && nasm_size=$(stat -c%s "$nasm_text")
        printf '| %s | %s | %s | %d | %d | %s | %s |\n' \
            "$base" "$result" "$category" "$ours_size" "$nasm_size" "$first_diff" "$note"

        mismatch_count=$((mismatch_count + 1))
        error_count=$((error_count + 1))
        printf '%s|%s|%s|%s|%d|%d\n' \
            "$base" "$family" "$category" "$first_sig" "$ours_size" "$nasm_size" >>"$analysis_tmp"
        rm -f "$ours_elf" "$nasm_obj" "$nasm_elf" "$nasm_ld_script" "$ours_text" "$nasm_text"
    done

    total=$((match_count + mismatch_count))
    echo
    echo "Summary: ${match_count}/${total} fixtures byte-identical; ${mismatch_count} mismatches total (${error_count} tool errors)."

    echo
    echo "## Discrepancy Buckets"
    echo
    echo "| Family | Count | Avg Size Delta (ours-nasm bytes) |"
    echo "|---|---:|---:|"

    if [[ -s "$analysis_tmp" ]]; then
        awk -F'|' '
            {
                family = $2;
                count[family] += 1;
                delta_sum[family] += ($5 - $6);
            }
            END {
                for (f in count) {
                    avg = delta_sum[f] / count[f];
                    printf("| %s | %d | %.1f |\n", f, count[f], avg);
                }
            }
        ' "$analysis_tmp" | sort
    fi

    echo
    echo "## Maintenance Plan"
    echo
    echo "1. Keep parity locked in CI: run \`make compare-nasm-gate\` on parser/encoder/fixup/ELF layout changes."
    echo "2. Expand fixture coverage from 10 to 35+ while preserving family tags for progress visibility."
    echo "3. Keep dual validation for parity-related changes: \`make compare-nasm-gate && make test-integration\`."
    echo "4. If parity regresses, group by first-diff signature and fix the highest-frequency signature first."
} >"$report_tmp"

mv "$report_tmp" "$REPORT_PATH"
echo "Wrote NASM comparison report: $REPORT_PATH"
