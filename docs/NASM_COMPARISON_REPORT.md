# NASM Comparison Report

Date: 2026-04-07 12:22:13Z
Platform: Linux 6.19.8-200.fc43.x86_64 x86_64
Our assembler: bin/x86_64-asm
Reference assembler: nasm

| Fixture | Result | Category | Our .text bytes | NASM .text bytes | First Diff | Notes |
|---|---|---|---:|---:|---|---|
| reference_nasm_arith2 | MATCH | byte-identical | 50 | 50 | - | byte-identical |
| reference_nasm_arith3 | MATCH | byte-identical | 45 | 45 | - | byte-identical |
| reference_nasm_arith | MATCH | byte-identical | 44 | 44 | - | byte-identical |
| reference_nasm_bitops2 | MATCH | byte-identical | 35 | 35 | - | byte-identical |
| reference_nasm_bitops3 | MATCH | byte-identical | 47 | 47 | - | byte-identical |
| reference_nasm_bitops4 | MATCH | byte-identical | 25 | 25 | - | byte-identical |
| reference_nasm_bitops | MATCH | byte-identical | 43 | 43 | - | byte-identical |
| reference_nasm_controlflow2 | MATCH | byte-identical | 41 | 41 | - | byte-identical |
| reference_nasm_controlflow3 | MATCH | byte-identical | 39 | 39 | - | byte-identical |
| reference_nasm_controlflow4 | MATCH | byte-identical | 34 | 34 | - | byte-identical |
| reference_nasm_controlflow | MATCH | byte-identical | 37 | 37 | - | byte-identical |
| reference_nasm_data2 | MATCH | byte-identical | 21 | 21 | - | byte-identical |
| reference_nasm_data3 | MATCH | byte-identical | 26 | 26 | - | byte-identical |
| reference_nasm_data | MATCH | byte-identical | 42 | 42 | - | byte-identical |
| reference_nasm_high_regs | MATCH | byte-identical | 46 | 46 | - | byte-identical |
| reference_nasm_logic2 | MATCH | byte-identical | 45 | 45 | - | byte-identical |
| reference_nasm_logic3 | MATCH | byte-identical | 50 | 50 | - | byte-identical |
| reference_nasm_logic4 | MATCH | byte-identical | 46 | 46 | - | byte-identical |
| reference_nasm_logic | MATCH | byte-identical | 48 | 48 | - | byte-identical |
| reference_nasm_memory2 | MATCH | byte-identical | 31 | 31 | - | byte-identical |
| reference_nasm_memory3 | MATCH | byte-identical | 31 | 31 | - | byte-identical |
| reference_nasm_memory4 | MATCH | byte-identical | 42 | 42 | - | byte-identical |
| reference_nasm_memory | MATCH | byte-identical | 30 | 30 | - | byte-identical |
| reference_nasm_shifts2 | MATCH | byte-identical | 43 | 43 | - | byte-identical |
| reference_nasm_shifts3 | MATCH | byte-identical | 48 | 48 | - | byte-identical |
| reference_nasm_shifts4 | MATCH | byte-identical | 30 | 30 | - | byte-identical |
| reference_nasm_shifts | MATCH | byte-identical | 33 | 33 | - | byte-identical |
| reference_nasm_sse2 | MATCH | byte-identical | 38 | 38 | - | byte-identical |
| reference_nasm_sse3 | MATCH | byte-identical | 41 | 41 | - | byte-identical |
| reference_nasm_sse4 | MATCH | byte-identical | 35 | 35 | - | byte-identical |
| reference_nasm_sse | MATCH | byte-identical | 45 | 45 | - | byte-identical |
| reference_nasm_stack2 | MATCH | byte-identical | 35 | 35 | - | byte-identical |
| reference_nasm_stack3 | MATCH | byte-identical | 40 | 40 | - | byte-identical |
| reference_nasm_stack4 | MATCH | byte-identical | 15 | 15 | - | byte-identical |
| reference_nasm_stack | MATCH | byte-identical | 32 | 32 | - | byte-identical |

Summary: 35/35 fixtures byte-identical; 0 mismatches total (0 tool errors).

## Discrepancy Buckets

| Family | Count | Avg Size Delta (ours-nasm bytes) |
|---|---:|---:|

## Maintenance Plan

1. Keep parity locked in CI: run `make compare-nasm-gate` on parser/encoder/fixup/ELF layout changes.
2. Expand fixture coverage from 10 to 35+ while preserving family tags for progress visibility.
3. Keep dual validation for parity-related changes: `make compare-nasm-gate && make test-integration`.
4. If parity regresses, group by first-diff signature and fix the highest-frequency signature first.
