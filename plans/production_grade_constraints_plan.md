# Production-Grade Closure Plan for Remaining Constraints/Gaps

Date: 2026-03-22

## Objective

Move the current "Known Constraints and Remaining Gaps" from baseline-safe to production-grade readiness through measurable, test-backed milestones.

## Track 1: High 8-bit Register + REX Constraint (Architectural)

This is an x86_64 ISA constraint and cannot be removed.
Production-grade target here is predictable behavior and complete diagnostics.

### Milestones

1.Expand negative tests across all affected instruction families

- Add high-8-bit + REX conflict tests beyond `mov` (e.g., `add/sub/xchg` paths where applicable)
- Assert exact diagnostics (including suggestion text)

2.Add parser-level guidance consistency

- Ensure parser/encoder errors both present clear remediation suggestions
- Standardize message wording for AH/BH/CH/DH conflict cases

3.Add integration matrix sample

- Add one integration test with a small file that intentionally triggers multiple conflict shapes

### Exit Criteria

- No code path emits ambiguous high-8-bit/REX errors
- Negative-path tests cover all instruction encoders that can accept 8-bit regs
- Docs clearly classify this as permanent architectural behavior (not a temporary implementation gap)

## Track 2: SSE Completeness

### Current State

- Core scalar SSE support implemented and covered
- Packed integer/compare/logical support implemented
- New regression coverage added for `pcmpeqq` and `pcmpgtq` (`0F 38` map)

#### Milestones

1. Coverage matrix completion

- Publish instruction-family x operand-form x addressing-mode matrix
- Mark unsupported forms and expected diagnostics

2. Unit expansion

- Add positive tests for missing families/forms (`psub*`, `pandn`, `pxor`, store-form cases)
- Add negative tests for size/type mismatches per family
- Add REX + complex addressing cases for each major family

3. Integration hardening
- Add one mixed scenario each for scalar SSE and packed SSE in realistic control flow
- Assert runtime behavior in addition to assembly success

4. Cross-checking
- Verify representative byte sequences against a reference assembler
- Document any intentionally unsupported forms

### Exit Criteria

- SSE claims are fully mapped and test-backed
- Diagnostics are deterministic for unsupported/invalid forms
- No known opcode-map blind spots in tested instructions

## Track 3: DWARF Completeness

### Current State

- Emits `.debug_info`, `.debug_abbrev`, `.debug_line`, `.debug_str`
- Includes compile-unit and per-function metadata
- Buffer overflow protection is present and tested

### Milestones

1. Metadata enrichment
- Expand DIE attribute coverage where practical
- Improve source correlation in line program for edge layouts

2. Validation depth
- Add integration assertions for more DWARF structural details
- Validate generated sections with external tooling checks

3. Robustness
- Add tests for larger symbol/function counts and section size pressure
- Confirm graceful failure remains deterministic for overflows

4. Documentation
- Define supported DWARF subset explicitly
- Separate "implemented baseline" vs "future enhancements"

### Exit Criteria

- Structural DWARF tests validate more than section presence
- Output is stable across representative programs
- Documentation accurately scopes baseline vs full-feature expectations

## Proposed Execution Order

1. SSE matrix and missing family tests
2. High-8-bit/REX diagnostic matrix expansion
3. DWARF metadata and structure assertions
4. Final doc sync and removal/refinement of "remaining gaps" wording

## Immediate Next Slice

- Add SSE packed-family tests for currently untested but implemented instructions (`psub*`, `pandn`, `pxor`)
- Add at least one high-8-bit/REX conflict test outside `mov`
- Add one deeper DWARF structure assertion in integration tests
