# Parser Fix Status and Follow-Up Plan

## Snapshot (March 2026)

The original parser desynchronization issue and related regressions have been fixed. This document now tracks parser-quality follow-up only.

## Completed

- Multi-line parsing token-flow issue resolved
- Unknown instruction diagnostics improved (no misleading fallback errors)
- Parser regression tests expanded
- Label declaration column tracking added and covered by tests
- Macro/include parsing paths integrated and validated

## Remaining Parser Improvements

### 1) Error Reporting Quality
- Add source-line echo and caret-position markers in parser diagnostics
- Normalize diagnostic wording across parser/encoder/fixup stages

### 2) Syntax Surface (Optional)
- Local labels (`.L1`, `.L2`) semantics
- Extended constant definitions and stricter expression handling

### 3) Parser Performance (Optional)
- Token and line-buffer optimization for large sources
- Parser hot-path profiling and targeted micro-optimizations

## Recommended Next Parser Slice

1. Improve diagnostics to include source line + caret column
2. Add tests that assert diagnostic text/position (not only error return code)
3. Re-run full parser + integration suite as acceptance gate
