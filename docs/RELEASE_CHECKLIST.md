# Release Checklist

Use this checklist before tagging a production release.

## Quality Gates

- [ ] `make compare-nasm-gate` passes (0 mismatches, 0 tool errors)
- [ ] `make ci` passes (parity + unit + disassembler + integration + stress)
- [ ] `make coverage` completes and artifacts are reviewed
- [ ] GitHub Actions CI workflow passes on the release commit

## Compatibility and Scope

- [ ] NASM comparison matrix reflects current release scope
- [ ] Any intentionally unsupported forms are documented in docs
- [ ] New instruction/addressing behavior has unit and integration coverage

## Runtime and Diagnostics

- [ ] Example programs run with expected exit behavior
- [ ] Error diagnostics remain actionable for parser and encoder failures
- [ ] No regressions in listing/disassembler semantic checks

## Documentation and Versioning

- [ ] README test and release instructions are current
- [ ] docs/NASM_COMPARISON_REPORT.md is regenerated and committed
- [ ] CHANGELOG.md has an entry for this release
- [ ] Version/tag name chosen and recorded in release notes

## Release Artifacts

- [ ] Source tree is clean after running release gates
- [ ] Binary builds reproducibly from a clean checkout
- [ ] Release notes summarize gates, compatibility, and known limitations
