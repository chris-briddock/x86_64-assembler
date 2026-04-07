# Execution Task Board

Last updated: 2026-04-07
Source roadmap: ROADMAP.md

Purpose: turn roadmap goals into a phase-by-phase, trackable execution board with clear owners, dates, dependencies, and done criteria.

## Status Key

- [ ] Not started
- [~] In progress
- [x] Completed
- [!] Blocked

## Working Rules

1. Implement first.
2. Add unit plus integration tests next.
3. Update examples plus documentation last.
4. Merge only when all acceptance checks for the task are met.
5. Keep one critical-path task active at a time and at most one parallel support task.

## Current Snapshot (from roadmap)

- Phase 1: Completed for SSE, SIB edge hardening, and DWARF milestones.
- Phase 2: Completed; diagnostics normalization and high-8-bit constraints are implemented and validated.
- Phase 3: Completed; listing, disassembler, and parser-performance tracks are implemented and validated.
- Phase 4: In progress; WS1 is complete, WS2 is in progress, and WS3 has started.

## Timeline Baseline

- Sprint A (2026-04-06 to 2026-04-12): Phase 2 parser plus encoder diagnostics completion.
- Sprint B (2026-04-13 to 2026-04-19): Phase 2 high-8-bit constraints plus docs.
- Sprint C (2026-04-20 to 2026-04-26): Phase 3 listing feature end-to-end.
- Sprint D (2026-04-27 to 2026-05-03): Phase 3 disassembler baseline.
- Sprint E (2026-05-04 to 2026-05-10): Phase 3 parser performance optimization.
- Sprint F (2026-05-11 to 2026-05-24): Phase 4 hardening, cross-reference validation, release gate.

## Phase 2: Diagnostics and Polish

| ID | Task | Status | Owner | Start | Due | Depends On | Done Criteria |
|---|---|---|---|---|---|---|---|
| P2-01 | Normalize remaining parser diagnostics (all error paths) | [x] | Active | 2026-04-06 | 2026-04-09 | None | No legacy parser error format remains; parser tests pass |
| P2-02 | Normalize remaining encoder diagnostics (all error paths) | [x] | Active | 2026-04-07 | 2026-04-11 | P2-01 (partial parallel allowed) | No legacy encoder error format remains; encoder tests pass |
| P2-03 | Add did-you-mean suggestions for unknown instructions | [x] | Active | 2026-04-09 | 2026-04-11 | P2-01 | Suggestions appear in diagnostics and have tests |
| P2-04 | Add label-not-found suggestions (closest labels) | [x] | Active | 2026-04-10 | 2026-04-12 | P2-01 | Missing-label diagnostics include actionable alternatives |
| P2-05 | Expand negative parser message-position coverage | [x] | Active | 2026-04-08 | 2026-04-12 | P2-01 | 50+ parser negative tests with line and column assertions |
| P2-06 | Expand negative encoder diagnostics coverage | [x] | Active | 2026-04-09 | 2026-04-12 | P2-02 | 30+ encoder negative tests with exact message fragments |
| P2-07 | Complete AH/BH/CH/DH plus REX conflict matrix tests | [x] | Active | 2026-04-13 | 2026-04-16 | P2-02 | 20+ conflict tests across MOV/ALU/XCHG/CMP/TEST |
| P2-08 | Document high-8-bit architectural constraints | [x] | Active | 2026-04-16 | 2026-04-19 | P2-07 | Docs explain ISA limitation and workarounds clearly |
| P2-09 | Phase 2 quality gate | [x] | Active | 2026-04-18 | 2026-04-19 | P2-01..P2-08 | make test-parser, make test-encoder, make test-integration all pass |

### Phase 2 Checklist

- [x] Parser diagnostics all use normalized structure.
- [x] Encoder diagnostics all use normalized structure.
- [x] Negative path tests expanded and passing.
- [x] High-8-bit plus REX conflicts documented and fully tested.
- [x] Documentation synchronized with actual diagnostics.

## Phase 3: Advanced Features

| ID | Task | Status | Owner | Start | Due | Depends On | Done Criteria |
|---|---|---|---|---|---|---|---|
| P3-01 | Listing format design doc | [x] | Active | 2026-04-06 | 2026-04-07 | P2-09 | docs listing spec completed with examples |
| P3-02 | Implement listing CLI option and writer | [x] | Active | 2026-04-06 | 2026-04-06 | P3-01 | listing file generated with source, address, bytes |
| P3-03 | Add listing integration tests | [x] | Active | 2026-04-06 | 2026-04-06 | P3-02 | 5+ listing tests pass and cross-check bytes |
| P3-04 | Disassembler design doc | [x] | Active | 2026-04-06 | 2026-04-06 | P3-03 | decoding scope and output format documented |
| P3-05 | Build opcode lookup plus core decode families | [x] | Active | 2026-04-06 | 2026-05-01 | P3-04 | common families decode correctly |
| P3-06 | Implement disassemble CLI mode | [x] | Active | 2026-04-06 | 2026-05-03 | P3-05 | disassemble output readable and consistent |
| P3-07 | Add disassembler validation tests | [x] | Active | 2026-05-02 | 2026-05-03 | P3-06 | 10+ tests including objdump comparison |
| P3-08 | Parser profiling baseline report | [x] | Active | 2026-05-04 | 2026-05-05 | P3-07 | hotspots identified on 10k+ line input |
| P3-09 | Symbol lookup optimization (hash-based) | [x] | Active | 2026-05-05 | 2026-05-08 | P3-08 | measured lookup speedup with no regressions |
| P3-10 | Token and line buffer optimization | [x] | Active | 2026-05-08 | 2026-05-10 | P3-09 | memory and throughput improved with tests green |
| P3-11 | Phase 3 quality gate | [x] | Active | 2026-05-10 | 2026-05-10 | P3-01..P3-10 | all Phase 3 tests pass plus perf report attached |

### Phase 3 Checklist

- [x] Listing feature shipped with tests and docs.
- [x] Disassembler baseline shipped with lookup-table linkage, integration fixtures, and reference comparison.
- [x] Parser optimization measured and validated.

## Phase 4: Production Hardening

| ID | Task | Status | Owner | Start | Due | Depends On | Done Criteria |
|---|---|---|---|---|---|---|---|
| P4-01 | Large-program stress suite fixtures | [x] | Active | 2026-05-11 | 2026-05-14 | P3-11 | 6 to 10 stress programs added |
| P4-02 | Edge-case test suite expansion | [~] | Active | 2026-05-12 | 2026-05-15 | P3-11 | 15+ edge-case scenarios validated |
| P4-03 | NASM comparison suite and byte diff report | [x] | Active | 2026-05-15 | 2026-05-18 | P4-01, P4-02 | 35+ comparison programs with discrepancy log |
| P4-04 | Debugger DWARF validation (gdb or lldb) | [ ] | Unassigned | 2026-05-17 | 2026-05-19 | P4-02 | breakpoints, stepping, symbols verified |
| P4-05 | Documentation completion pass | [ ] | Unassigned | 2026-05-19 | 2026-05-21 | P4-03, P4-04 | README/docs updated and no stale TODO claims |
| P4-06 | Example suite finalization | [ ] | Unassigned | 2026-05-19 | 2026-05-22 | P4-05 | 12+ examples complete and runnable |
| P4-07 | Coverage and test docs report | [ ] | Unassigned | 2026-05-21 | 2026-05-23 | P4-01, P4-02 | TESTING doc and coverage matrix published |
| P4-08 | Final release gate run | [ ] | Unassigned | 2026-05-23 | 2026-05-24 | P4-03..P4-07 | full test suite green, no critical issues |
| P4-09 | Release prep bundle | [ ] | Unassigned | 2026-05-24 | 2026-05-24 | P4-08 | changelog, tag plan, quick-start, final binary check |

### Phase 4 Checklist

- [ ] Stress and edge-case suites complete.
- [ ] Cross-reference validation and debugger validation complete.
- [ ] Release documentation and examples complete.
- [ ] Final gate passed.

## Weekly Standup Template

Use this section every week to keep momentum.

### Week of ____

- Completed:
  - 
- In progress:
  - 
- Blockers:
  - 
- Next:
  - 

### Week of 2026-04-06

- Completed:
  - Added standardized parser diagnostics for SSE operand validation paths.
  - Converted a legacy parser unexpected-token operand error to normalized diagnostic format.
  - Restored parser suite to green after message-fragment compatibility alignment.
  - Isolated encoder crash root cause with ASan (`stack-overflow` in encoder tests from oversized stack context).
  - Fixed encoder crash by reducing `assembler_context_t` stack footprint (dynamic listing buffer).
  - Revalidated suites: `make test-encoder` (137/137 pass), `make test-parser` (77/77 pass).
  - Normalized macro/preprocessor parser diagnostics for macro definition and expansion error paths to standardized category plus suggestion format.
  - Reduced remaining legacy parser `Error:` call sites from 44 to 35.
  - Normalized include and preprocessor conditional/error diagnostics clusters to standardized category plus suggestion format.
  - Reduced remaining legacy parser `Error:` call sites from 35 to 0.
  - Updated parser `%error` test assertion to the standardized diagnostic contract.
  - Revalidated suites: `make test-parser` (77/77 pass), `make test-encoder` (137/137 pass).
  - Normalized assembler/encoder diagnostics for `align`, `incbin`, `.comm/.lcomm`, unhandled-instruction fallback, and encode-failure wrappers to the standardized structure with suggestions.
  - Reduced remaining legacy assembler `Error:` call sites in `src/x86_64_asm.c` from 47 to 34.
  - Normalized remaining legacy diagnostics in symbol/fixup/output/CLI/core encoding paths across `src/x86_64_asm.c`, `src/asm.c`, and `src/x86_64_encoding_core.c`.
  - Reduced remaining legacy `fprintf(stderr, "Error: ...")` call sites in `src/**/*.c` from 34 to 0.
  - Revalidated suites after full encoder diagnostics normalization: `make test-encoder` (137/137 pass), `make test-parser` (77/77 pass).
  - Completed parser did-you-mean and label did-you-mean diagnostics with additional negative-path regression tests.
  - Completed high-8-bit plus REX conflict expansion for `xchg` and `test`, with matrix coverage in encoder tests.
  - Updated architecture-constraint documentation and validated full Phase 2 quality gate.
  - Revalidated suites after Phase 2 completion: `make test-parser` (80/80 pass), `make test-encoder` (138/138 pass), `make test-integration` (43/43 pass).
  - Added listing format design spec in `docs/LISTING_FORMAT.md`.
  - Implemented listing row capture hooks across instruction and data emission, including source line text capture.
  - Validated listing smoke output now includes line, address, machine bytes, and source text columns.
  - Added 5 integration tests validating listing sections, rows, address monotonicity, byte-level consistency, and symbol appendix entries.
  - Revalidated full suites after listing test additions: `make test-parser` (80/80 pass), `make test-encoder` (138/138 pass), `make test-integration` (48/48 pass).
  - Added disassembler design specification in `docs/DISASSEMBLER_DESIGN.md` with CLI contract, decode pipeline, output format, and phased opcode-family rollout.
  - Implemented baseline disassembler core in `src/x86_64_disassembler.c` with ELF `.text` extraction and first decode families.
  - Wired CLI disassembly mode in `src/asm.c` (`--disassemble`, `-D`) and validated command output on built binaries.
  - Added disassembler unit suite `tests/test_disassembler.c` and Make target `test-disassembler`.
  - Expanded disassembler decode coverage for stack and short control-flow opcodes (`50-5F`, `68`, `6A`, `70-7F`, `EB`, `90`, `C9`, `FF /6`, `8F /0`).
  - Added disassembler unit coverage for push/pop, short jumps/jcc, and `nop`/`leave` output.
  - Expanded disassembler decode coverage for `lea` (`8D /r`) and `movsxd` (`63 /r`) compiler-emitted forms.
  - Added disassembler unit coverage for `lea`/`movsxd` and integration semantic comparison against `objdump -d -M intel`.
  - Revalidated suites after disassembler coverage expansion: `make test-parser` (80/80 pass), `make test-encoder` (138/138 pass), `make test-disassembler` (7/7 pass), `make test-integration` (49/49 pass).
  - Expanded two-byte decode families with `movzx` (`0F B6/B7`), `setcc` (`0F 90-9F`), and selected `cmovcc` (`0F 40-4F`) in `src/x86_64_disassembler.c`.
  - Added focused disassembler unit coverage for new families in `tests/test_disassembler.c`; `make test-disassembler` now passes 8/8.
  - Revalidated integration after decode expansion: `make test-integration` (49/49 pass).
  - Expanded two-byte decode families with selected `movsx` (`0F BE/BF`), `imul` (`0F AF`), and bit-test register forms (`0F A3/AB/B3/BB`).
  - Added focused disassembler unit coverage for `movsx`/`imul`/bit-test families in `tests/test_disassembler.c`; `make test-disassembler` now passes 9/9.
  - Revalidated integration after second decode expansion: `make test-integration` (49/49 pass).
  - Added operand-size override (`0x66`) handling for selected decode paths, including two-byte families and immediate-width-sensitive forms (`B8+rd`, `C7 /0`, `81 /digit`).
  - Added focused disassembler unit coverage for 16-bit override forms in `tests/test_disassembler.c`; `make test-disassembler` now passes 10/10.
  - Added integration semantic cross-check fixture using raw bytes and `objdump -D -b binary -m i386:x86-64 -M intel` to validate 0x66 family mnemonics.
  - Revalidated integration after operand-override expansion: `make test-integration` (50/50 pass).
  - Expanded two-byte decode coverage for immediate bit-test forms (`0F BA /4-/7`) covering `bt`/`bts`/`btr`/`btc` with immediate bit index.
  - Added focused unit coverage for register, memory, and operand-override immediate bit-test forms in `tests/test_disassembler.c`; `make test-disassembler` now passes 11/11.
  - Added integration semantic cross-check fixture for `0F BA` forms (including SIB addressing) against `objdump`.
  - Revalidated integration after `0F BA` expansion: `make test-integration` (51/51 pass).
  - Expanded decode coverage for selected bit-scan and test forms: `bsf` (`0F BC`), `bsr` (`0F BD`), and `test r/m,reg` (`85 /r`) with operand-size override handling.
  - Added focused unit coverage for `bsf`/`bsr`/`test` register, memory, and 16-bit override forms in `tests/test_disassembler.c`; `make test-disassembler` now passes 12/12.
  - Added integration semantic cross-check fixture for `bsf`/`bsr`/`test` forms (including SIB memory addressing) against `objdump`.
  - Revalidated integration after bit-scan/test expansion: `make test-integration` (52/52 pass).
  - Expanded decode coverage for selected atomic/exchange families: `xadd` (`0F C0/C1`) and `cmpxchg` (`0F B0/B1`) with 8/16/32/64 handling.
  - Added focused unit coverage for `xadd`/`cmpxchg` register, memory, and operand-override forms in `tests/test_disassembler.c`; `make test-disassembler` now passes 13/13.
  - Added integration semantic cross-check fixture for `xadd`/`cmpxchg` forms including RIP-relative and SIB/displacement-heavy addressing against `objdump`.
  - Revalidated integration after atomic/exchange expansion: `make test-integration` (53/53 pass).
  - Added edge-case coverage for `setcc` and `movzx`/`movsx` encodings where REX changes 8-bit register naming (e.g., `ah` vs `spl`) and RIP-relative memory forms.
  - Added focused disassembler unit coverage for these edge encodings in `tests/test_disassembler.c`; `make test-disassembler` now passes 14/14.
  - Added integration semantic cross-check fixture for `setcc`/`movzx`/`movsx` edge forms against `objdump`, including RIP-relative and REX-dependent `spl` forms.
  - Revalidated integration after setcc/movx edge expansion: `make test-integration` (54/54 pass).
  - Quantified remaining fallback density on representative project binaries via `./bin/x86_64-asm --disassemble` scan:
    - `bin/x86_64-asm`: 12608 / 32933 rows (`38.28%`) fallback
    - `bin/test_encoder`: 22358 / 59810 rows (`37.38%`) fallback
    - `bin/test_parser`: 15392 / 42216 rows (`36.46%`) fallback
    - `bin/test_integration`: 15178 / 42368 rows (`35.82%`) fallback
    - `bin/test_disassembler`: 12913 / 34302 rows (`37.65%`) fallback
  - Aggregated top unresolved bytes across sampled binaries: `0x0f`, `0x1f`, `0x66`, `0x80`, `0xf3`, `0x2e`, `0x64`, `0x67` (with many trailing `0x00` rows caused by one-byte fallback progression).
  - Pattern sampling indicates highest-impact missing families are multi-byte NOP/prefix-heavy forms (e.g., `0F 1F /0` with legacy prefixes) and additional Group-1 immediate variants around `0x80`.
  - Implemented multi-byte NOP decode for `0F 1F /0` (including legacy-prefix-heavy forms) and broader legacy prefix consumption in `src/x86_64_disassembler.c`.
  - Implemented Group-1 immediate decode for opcode `0x80` (covered ext ops in current group mnemonic table) in `src/x86_64_disassembler.c`.
  - Added focused unit coverage for multi-byte NOP and `0x80` forms in `tests/test_disassembler.c`; `make test-disassembler` now passes 15/15.
  - Added integration semantic cross-check fixture for multi-byte NOP + `0x80` families against `objdump`; `make test-integration` now passes 55/55.
  - Re-ran fallback quantification after this slice:
    - `bin/x86_64-asm`: 5392 / 26791 rows (`20.13%`) fallback
    - `bin/test_encoder`: 7710 / 47393 rows (`16.27%`) fallback
    - `bin/test_parser`: 5406 / 33613 rows (`16.08%`) fallback
    - `bin/test_integration`: 6361 / 35075 rows (`18.14%`) fallback
    - `bin/test_disassembler`: 5488 / 28098 rows (`19.53%`) fallback
  - Fallback density dropped materially from ~36%-38% to ~16%-20% across sampled binaries.
- In progress:
  - Disassembler implementation baseline (P3-05): expanding opcode-family coverage and reducing fallback `db` rows.
  - Disassemble CLI mode hardening (P3-06): output consistency against reference tools.
- Blockers:
  - None currently.
- Next:
  - Expand remaining Group-1 immediate and unary families around high-frequency unresolved bytes (`0xC0`/`0xC1`, `0xF6`, selected `0x88` forms where appropriate) to further reduce fallback rows.
  - Add a consolidated semantic fixture blending these unresolved families and re-run fallback quantification to track next reduction.

### Week of 2026-04-07

- Completed:
  - Added `integration_disassembler_source_roundtrip_semantic` to validate source-to-disassembly semantic roundtrip behavior.
  - Implemented robust roundtrip comparison flow by extracting executable bytes from emitted ELF program headers and comparing mnemonic presence against `objdump` raw-binary disassembly.
  - Linked encoder/disassembler group opcode mappings through shared lookup metadata in `include/x86_64_asm/opcode_lookup.h` to close the remaining lookup-table generation gap.
  - Added parser profiling instrumentation and benchmark target (`make profile-parser`) with results documented in `docs/PARSER_PROFILING.md`.
  - Measured parser performance on a ~15k-line synthetic program (5 runs): average parse time ~158.540 ms; instruction buffer realloc events: 0.
  - Measured symbol lookup performance at 3500 symbols/400000 lookups: 63.71x speedup for hash lookup vs linear scan.
  - Revalidated full Phase 3 gate: `make test-parser` (80/80 pass), `make test-encoder` (138/138 pass), `make test-disassembler` (16/16 pass), `make test-integration` (57/57 pass).
  - Final handoff rerun (2026-04-07): `make test-parser` (80/80 pass), `make test-encoder` (138/138 pass), `make test-disassembler` (16/16 pass), `make test-integration` (57/57 pass).
  - Expanded Phase 4 stress corpus to 6 fixtures (`stress_100_functions`, `stress_1000_labels`, `stress_50kb_mixed`, `stress_macro_nested`, `stress_sse_broad`, `stress_addressing_matrix`) and re-generated `docs/STRESS_METRICS.md`.
  - Added NASM comparison harness target (`make compare-nasm`) with categorized mismatch reporting and first-difference metadata in `docs/NASM_COMPARISON_REPORT.md`.
  - Expanded NASM reference fixture set from 5 to 10 programs across arithmetic, control-flow, memory, SSE, logic, shifts, bit-ops, stack, and data categories.
  - Implemented encoder canonicalization for `mov r64, imm` with non-negative 32-bit immediates to prefer `B8+rd imm32` (including `REX.B` for `r8-r15`) and added unit coverage for `mov rax, imm32` / `mov r8, imm32` encodings.
  - Implemented safe short-branch canonicalization for backward label targets (`jcc`/`jmp`) while preserving near+fixup fallback for unresolved/forward labels to avoid control-flow sizing regressions.
  - Added encoder unit coverage for short backward `jnz` and `jmp` label forms.
  - Revalidated encoder suite after canonicalization: `make test-encoder` (144/144 pass).
  - Re-ran NASM comparison after canonicalization: `docs/NASM_COMPARISON_REPORT.md` improved from 0/10 to 4/10 byte-identical fixtures.
  - Attempted forward short-branch selection for resolved labels and reverted it after integration regressions to preserve runtime correctness (kept backward-only short-branch policy).
  - Corrected NASM comparison extraction in `tools/compare_with_nasm.sh` to compare executable bytes from ELF entrypoint onward (code-to-code), removing leading data contamination from mixed executable segments.
  - Revalidated after harness correction: `make compare-nasm` remains 4/10 matches with updated first-diff signatures; `make test-integration` remains green (61/61 pass).
  - Updated label-memory encoding to NASM-style absolute disp32 for bare `[label]` forms (explicit `[rip+...]` unchanged), including displacement-addend-preserving fixup resolution.
  - Added encoder unit coverage for absolute label memory forms and fixup addends (`mov_r64_mem_label_absolute`, `mov_r64_mem_label_disp_addend`).
  - Re-enabled forward short-branch selection with iterative text-layout stabilization and conservative fallback when convergence is not reached.
  - Revalidated full gates after branch/layout updates: `make test-encoder` (146/146 pass), `make test-integration` (61/61 pass).
  - NASM harness now links reference NASM objects using a temporary data-first linker script before extracting `.text`, ensuring resolved code-to-code comparison under equivalent layout assumptions.
  - Revalidated after harness alignment: `make compare-nasm` now reports 10/10 byte-identical fixtures with zero tool errors.
  - Expanded NASM fixture corpus from 10 to 35 fixtures (`reference_nasm_*`) across arithmetic, control-flow, data, memory, logic, shifts, bit-ops, stack, and SSE categories.
  - Revalidated parity gate after fixture expansion: `make compare-nasm-gate` now reports 35/35 byte-identical fixtures with 0 mismatches and 0 tool errors.
  - Integration stability preserved after parity completion: `make test-integration` remains 61/61 pass.
  - Revalidated integration after Phase 4 updates: `make test-integration` (61/61 pass).
- In progress:
  - None for Phase 3.
- Blockers:
  - None currently.
- Next:
  - Expand NASM comparison matrix from 10 toward 35+ fixtures and start discrepancy bucketing by instruction family.
  - Continue edge-case suite expansion toward P4-02 done criteria (15+ scenarios) and re-run parser/encoder/integration gates.

## Blocker Log

| Date | Task ID | Blocker | Owner | Mitigation | Status |
|---|---|---|---|---|---|
| 2026-04-06 | P2-02 | `make test-encoder` segmentation fault before first test output | Active | Re-run after clean, use sanitizer run, narrow to first crashing test case | Resolved |
|  |  |  |  |  |  |

## Definition of Done (Project)

- 300 plus tests passing.
- Line coverage above 90 percent and branch coverage above 85 percent in critical paths.
- Feature docs match implementation with no stale claims.
- Stress, reference, and debugger validations complete.
- Release artifacts prepared and verified.
