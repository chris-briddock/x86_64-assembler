# Phase 4 Action Plan (Execution-Ready)

Date: 2026-04-07
Scope: Move from Phase 3 completion to production hardening and release readiness.

## Planning Principles

1. Keep one critical-path implementation task active at a time.
2. For each slice: implement first, then unit/integration tests, then examples/docs.
3. Do not move to release tasks until stress, edge, and cross-reference gates are green.

## Workstreams

## WS1: Stress and Scalability (Critical Path)

Objective: Prove assembler stability and acceptable performance on large inputs.

Task WS1-1: Stress fixture suite creation
- Build 6 to 10 stress programs under examples/ or tests/fixtures/ covering:
  - 100+ functions
  - 1000+ labels
  - 50KB+ source
  - mixed instruction families
  - deep macro nesting (10 levels)
  - broad SSE family usage
- Deliverable: reproducible stress corpus and runner target.
- Exit criteria:
  - no crashes/hangs
  - successful assembly across suite

Task WS1-2: Stress metrics collection
- Capture assembly duration, output size, and process memory proxy data per fixture.
- Establish baseline and trend table.
- Deliverable: stress metrics report in docs/.
- Exit criteria:
  - 50KB program assembles under 10 seconds on this environment

## WS2: Edge-Case Hardening

Objective: Ensure boundary conditions fail safely or succeed predictably.

Task WS2-1: Edge-case test matrix
- Add at least 15 cases in parser/encoder/integration layers for:
  - empty sections
  - very long labels
  - deep memory expressions
  - max/min displacements
  - mixed operand sizes
  - circular macro expansion
  - duplicate includes
  - large immediates
- Deliverable: expanded tests and expected diagnostics.
- Exit criteria:
  - deterministic outcomes
  - standardized actionable diagnostics

## WS3: Reference Validation Against NASM

Objective: Verify encoding equivalence and explain differences.

Task WS3-1: Comparison harness
- Add a harness to assemble equivalent fixtures with both tools and byte-compare results.
- Initial matrix target: 35+ programs across general, memory, SSE, high-register, and branch families.
- Deliverable: discrepancy report with categorized diffs.
- Exit criteria:
  - 95%+ byte-identical outcomes
  - all non-identical cases documented and intentional

## WS4: DWARF Debugger Validation

Objective: Confirm debug usability in real debugger workflows.

Task WS4-1: GDB validation pack
- Validate breakpointing, stepping, and symbol visibility on representative binaries.
- Capture session logs and expected command/output snippets.
- Deliverable: debugger validation notes in docs/.
- Exit criteria:
  - successful baseline debugging workflow in at least one debugger

## WS5: Documentation and Example Finalization

Objective: Ensure user-facing material matches implementation status.

Task WS5-1: Docs completion pass
- Update README and core docs for final constraints/capabilities.
- Remove stale TODO/future wording from user-facing docs.
- Add completion checklist and release notes draft.

Task WS5-2: Example suite completion
- Ensure 12+ examples, each runnable and purpose-specific.
- Add missing examples for function calls, advanced macros, conditional logic, addressing modes, string ops, packed SSE, and embedded data.

Task WS5-3: Testing docs and coverage matrix
- Add docs/TESTING.md and test matrix by feature.
- Add coverage generation command and baseline numbers.

## WS6: Final Release Gate

Objective: Confirm production readiness and package release artifacts.

Task WS6-1: Full gate run
- Run:
  - make test
  - make test-parser
  - make test-encoder
  - make test-disassembler
  - make test-integration
  - stress suite
  - reference comparison suite
  - debugger validation suite
- Exit criteria: zero unexplained failures.

Task WS6-2: Release bundle
- Prepare CHANGELOG, CONTRIBUTING, quick-start, version tag plan, and final binary checks.

## Suggested Execution Order

1. WS1-1 stress fixtures
2. WS2-1 edge-case matrix
3. WS1-2 stress metrics
4. WS3-1 NASM comparison harness
5. WS4-1 debugger validation
6. WS5-1/WS5-2/WS5-3 docs + examples + testing docs
7. WS6 full release gate and bundle

## Sprint Breakdown (Practical)

Sprint 1 (next):
- Complete WS1-1 and WS2-1
- Start WS1-2 metrics capture

Sprint 2:
- Complete WS1-2
- Complete WS3-1

Sprint 3:
- Complete WS4-1
- Complete WS5-1/WS5-2/WS5-3

Sprint 4:
- Execute WS6-1 and WS6-2

## Immediate Next Slice (Start Here)

Slice A (first implementation slice): Stress fixture suite foundation
- Build first 3 stress fixtures:
  1. 100+ functions fixture
  2. 1000+ labels fixture
  3. 50KB mixed-instruction fixture
- Add runner target in Makefile.
- Add integration assertions for successful assembly and non-empty output.
- Then update docs with fixture usage.
