# SSE TODO (Remaining Work Only)

## Baseline Status

Core SSE support is implemented and test-backed. The checklist below focuses on completeness.

## Milestone 1: Define Coverage Envelope

- [ ] Publish a supported-SSE matrix in docs (mnemonic + legal operand forms)
- [ ] Mark unsupported forms explicitly with expected diagnostics

## Milestone 2: Unit Coverage Expansion

- [ ] Add missing positive-path tests for currently supported SSE families
- [ ] Add size-mismatch/illegal-combination negative tests
- [ ] Add tests for addressing edge cases (displacement, SIB, RIP-relative where supported)
- [ ] Add tests that validate diagnostics for unsupported forms

## Milestone 3: Integration Hardening

- [ ] Add at least one integration scenario per major supported SSE family
- [ ] Add one mixed-instruction integration scenario (SSE + scalar/control-flow)
- [ ] Ensure all new integration scenarios assert runtime behavior, not just assembly success

## Milestone 4: Verification and Docs

- [ ] Cross-check representative encodings against a reference assembler
- [ ] Update `docs/ASSEMBLY_REFERENCE.md` to reflect actual supported SSE subset
- [ ] Confirm README and troubleshooting wording matches matrix coverage

## Exit Criteria

- [ ] SSE support claims are test-backed
- [ ] Negative-path diagnostics are predictable and documented
- [ ] No stale TODO items describing already-completed baseline SSE work
