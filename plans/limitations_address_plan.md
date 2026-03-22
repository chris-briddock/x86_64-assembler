# Limitations Status and Remaining Roadmap

## Snapshot (March 2026)

This document was originally a full implementation plan. It is now a status ledger of what was addressed and what remains.

## Priority 1 Status

### Addressed
- `jnz` synonym support
- String instruction support (`movsb`, `movsw`, `movsd`, `movsq`)
- Symbol lookup architecture improved beyond prior linear-only bottlenecks

### Constraint (Not Removable)
- High 8-bit register with REX conflict is architectural x86_64 behavior; assembler diagnostics and tests cover this path

### Remaining
- SIB scale-factor feature depth and full coverage matrix closure

## Priority 2 Status

### Addressed
- SSE baseline support (XMM registers, selected move/arithmetic instructions)
- Bit manipulation instructions (`bt`, `bts`, `btr`, `btc`)
- `setcc` coverage implemented

### Remaining
- SSE edge-case completeness (instruction families, operand forms, negative cases)

## Priority 3 Status

### Addressed
- Macro support
- Include support
- DWARF baseline debug output with line mapping

### Remaining
- Listing file generation
- Disassembler mode
- DWARF expansion to broader production-grade completeness

## Priority 4 Status

### Addressed
- Parser diagnostics significantly improved from original baseline

### Remaining
- Add source-line + caret marker diagnostics everywhere
- Local label strategy (`.L*`) and richer constant-expression capabilities
- Cross-platform output formats are still out of scope (Linux ELF64 remains target)

## Next Execution Order

1. SSE edge-case matrix and strict tests
2. DWARF completeness increments with integration assertions
3. SIB scale-factor coverage completion
4. Listing file/disassembler scoping decision
