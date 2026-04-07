# Listing File Format

Status: Draft (Phase 3.1)
Owner: Active
Last updated: 2026-04-06

## Goal

Define a stable, human-readable listing output for the assembler `-l` option.

The listing file is intended to help users:
- Inspect emitted bytes per source line.
- Correlate source text with assembled addresses.
- Review final symbol addresses.

## CLI Contract

- Enable listing output with `-l`.
- Listing output path is `<output>.lst`.
- If writing the listing fails, assembly exits with an I/O diagnostic.

Example:

```bash
./bin/x86_64-asm examples/hello.asm -o hello.out -l
```

Produces:
- `hello.out`
- `hello.out.lst`

## File Structure

A listing file has three sections:

1. Preamble
2. Instruction listing table
3. Symbol table appendix

### 1) Preamble

Current preamble lines:

```text
; x86_64-asm listing file
; Source: <source-path>
; base_address=0x<hex>
```

Notes:
- Preamble lines begin with `;`.
- `Source` uses the assembler context file name.
- `base_address` is printed in lowercase hex without zero-padding.

### 2) Instruction Listing Table

Header row:

```text
Line   Address  Machine Code                     Source
----   -------  ------------                     ------
```

Column definitions:
- `Line`: source line number (decimal).
- `Address`: instruction address (lowercase hex, no `0x`, no fixed width).
- `Machine Code`: emitted bytes in lowercase hex, space-separated, up to 16 bytes shown.
- `Source`: original source line, trailing newline removed.

Formatting details:
- Table uses left-aligned fixed-width fields.
- Machine bytes are truncated to at most 16 bytes in a row.
- Source text is copied from stored line text and newline-trimmed.

### 3) Symbol Table Appendix

Rendered after a blank line:

```text
; Symbol Table
; Name                           Address          Section
; ----                           -------          -------
; <name>                         0x<16-hex>       text|data
```

Symbol table rules:
- Every symbol currently in the assembler symbol table is emitted.
- `Address` is zero-padded to 16 hex digits with `0x` prefix.
- `Section` maps `0` to `text`, all other values to `data`.

## Current Constraints

- Listing entry byte display is capped at 16 bytes per row.
- Listing rows are populated from encoder/data emission tracking hooks.
- No separate relocation/fixup annotation is emitted yet.

## Acceptance Criteria (Phase 3.1)

- A user can run with `-l` and always obtain `<output>.lst` on success.
- Listing file contains the three sections above with correct formatting.
- Listing content aligns with emitted machine code and symbol addresses.
- Documentation and implementation stay synchronized.

## Validation Plan

1. Golden-file check on representative examples:
   - `examples/hello.asm`
   - `examples/loop.asm`
   - `examples/sse_test.asm`
2. Spot-check byte rows against encoder output bytes.
3. Spot-check symbol addresses against debug dump output.
4. Integration tests for listing generation and formatting expectations are implemented in `tests/test_integration.c`.

Implemented listing integration tests:
- `integration_listing_file_sections`
- `integration_listing_rows_present`
- `integration_listing_address_monotonic`
- `integration_listing_bytes_match_buffers`
- `integration_listing_symbol_table_entries`

## Follow-up Tasks

- Expand coverage with stronger source-column normalization checks for long lines/comments.
- Decide whether to include section names per row (`text`/`data`).
- Decide whether to include relocation/fixup markers in listing rows.
