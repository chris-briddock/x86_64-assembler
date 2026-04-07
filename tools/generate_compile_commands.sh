#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

json_escape() {
    printf '%s' "$1" | sed -e 's/\\/\\\\/g' -e 's/"/\\"/g'
}

rm -f compile_commands.json

if command -v bear >/dev/null 2>&1; then
    # Build translation units used by the main binary and tests.
    bear --output compile_commands.json -- make -B all test-unit test-disassembler test-integration >/dev/null
    echo "Generated with bear: $ROOT_DIR/compile_commands.json"
    exit 0
fi

# Fallback path for environments without bear:
# derive compile commands from a dry-run build transcript.
dry_run_log="$(mktemp)"
trap 'rm -f "$dry_run_log"' EXIT
make -n -B all test-unit test-disassembler test-integration >"$dry_run_log"

mapfile -t compile_cmds < <(
    awk '/(^|[[:space:]])(gcc|clang|cc)([[:space:]]|$)/ && / -c / && / -o / { print }' "$dry_run_log" |
        awk '!seen[$0]++'
)

if [[ "${#compile_cmds[@]}" -eq 0 ]]; then
    echo "error: could not derive compile commands from make dry-run output" >&2
    exit 1
fi

{
    echo "["
    first_entry=1
    for i in "${!compile_cmds[@]}"; do
        cmd="${compile_cmds[$i]}"
        src="$(printf '%s\n' "$cmd" | sed -E 's/.* -c -o [^ ]+ ([^ ]+)$/\1/')"
        if [[ -z "$src" || "$src" == "$cmd" ]]; then
            continue
        fi

        esc_dir="$(json_escape "$ROOT_DIR")"
        esc_cmd="$(json_escape "$cmd")"
        esc_src="$(json_escape "$src")"

        if [[ "$first_entry" -eq 0 ]]; then
            echo ","
        fi
        printf '  {"directory":"%s","command":"%s","file":"%s"}' "$esc_dir" "$esc_cmd" "$esc_src"
        first_entry=0
    done
    echo
    echo "]"
} >compile_commands.json

echo "Generated: $ROOT_DIR/compile_commands.json"
