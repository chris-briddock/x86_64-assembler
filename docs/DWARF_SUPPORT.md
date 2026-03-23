# DWARF Support

This document defines the DWARF subset emitted by the assembler when -g is enabled.
It is intended to set clear expectations for debugger tooling and contributor work.

## Scope

- DWARF version: 4
- Unit format: 32-bit DWARF unit length fields
- Address size: 8 bytes (x86_64)
- Endianness: Little-endian (ELF64 Linux output)

## Emitted Sections

When debug output is enabled, the assembler emits:

- .debug_info
- .debug_abbrev
- .debug_line
- .debug_str

It also emits a companion text debug map file: <output>.dbg.

## Abbreviation Codes

The current .debug_abbrev table contains the following entries.

### Abbrev 1: DW_TAG_compile_unit (has children)

- DW_AT_producer: DW_FORM_strp
- DW_AT_language: DW_FORM_data2 (DW_LANG_Asm)
- DW_AT_name: DW_FORM_strp
- DW_AT_comp_dir: DW_FORM_strp
- DW_AT_stmt_list: DW_FORM_sec_offset
- DW_AT_low_pc: DW_FORM_addr
- DW_AT_high_pc: DW_FORM_data8 (size/span)

### Abbrev 2: DW_TAG_base_type

- DW_AT_name: DW_FORM_strp
- DW_AT_encoding: DW_FORM_data1
- DW_AT_byte_size: DW_FORM_data1

### Abbrev 3: DW_TAG_pointer_type

- DW_AT_name: DW_FORM_strp
- DW_AT_byte_size: DW_FORM_data1
- DW_AT_type: DW_FORM_ref4

### Abbrev 4: DW_TAG_subprogram (has children)

- DW_AT_name: DW_FORM_strp
- DW_AT_decl_file: DW_FORM_data1
- DW_AT_decl_line: DW_FORM_udata
- DW_AT_decl_column: DW_FORM_udata
- DW_AT_external: DW_FORM_flag
- DW_AT_low_pc: DW_FORM_addr
- DW_AT_high_pc: DW_FORM_data8 (size/span)
- DW_AT_type: DW_FORM_ref4

### Abbrev 5: DW_TAG_lexical_block

- DW_AT_low_pc: DW_FORM_addr
- DW_AT_high_pc: DW_FORM_data8 (size/span)

### Abbrev 6: DW_TAG_label

- DW_AT_name: DW_FORM_strp
- DW_AT_decl_file: DW_FORM_data1
- DW_AT_decl_line: DW_FORM_udata
- DW_AT_decl_column: DW_FORM_udata
- DW_AT_low_pc: DW_FORM_addr

## Line Table Behavior

The .debug_line program uses a compact standard-opcode flow:

- DW_LNE_set_address to initialize PC to the text base address
- DW_LNS_advance_pc and DW_LNS_advance_line based on line-map deltas
- DW_LNS_copy to commit rows
- DW_LNE_end_sequence terminator at the end of the sequence

Header constants currently emitted:

- minimum_instruction_length = 1
- maximum_operations_per_instruction = 1
- default_is_stmt = 1
- line_base = -5
- line_range = 14
- opcode_base = 13

## Validation Commands

Recommended validation commands for generated binaries:

- readelf --debug-dump=info <binary>
- readelf --debug-dump=line <binary>
- dwarfdump <binary>
- llvm-dwarfdump <binary>

Integration tests validate both structural parsing and external tool compatibility.
When dwarfdump is unavailable in an environment, the dwarfdump integration test is skipped.

## Current Limitations

This is a practical baseline subset, not full production-grade DWARF coverage.

- Single compile unit model per output binary
- No .debug_frame or .eh_frame call-frame information
- No location lists, range lists, or macro information
- No variable/parameter DIE emission yet
- No inlined-function DIE modeling
- Minimal type graph (baseline unsigned 64-bit + pointer type)
- Limited line-table expressiveness compared to full compiler-grade emitters

## Debugger Integration Guidance

For debugger/tool consumers:

- Expect reliable function and label names in .debug_info/.debug_str
- Use low_pc + high_pc as address range start + span
- Use .debug_line for source line correlation in simple assembly programs
- Treat unsupported DWARF features as absent by design, not malformed output

## Evolution Plan

Future DWARF work should preserve backward compatibility for existing tags/forms where possible,
and expand by adding new abbreviation entries rather than mutating semantics of current ones.
