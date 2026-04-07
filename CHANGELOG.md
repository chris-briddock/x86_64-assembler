# Changelog

All notable changes to this project are documented in this file.

## [Unreleased]

## [0.1.0] - 2026-04-07

### Added
- NASM byte-parity comparison harness and strict parity gate target.
- CI aggregate production gate target (`make ci`).
- Stress fixture suite and stress assembly validation target.
- Disassembler test suite and semantic integration checks.
- Listing output support with integration coverage.
- Parser profiling and stress metrics tooling.

### Changed
- NASM comparison flow now links NASM fixtures under a controlled layout before byte comparison.
- Default `make test` now runs parity gate before legacy example execution.
- Documentation updated for maintenance-phase parity handling and gate usage.

### Quality Snapshot
- NASM parity: 10/10 byte-identical fixtures.
- Unit tests: 229/229 pass.
- Disassembler tests: 16/16 pass.
- Integration tests: 61/61 pass.
- Stress fixture assembly checks: pass.
