# Phase 3 Status: Advanced Features and Testing

## Snapshot (March 2026)

Phase 3 is largely complete. This document now tracks residual work only.

## 1) Extended Instruction Support

### Done
- String instructions implemented: `movsb`, `movsw`, `movsd`, `movsq`
- SSE/XMM baseline implemented and tested (selected move and arithmetic paths)
- `setcc` family implemented
- Bit manipulation instructions implemented (`bt`, `bts`, `btr`, `btc`)
- `jnz` synonym support added

### Partially Complete
- SSE support is functional, but not all instruction/operand edge cases are covered by tests

### Remaining
- SIB scale-factor coverage is not yet complete in plan terms (`[base + index*scale]` matrix coverage and validation)

## 2) Testing Infrastructure

### Done
- Parser unit tests
- Encoder unit tests
- Integration tests with assertions
- CI workflow in `.github/workflows/ci.yml`

### Partially Complete
- Example execution in CI is currently `continue-on-error: true` and should become strict if we want hard gating

### Remaining
- Optional: add broader failure-mode integration tests for large source sets and stress cases

## 3) Developer Experience

### Done
- Comprehensive `README.md`
- Assembly language reference guide
- Troubleshooting guide
- Example walkthroughs

### Remaining
- Keep docs synchronized as features deepen (especially SSE and DWARF coverage notes)

## 4) Advanced Features

### Done
- Macro support (`.macro`/`.endm`)
- Include support (`.include`) with nested include handling
- Debug generation baseline via `-g` with DWARF sections and line mapping

### Partially Complete
- DWARF is useful baseline output, not full production-grade completeness

### Remaining
- Listing file generation
- Disassembler mode

## Recommended Next Slice

1. Complete SSE edge-case test matrix (highest confidence gain for least implementation risk)
2. Expand DWARF completeness incrementally (types, richer DIE relationships, broader debug semantics)
3. Decide whether listing/disassembler are in-scope for current release target
