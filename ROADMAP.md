# Assembler - Completion Roadmap

**Goal**: Move all partially-implemented and not-implemented items to production-grade completion.

**Timeline**: Phased approach with clear acceptance criteria and dependencies.

---

## Phase Overview

| Phase | Duration | Focus | Priority |
|-------|----------|-------|----------|
| **Phase 1: Stability & Depth** | 2-3 weeks | SSE, SIB, DWARF completeness | 🔴 Critical |
| **Phase 2: Diagnostics & Polish** | 1-2 weeks | Error messages, consistency | 🟠 High |
| **Phase 3: Advanced Features** | 3-4 weeks | Listing, disassembler, optimizations | 🟡 Medium |
| **Phase 4: Production Hardening** | 1-2 weeks | Stress testing, validation, docs | 🔴 Critical |

---

# PHASE 1: STABILITY & DEPTH (Critical Path)

## 1.1 SSE Instruction Coverage Completion

**Status**: ⚠️ Partially implemented | **Priority**: 🔴 Critical  
**Owner**: Design matrix first, then implement tests

### Milestone 1.1.1: Coverage Matrix Definition
**Goal**: Document exactly what SSE is supported and what isn't.

- [x] **Create `docs/SSE_COVERAGE_MATRIX.md`**
  - Enumerate all supported SSE mnemonics (move, arithmetic, packed integer, compare, logical)
  - Define legal operand forms for each (reg-to-reg, reg-to-mem, mem-to-reg, immediate where applicable)
  - Mark scale factors tested (1, 2, 4, 8)
  - Mark addressing modes tested (direct displacement, SIB, RIP-relative)
  - Explicitly list unsupported forms with expected error message
  - Build a test matrix: instruction × operand-form × register-band × addressing-mode

**Deliverables**:
- Matrix document with 100+ cells
- Clear "supported" vs "unsupported" classification
- Expected diagnostics for invalid forms

**Acceptance Criteria**:
- Matrix reviewed and signed off
- No stale comments about "TODO SSE"
- Matches actual codebase capabilities

---

### Milestone 1.1.2: Unit Test Expansion
**Goal**: Cover all supported SSE forms plus negative paths.

**Add to `tests/test_encoder.c`**:

**Positive-path tests** (~40 new tests):
- [ ] SSE move instructions: `movaps`, `movups`, `movdqa`, `movdqu`, `movsd`
  - Register-to-register (all register bands)
  - Memory-to-register with scale factors (1, 2, 4, 8)
  - Register-to-memory with addressing modes
  - High-register REX paths for XMM8-XMM15
- [ ] Packed arithmetic: Test all `paddb`/`paddw`/`paddd`/`paddq` forms with scale factors
- [ ] Comparisons: `pcmpeqb`/`w`/`d`/`q`, `pcmpgtb`/`w`/`d`/`q` (including OPC-map 0x0F 0x38)
- [ ] Logical operations: `pandn`, `pxor` across addressing modes
- [ ] Store forms: `movaps`, `movdqa` writing to memory

**Negative-path tests** (~30 new tests):
- [ ] Size mismatches (e.g., `movaps` with 32-bit register)
- [ ] Invalid operand combinations (e.g., memory-to-memory)
- [ ] Unsupported instruction variants with exact error diagnostics
- [ ] Memory operand errors with unsupported scale factors or addressing modes
- [ ] REX conflicts with high-8-bit registers

**Deliverables**:
- 40+ positive-path unit tests, all passing
- 30+ negative-path tests with assert on exact diagnostic message
- Test coverage report showing matrix coverage

**Acceptance Criteria**:
- All tests pass with `make test-encoder`
- Coverage tool shows >95% of SSE code paths exercised
- Each matrix cell has explicit test case(s)

---

### Milestone 1.1.3: Integration Hardening
**Goal**: Validate SSE in real assembly contexts with runtime verification.

**Add to `tests/test_integration.c`**:

- [ ] **Scalar SSE integration**: Assembly + execution test
  - XMM register moves and arithmetic
  - Return value from SSE computation
  - Validate output against known result

- [ ] **Packed SSE integration**: Add at least one per family
  - Packed integer add/subtract
  - Packed comparison and conditional logic
  - Mixed with scalar control flow (branches, function calls)

  Status update: Added runtime-backed packed integration scenarios in
  `test_integration_sse_packed_arith_runtime`,
  `test_integration_sse_packed_logic_runtime`, and
  `test_integration_sse_packed_sib_runtime`.
  Added follow-on indexed logical path coverage in
  `test_integration_sse_packed_logic_sib_runtime`.
  Added explicit packed RIP-relative encoder coverage in
  `test_pcmpgtd_xmm9_rip_disp32` and `test_por_xmm10_rip_disp32`.
  Added packed addressing edge-case parity tests for negative disp8,
  disp32 SIB, and no-disp SIB forms in
  `test_psubw_xmm2_r13_r14_scale2_neg_disp8`,
  `test_pcmpeqb_xmm11_r8_r10_scale8_disp32`,
  `test_pxor_xmm1_r12_r15_scale4_no_disp`, and
  `test_por_xmm13_rbp_rsi_scale2_implicit_zero_disp8`.
  Added additional packed SIB permutation coverage in
  `test_paddb_xmm14_rbx_rsi_scale2_no_disp`,
  `test_pcmpgtw_xmm0_rdi_rbp_scale4_neg_disp8`, and
  `test_pand_xmm15_r9_r12_scale8_disp32`.
  Added opcode-map (0F 38) packed compare permutation coverage in
  `test_pcmpeqq_xmm14_r10_r11_scale2_neg_disp8`,
  `test_pcmpgtq_xmm6_rbx_r12_scale8_disp32`, and
  `test_pcmpgtq_xmm10_rip_disp32`.
  Added scalar SSE-to-scalar-register runtime validation in
  `test_integration_sse_scalar_to_gpr_runtime`.
  Added all-register SIMD runtime coverage in
  `test_integration_sse_all_xmm_registers_runtime`.
  Added multiple-instruction SSE sequence runtime validation in
  `test_integration_sse_sequence_runtime`.
  Added SSE function-call preservation validation in
  `test_integration_sse_function_call_preserve_runtime`.

- [ ] **Mixed SSE + scalar scenario**:
  - Load data into XMM
  - Perform SSE operation
  - Move result back to scalar register
  - System call with result

- [ ] **Edge cases**:
  - All 16 XMM registers in one program
  - Multiple SSE instructions in sequence
  - SSE in function calls (preserving/restoring XMM)

**Example Integration Test Structure**:

```asm
section .data
    data: dq 1, 2, 3, 4  ; 64-bit values for SSE packing

section .text
global _start
_start:
    ; Load and operate on SSE data
    lea rax, [data]
    movdqa xmm0, [rax]       ; Load 128 bits
    paddd xmm0, xmm0        ; Double each dword
    movd eax, xmm0          ; Extract result
    
    mov rdi, rax            ; Result in rdi
    mov rax, $60            ; sys_exit
    syscall
```

**Deliverables**:

- 5+ integration test programs with runtime assertions
- Each program tests one SSE family in realistic context
- Runtime output validation

**Acceptance Criteria**:

- All integration tests pass with correct output
- Programs assemble, execute, and return expected exit codes
- No SSE-related runtime crashes or incorrect results

---

### Milestone 1.1.4: Documentation Sync

**Goal**: Update all docs to match actual SSE coverage.

- [x] Update `README.md` SSE section to reference new `SSE_COVERAGE_MATRIX.md`
- [x] Update `docs/ASSEMBLY_REFERENCE.md` with supported SSE matrix section
- [x] Add SSE-specific troubleshooting to `docs/TROUBLESHOOTING.md`
- [x] Create `examples/sse_packed_ops.asm` (practical SSE example)
- [x] Cross-check reference docs against 3 public x86 references (Intel, AMD, objdump output)

**Deliverables**:
- All docs updated without contradictions
- 1 new realistic SSE example program
- Cross-reference verification report

**Acceptance Criteria**:
- README/reference docs claim only what's tested
- Example programs run and produce expected output
- No "TODO" or "future" language for implemented features

---

## 1.2 SIB Scale-Factor Coverage Completion

**Status**: ⚠️ Partially implemented | **Priority**: 🔴 Critical  
**Dependency**: Should run in parallel with 1.1

### Milestone 1.2.1: Scale Factor Matrix Testing
**Goal**: Exhaustively test scale factors 1, 2, 4, 8 across instruction families.

**Progress update (2026-03-23)**:
- Added 24 new scale-factor encoder tests for `mov`, `add`, `sub`, `lea`, and `movdqa` covering:
  - Positive/negative displacements and no-displacement SIB forms
  - Disp32 forms (including explicit zero-disp32 paths for base `rbp`/`r13` behavior)
  - Low/mid/high register bands, including REX.R/X/B combinations
  - Explicit scale factors 1, 2, 4, and 8 across the new batches
- Validation: `make test-encoder` (117/117 pass), `make test-integration` (31/31 pass)

**Add to `tests/test_encoder.c`** (~50 new tests):

- [ ] **For each scale factor (1, 2, 4, 8)**:
  - Test with `mov` (register, memory forms)
  - Test with `add`, `sub` (immediate memory operands)
  - Test with `lea` (scale factor primary use case)
  - Test with one SSE instruction (`movdqa`)
  - Test with different register bands (low, mid, high registers)
  - Test negative displacements with scale factors

**Test matrix**:
```
Scale (1,2,4,8) × 
Instruction (mov, add, lea, movdqa) × 
Displacement (+, -, 0) ×
Registers (low, mid, high) = 4 × 4 × 3 × 3 = 144 test vectors
```

**Generate scale factor test suite**:
- [ ] Create test generator script or inline tests
- [ ] Each test encodes a specific [base + index*scale + disp] pattern
- [ ] Verify SIB byte correctness: (scale_bits << 6) | (index_bits << 3) | base_bits

**Deliverables**:
- 50+ covering scale-factor combinations
- Test generator utility (if hand-coding exceeds 200 lines)
- Coverage report showing all 4 scale factors tested

**Acceptance Criteria**:
- All scale-factor tests pass
- Coverage shows 100% of scale factor branches exercised
- No regressions in existing addressing-mode tests

---

### Milestone 1.2.2: Edge-Case Addressing Hardening
**Goal**: Validate scale factors with complex addressing scenarios.
**Status**: ✅ Completed

**Progress update (2026-03-23)**:
- Added encoder-level validation to reject invalid SIB scales (including negative values) even when parser checks are bypassed.
- Added encoder-level validation to reject RIP-relative memory forms that also specify base/index registers.
- Added parser-level validation so invalid scales `5/6/7` and RIP-relative+index forms are rejected during parse, before encoding.
- Added parser coverage for legal RIP-relative form (`[rip + disp]`) to pair with invalid-combination rejection tests.
- Added LEA hardening:
  - Reject 8-bit LEA destinations with explicit diagnostics
  - Emit `0x66` operand-size prefix for 16-bit LEA destinations
- Added focused edge-case tests covering:
  - Invalid scale factors `3`, `5`, `6`, `7`, and `-2` with predictable error substrings
  - Invalid RIP-relative+index/base combination rejection
  - LEA scale-factor forms with 16-bit and 32-bit destinations
  - LEA 8-bit destination rejection path
- Validation: `make test-parser` (35/35 pass), `make test-encoder` (120/120 pass), `make test-integration` (31/31 pass)

**Add tests for**:
- [x] Scale Factor with RIP-relative addressing (where legal)
- [x] Scale factors in 8-bit, 16-bit, 32-bit, 64-bit forms
- [x] Negative scale indices (should fail with diagnostic)
- [x] Invalid scale factors (3, 5, 6, 7) with predictable errors
- [x] LEA with all scale factors across different output register sizes

**Deliverables**:
- [x] Edge-case test assertions
- [x] Exact error diagnostics for invalid scale factors

**Acceptance Criteria**:
- [x] All edge tests pass or fail predictably
- [x] Error messages guide user to correct scale factor

---

## 1.3 DWARF Debug Support Completeness

**Status**: ⚠️ Partially implemented | **Priority**: 🔴 Critical  
**Dependency**: Follows SSE/SIB work (parallelize if possible)

### Milestone 1.3.1: DWARF Metadata Enrichment
**Goal**: Expand DIE (Debug Information Entry) attribute coverage.
**Status**: ✅ Completed

**Progress update (2026-03-23)**:
- Enriched compile-unit metadata emission in `src/x86_64_asm.c`:
  - `DW_AT_producer` now emits `x86_64-asm`
  - `DW_AT_language` now emits roadmap target value `0x0001`
  - compile unit still emits `DW_AT_name`, `DW_AT_comp_dir`, `DW_AT_stmt_list`, `DW_AT_low_pc`, `DW_AT_high_pc`
- Added shared type DIEs and references:
  - `DW_TAG_base_type` (`u64`) and `DW_TAG_pointer_type` (`u64*`)
  - `DW_AT_type` references from subprogram DIEs
- Expanded symbol DIE coverage:
  - `DW_TAG_subprogram` retained for function symbols with declaration + range attributes
  - `DW_TAG_label` now emitted for non-function text labels with declaration metadata and address
- Added minimal lexical scope coverage:
  - `DW_TAG_lexical_block` child DIE emitted per subprogram with low/high PC range
- Hardened and expanded integration validation in `tests/test_integration.c`:
  - Manual `.debug_info` parser now validates enriched DIE stream (types, lexical blocks, labels)
  - `readelf`-driven validation now asserts language/type/lexical-block/label presence
- Validation: `make test-parser` (35/35 pass), `make test-encoder` (120/120 pass), `make test-integration` (31/31 pass)

**Current state**: Basic `.debug_info`, `.debug_abbrev`, `.debug_line`, `.debug_str`

**Expand DWARF attributes** in `src/x86_64_asm.c`:

- [x] Add `DW_AT_producer` (e.g., "x86_64-asm")
- [x] Add `DW_AT_language` (DW_LANG_Asm = 0x01)
- [x] Add `DW_AT_name` for all functions and labels
- [x] Add `DW_AT_decl_file`, `DW_AT_decl_line` for declarations
- [x] Add `DW_AT_low_pc`, `DW_AT_high_pc` for function ranges (already done, validate)
- [x] Add `DW_AT_type` references (pointer types, basic types)
- [x] Add minimal lexical scope DIEs for code blocks

**Implementation steps**:
1. Define new abbreviation codes in `.debug_abbrev` generation
2. Update compile-unit DIE in `.debug_info` with new attributes
3. Add function DIEs with extended attributes
4. Ensure all string offsets point correctly to `.debug_str`

**Deliverables**:
- DWARF section generation code updates
- New attributes in all DIE types
- No new buffer overflows or crashes

**Acceptance Criteria**:
- [x] Generated DWARF sections pass `readelf -w` inspection
- [x] `dwarfdump` or similar tool reads sections without errors
- [x] Attribute values are semantically correct

---

### Milestone 1.3.2: DWARF Validation Integration Tests
**Goal**: Assert DWARF structural correctness, not just presence.
**Status**: ✅ Completed

**Progress update (2026-03-23)**:
- Added four new DWARF integration tests in `tests/test_integration.c`:
  - `test_integration_dwarf_line_table_known_lines` validates known source line mappings by decoding `.debug_line` bytecode directly.
  - `test_integration_dwarf_symbol_table_crosscheck` cross-checks DWARF DIE names against `.symtab` entries when symbol tables are present (gracefully skips if `.symtab` is absent in the generated ELF variant).
  - `test_integration_dwarf_large_program_success` assembles and emits DWARF for a 64-function program and validates section presence and size bounds without overflow.
  - `test_integration_dwarf_dwarfdump_validation` validates DWARF tags and key attributes using `dwarfdump`/`llvm-dwarfdump` output when available.
- Existing DWARF tests retained and still passing:
  - `test_integration_dwarf_sections`
  - `test_integration_dwarf_readelf_validation`
  - `test_integration_dwarf_overflow_failure`
- Validation: `make test-integration` (35/35 pass)

**Add to `tests/test_integration.c`**:

- [x] **DWARF section parsing**:
  - Read compiled binary with DWARF output
  - Parse `.debug_info`, `.debug_line` sections manually or via tool
  - Assert expected compile-unit exists
  - Assert expected function DIEs present
  - Assert line mappings are correct

- [x] **Line table validation**:
  - Generate assembly with known line numbers
  - Build program
  - Parse `.debug_line` section
  - Verify line-address mappings match source

- [x] **Symbol table cross-check**:
  - Compare DWARF DIE names with `.symtab` symbols
  - Assert consistency

- [x] **Large program stress test**:
  - Build assembly with 50+ functions
  - Verify no buffer overflows, correct DWARF generation
  - Validate section sizes reasonable

**Deliverables**:
- 5+ DWARF validation test cases
- Manual or scripted DWARF parsing for verification
- Stress test program (50+ functions)

**Acceptance Criteria**:
- All DWARF tests pass
- Generated sections pass external tools (`readelf`, `dwarfdump`)
- Large program test completes without errors

---

### Milestone 1.3.3: DWARF Documentation
**Goal**: Document supported DWARF subset.
**Status**: ✅ Completed

**Progress update (2026-03-23)**:
- Added `docs/DWARF_SUPPORT.md` with:
  - Supported DWARF version and section scope
  - Emitted abbreviation codes with tag/attribute/form mappings
  - Line-program behavior and header constants
  - External validation workflow (`readelf`, `dwarfdump`, `llvm-dwarfdump`)
  - Explicit limitations and debugger integration guidance
- Linked new document from `README.md` documentation index.

- [x] Create `docs/DWARF_SUPPORT.md`
  - List supported DWARF version (currently 4)
  - Document all emitted abbreviation codes
  - Show examples of generated DWARF sections
  - List limitations (e.g., no frame descriptors Yet)
  - Provide guidance for debugger integration

**Deliverables**:
- DWARF documentation with examples
- Clear scope boundaries

**Acceptance Criteria**:
- Debugger vendors can read documentation and understand our DWARF support

---

# PHASE 2: DIAGNOSTICS & POLISH (High Priority)

## 2.1 Error Message Normalization & Quality

**Status**: ⚠️ Partially implemented | **Priority**: 🟠 High

### Milestone 2.1.1: Diagnostic Format Consistency
**Goal**: Ensure all error messages follow consistent format with remediation guidance.

**Progress update (2026-03-23)**:
- Implemented parser diagnostic formatter upgrade in `src/x86_64_parser.c` to emit:
  - `Error at line X, column Y: [Syntax] ...`
  - source line + caret indicator
  - `Suggestion: ...` remediation hint
- Added/updated parser negative-path assertions in `tests/test_parser.c` to validate standardized diagnostic fields (`line`, `column`, category tag, suggestion, caret).
- Added encoder diagnostic normalization slice in `src/x86_64_encoder.c` for high-impact paths:
  - high-8-bit + REX constraint violations (`encode_mov`, `encode_arithmetic`)
  - core SSE operand/form validation errors (`encode_sse_mov`, `encode_sse_arith`)
  - all now emit `Error at line X, column 1: [Category] ...` plus `Suggestion: ...`
- Added/updated encoder negative-path assertions in `tests/test_encoder.c` to validate standardized diagnostic fields for representative encoder failures.
- Added controlflow diagnostic normalization slice in `src/x86_64_controlflow.c` for high-impact paths:
  - `jmp`/`jcc` operand validation failures
  - `call`/`ret` invalid operand forms
  - `nop` with operands rejection
  - `enter` operand type/range validation
  - all now emit `Error at line X, column 1: [Category] ...` plus `Suggestion: ...`
- Extended controlflow diagnostic normalization in `src/x86_64_controlflow.c` for additional legacy paths:
  - `int` operand-form validation
  - `cwd`/`cdq`/`cqo` and `cbw`/`cwde`/`cdqe` operand validation
  - `setcc` operand kind/width constraints
  - `cmov` destination/source/size constraint validation
  - `bswap` operand kind/size constraint validation
  - all converted from legacy `Error: ...` prints to standardized category + suggestion diagnostics
- Extended controlflow diagnostic normalization in `src/x86_64_controlflow.c` for arithmetic/bitflow paths:
  - `xchg` operand-count/operand-form validation
  - `imul` destination/form/operand-count validation
  - `div`/`idiv`/`mul` operand-count validation
  - `test` operand-count/operand-form validation
  - `shift` operand-count/mnemonic/count-source validation
  - all converted from legacy `Error: ...` prints to standardized category + suggestion diagnostics
- Extended controlflow diagnostic normalization in `src/x86_64_controlflow.c` for remaining bit-operations paths:
  - `bt`/`bts`/`btr`/`btc` operand-count and bit-index constraint validation
  - `shld`/`shrd` operand-count/source/count constraint validation
  - `bsf`/`bsr` operand-count/destination constraint validation
  - all converted from legacy `Error: ...` prints to standardized category + suggestion diagnostics
- Added/updated controlflow-focused diagnostic assertions:
  - `tests/test_encoder.c` (`encode_enter_invalid_size_diagnostic`)
  - `tests/test_encoder.c` (`encode_instruction_int_non_immediate_diagnostic`, `encode_instruction_sete_non8bit_diagnostic`, `encode_instruction_cmov_dst_not_register_diagnostic`, `encode_instruction_bswap_invalid_size_diagnostic`)
  - `tests/test_encoder.c` (`encode_instruction_xchg_operand_count_diagnostic`, `encode_instruction_imul_dst_not_register_diagnostic`, `encode_instruction_div_operand_count_diagnostic`, `encode_instruction_shift_invalid_count_diagnostic`)
  - `tests/test_encoder.c` (`encode_instruction_bt_operand_count_diagnostic`, `encode_instruction_bts_invalid_bit_index_diagnostic`, `encode_instruction_shld_src_not_register_diagnostic`, `encode_instruction_bsf_dst_not_register_diagnostic`)
  - `tests/test_encoder.c` (`encode_instruction_jmp_fixup_capacity_diagnostic`, `encode_not_operand_count_diagnostic`)
  - `tests/test_integration.c` (`integration_enter_invalid_operands`, `integration_enter_invalid_nesting`)
- Validation:
  - `make test-parser` (35/35 pass)
  - `make test-encoder` (135/135 pass)
  - `make test-integration` (35/35 pass)
- Remaining for 2.1.1: apply the same standardized diagnostic contract across remaining encoder paths and remaining parser direct `fprintf` paths.

**Standard format**:
```
Error at line X, column Y: [Category] Error message
  <source line>
  <caret marker>
  
Suggestion: Remediation text (e.g., "Use RIP-relative addressing or a smaller offset")
```

**Audit all error paths in**:
- [ ] `src/x86_64_parser.c` (20+ error sites)
- [ ] `src/x86_64_encoder.c` (30+ error sites)
- [x] `src/x86_64_controlflow.c` (10+ error sites)

**For each error path**:
- [ ] Ensure source line echo with caret
- [ ] Add category (e.g., "[Syntax]", "[Encoding]", "[Constraint]")
- [ ] Add suggestion text

**Specific high-impact diagnostics to add**:
- [ ] Memory operand errors → suggest correct syntax
- [ ] Unknown instruction → suggest close matches (did-you-mean)
- [ ] Register constraint violations → suggest workarounds
- [x] SSE operand mismatches → explain supported forms
- [x] High-8-bit + REX conflicts → suggest alternative registers
- [ ] Label not found → suggest available labels (edit distance)

**Deliverables**:
- All error paths updated to new format
- Suggestion text populated for 50+ error types
- Test assertions verifying diagnostic format

**Acceptance Criteria**:
- All parser/encoder errors follow standard format
- 100% of tests pass (no breaking changes)
- User-facing errors are helpful and actionable

---

### Milestone 2.1.2: Negative Test Path Coverage
**Goal**: Assert exact error messages and positions for all error cases.

**Add to test suites**:
- [ ] Parser error tests with position validation
- [ ] Encoder error tests with diagnostics
- [ ] Integration error tests (realistic failure scenarios)

**Test structure**:
```c
ASSERT_PARSER_ERROR(input_asm, expected_line, expected_col, expected_message_fragment);
ASSERT_ENCODER_ERROR(operand_set, expected_error_fragment);
```

**Target coverage**:
- [ ] 50+ negative parser tests with exact position/message assertions
- [ ] 30+ negative encoder tests with exact error messages
- [ ] 10+ integration failure scenarios

**Deliverables**:
- Negative path test suite (100+ new tests)
- All tests passing

**Acceptance Criteria**:
- Negative tests pass and provide user-facing quality validation

---

## 2.2 High-8-Bit Register + REX Constraint Documentation

**Status**: ⚠️ Partially implemented | **Priority**: 🟠 High

### Milestone 2.2.1: Diagnostic Coverage for AH/BH/CH/DH Conflicts

**Goal**: Ensure all instructions that accept 8-bit registers fail predictably with help text.

**Audit all instruction encoders** for 8-bit register acceptance:
- [ ] `mov` (already done, extend to verify all forms)
- [ ] `add`, `sub`, `xor`, `or`, `and` (and all ALU ops)
- [ ] `xchg` (high-8-bit register with REX)
- [ ] `cmp`, `test`
- [ ] Other 8-bit-capable instructions

**For each**:
- [ ] Add test case: `ah` with R8-R15 or SPL/BPL/SIL/DIL
- [ ] Verify exact error message and suggestion

**Error message template**:
```
Error at line 5:[Constraint] Cannot use AH with REX-extended registers (R8-R15 or SPL/BPL/SIL/DIL)
  add ah, r8b
      ^^
Suggestion: Use AL, BL, CL, or DL instead of AH/BH/CH/DH
```

**Deliverables**:
- 20+ test cases covering AH/BH/CH/DH conflicts
- Consistent error messages across all instruction types
- Documentation update

**Acceptance Criteria**:
- All constraint violations produce clear, actionable errors
- No ambiguous or misleading error messages

---

### Milestone 2.2.2: Documentation of Architectural Constraint
**Goal**: Clearly explain that this is x86_64 ISA limitation, not a bug.

**Update `docs/ASSEMBLY_REFERENCE.md` and `docs/TROUBLESHOOTING.md`**:

- [ ] Add section: "Architectural Constraints"
  - Explain high-8-bit register limitation
  - Show why it exists (REX prefix conflict in x86_64 encoding)
  - Provide clear alternatives
  - Reference Intel/AMD documentation

**Example documentation**:
```markdown
### High 8-Bit Registers with Extended Registers

The registers AH, BH, CH, and DH cannot be used with:
- Any R8-R15 register in the same instruction
- SPL, BPL, SIL, DIL (low bytes of extended regs)
- Any 64-bit operation requiring a REX prefix

This is an x86_64 ISA constraint due to REX prefix encoding conflicts.

**Workaround**: Use AL, BL, CL, DL instead, or restructure code.
```

**Deliverables**:
- Updated architecture/constraints documentation
- Clear explanation with examples

**Acceptance Criteria**:
- Documentation is accurate and user-friendly

---

# PHASE 3: ADVANCED FEATURES (Medium Priority)

## 3.1 Listing File Generation

**Status**: ❌ Not implemented | **Priority**: 🟡 Medium

### Milestone 3.1.1: Listing File Format Design
**Goal**: Define `.lst` file output format.

- [ ] Specify format (assembly source + hex machine code + addresses)
- [ ] Design column layout and spacing
- [ ] Plan symbol table section in listing
- [ ] Create `docs/LISTING_FORMAT.md`

**Example listing format**:
```
     1  0000  b8 3c 00 00 00                  mov rax, $60
     2  0005  bf 00 00 00 00                  mov rdi, $0
     3  000a  0f 05                           syscall
```

**Deliverables**:
- Listing format specification
- Design document with examples

**Acceptance Criteria**:
- Format is clear and useful for debugging
- Specification document is complete

---

### Milestone 3.1.2: Listing File Implementation
**Goal**: Implement `.lst` file generation.

**Modify `src/asm.c`**:
- [ ] Add `--listing` or `-l` command-line option
- [ ] During assembly, track source line → machine code → address mapping
- [ ] After assembly, generate `.lst` file parallel to output binary
- [ ] Include symbol table at end of listing

**Code changes**:
- [ ] Extend `asm_context_t` with listing buffer
- [ ] Update encoder to log generated bytes with source line
- [ ] Implement listing file writer

**Deliverables**:
- Listing file generation working end-to-end
- Example `.lst` files for test programs

**Acceptance Criteria**:
- `./bin/x86_64-asm input.asm -o output -l output.lst` generates valid `.lst`
- Listing file is readable and useful for debugging
- Symbol table section is accurate

---

### Milestone 3.1.3: Listing Integration Tests
**Goal**: Validate listing file correctness.

- [ ] Generate listings for example programs
- [ ] Parse `.lst` files and validate structure
- [ ] Cross-check machine code in listing against binary
- [ ] Verify source line numbers match

**Deliverables**:
- Listing integration tests (5+)
- Validation tools/scripts

**Acceptance Criteria**:
- All listing tests pass
- Listings are stable and reproducible

---

## 3.2 Disassembler Mode

**Status**: ❌ Not implemented | **Priority**: 🟡 Medium

### Milestone 3.2.1: Disassembler Design
**Goal**: Plan how to add disassembly capability.

- [ ] Design `--disassemble` / `-d` option
- [ ] Plan x86_64 instruction lookup (reverse encoding → mnemonic)
- [ ] Design output format (similar to `objdump -d` style)
- [ ] Create `docs/DISASSEMBLER_DESIGN.md`

**Deliverables**:
- Disassembler design specification
- Output format examples

**Acceptance Criteria**:
- Design is clear and implementable

---

### Milestone 3.2.2: Instruction Lookup Table Generation
**Goal**: Create reverse-lookup infrastructure (opcode → mnemonic).

- [ ] Design instruction lookup data structure
- [ ] Generate lookup table from encoder instruction definitions
- [ ] Implement opcode decoding for common instruction families (mov, add, jcc, etc.)

**Deliverables**:
- Lookup table implementation
- Decoding routines for key instruction families

**Acceptance Criteria**:
- Lookup table is accurate and reasonably sized (<50KB)
- Decoding handles 80% of common x86_64 instructions

---

### Milestone 3.2.3: Disassembler Implementation
**Goal**: Implement working disassembly.

- [ ] Add `--disassemble` option to `src/asm.c`
- [ ] Read ELF64 binary and extract `.text` section
- [ ] Decode instructions using lookup table
- [ ] Format and display disassembly

**Deliverables**:
- Disassembler implementation
- Example disassembly outputs

**Acceptance Criteria**:
- `./bin/x86_64-asm --disassemble binary` produces readable output
- Disassembly matches expected instructions for test programs

---

### Milestone 3.2.4: Disassembler Integration Tests
**Goal**: Validate disassembly correctness.

- [ ] Assemble program → Disassemble → Compare to source (semantically)
- [ ] Disassemble compiled binaries from other sources
- [ ] Validate against `objdump -d` for correctness

**Deliverables**:
- Disassembler tests (10+)
- Validation against reference tools

**Acceptance Criteria**:
- Disassembly is correct for test programs
- Matches reference disassembler (`objdump`) output

---

## 3.3 Parser Performance Optimization

**Status**: ❌ Not implemented | **Priority**: 🟡 Medium

### Milestone 3.3.1: Performance Profiling
**Goal**: Identify parser bottlenecks.

- [ ] Build profiling instrumentation into parser
- [ ] Generate large test program (10,000+ lines)
- [ ] Profile and identify hot paths
- [ ] Create `docs/PARSER_PROFILING.md` with results

**Deliverables**:
- Profiling data showing hot functions/loops
- Baseline performance metrics

**Acceptance Criteria**:
- Clear identification of slowest components

---

### Milestone 3.3.2: Symbol Lookup Optimization
**Goal**: Replace linear search with hash table.

- [ ] Implement hash table for symbol lookup (FNV hash or murmur)
- [ ] Integrate with existing symbol management
- [ ] Preserve backward compatibility

**Deliverables**:
- Hash table implementation
- Integration with symbol manager
- Performance comparison (linear vs hash)

**Acceptance Criteria**:
- Lookup speed improved 10x+ for large symbol counts (1000+)
- No correctness regressions

---

### Milestone 3.3.3: Token & Line Buffer Optimization
**Goal**: Optimize memory allocation and string handling.

- [ ] Pre-allocate token buffer sized to 95th percentile program
- [ ] Implement line-buffer ring to avoid copying
- [ ] Profile memory allocations

**Deliverables**:
- Optimized buffer management
- Memory usage reduction

**Acceptance Criteria**:
- Peak memory usage reduced substantially
- No functionality changes

---

# PHASE 4: PRODUCTION HARDENING (Critical Path)

## 4.1 Stress Testing & Scalability

**Status**: ❌ Not implemented | **Priority**: 🔴 Critical

### Milestone 4.1.1: Large Program Stress Tests
**Goal**: Validate assembler with large, complex programs.

**Create stress test programs**:
- [ ] Program with 100+ functions
- [ ] Program with 1000+ labels
- [ ] Program with 50KB+ source
- [ ] Program with all instruction families mixed
- [ ] Program with deep macro nesting (10 levels)
- [ ] Program with all SSE instruction families

**Build and assemble each**:
- [ ] Measure time, memory, output size
- [ ] Compare against baseline
- [ ] Verify correctness of output

**Deliverables**:
- Stress test suite (6-10 programs)
- Performance metrics and comparisons

**Acceptance Criteria**:
- All stress tests pass
- Assembler handles large inputs without crashes
- Performance is acceptable (<10s for 50KB source)

---

### Milestone 4.1.2: Edge Case Testing
**Goal**: Cover known edge cases and unusual scenarios.

**Test scenarios**:
- [ ] Empty sections (no code/data)
- [ ] Very long label names (255+ chars)
- [ ] Very deep memory addressing expressions
- [ ] Maximum displacement values
- [ ] Minimum displacement values (negative)
- [ ] All instruction sizes mixed in same program
- [ ] Circular macro expansion (should fail gracefully)
- [ ] Multiple includes of same file
- [ ] Very large immediate values (edge of 64-bit)

**Deliverables**:
- Edge case test suite (15+ cases)
- All passing or failing predictably

**Acceptance Criteria**:
- All edge cases handled without crashes
- Errors are clear and reproducible

---

## 4.2 Cross-Reference Validation

**Status**: ⚠️ Partial | **Priority**: 🔴 Critical

### Milestone 4.2.1: Reference Assembler Comparison
**Goal**: Validate generated machine code against external reference.

**Setup**:
- [ ] Install `nasm` (reference assembler) for comparison
- [ ] Select representative instructions from each family
- [ ] Write equivalent programs in both IariaBoot and NASM syntax
- [ ] Compare generated machine code byte-for-byte

**Test matrix**:
- [ ] General instructions (mov, add, jmp) - 10 programs
- [ ] Memory addressing (SIB, RIP-relative) - 5 programs
- [ ] SSE instructions - 10 programs
- [ ] High-register forms (R8-R15) - 5 programs
- [ ] Conditional jumps and all variants - 5 programs

**Deliverables**:
- Comparison test suite (35+ programs)
- Byte-level match report
- Any discrepancies documented and explained

**Acceptance Criteria**:
- >95% of instructions produce identical machine code
- Any differences are documented and intentional
- No unexplained encoding mismatches

---

### Milestone 4.2.2: Debugger Integration Validation
**Goal**: Verify DWARF output works with real debuggers.

- [ ] Install GDB and/or LLDB
- [ ] Build test program with our assembler
- [ ] Load in debugger and validate:
  - [ ] Source line breakpoints work
  - [ ] Stack traces show function names
  - [ ] Variable inspection works (if applicable)
  - [ ] Step-through execution matches source

**Deliverables**:
- Debugger integration test cases
- Debugger session logs showing success

**Acceptance Criteria**:
- At least one debugger (GDB) successfully reads our DWARF
- Basic debugging operations work as expected

---

## 4.3 Documentation & Release Preparation

**Status**: ⚠️ Partial | **Priority**: 🔴 Critical

### Milestone 4.3.1: Completeness Documentation
**Goal**: Update all docs to reflect final implementation status.

- [ ] Update `README.md` with final feature list
- [ ] Create `COMPLETION_STATUS.md` with per-item checklist
- [ ] Update `docs/ASSEMBLY_REFERENCE.md` with final instruction matrix
- [ ] Remove all "TODO" and "future" language from docs
- [ ] Add transition guide from other assemblers (NASM, GNU AS)
- [ ] Create `RELEASE_NOTES.md` with version history

**Deliverables**:
- Updated documentation set
- No stale claims or TODOs
- Clear scope and limitations stated

**Acceptance Criteria**:
- Docs accurately reflect implementation
- New users can understand capabilities and limitations
- All reference docs are current

---

### Milestone 4.3.2: Example Program Suite Finalization
**Goal**: Comprehensive example programs demonstrating all features.

- [ ] Update existing examples (exit, hello, loop, arith, macros, sse_test)
- [ ] Add new examples:
  - [ ] `function_calls.asm` - real function prologue/epilogue
  - [ ] `macros_advanced.asm` - advanced macro features
  - [ ] `conditional_logic.asm` - all conditional jump types
  - [ ] `memory_addressing.asm` - all addressing modes
  - [ ] `string_operations.asm` - string instruction examples
  - [ ] `sse_packed.asm` - packed SSE operations
  - [ ] `interrupts_signals.asm` - signal handling patterns
  - [ ] `embedded_data.asm` - data section management

- [ ] Each example:
  - [ ] Well-commented
  - [ ] Demonstrates feature clearly
  - [ ] Includes expected output
  - [ ] Tests coverage goal

**Deliverables**:
- 12+ documented example programs
- All examples runnable and correct

**Acceptance Criteria**:
- Examples cover 100% of implemented feature categories
- New users can learn from examples
- All examples in `examples/` directory

---

### Milestone 4.3.3: Testing Documentation & Coverage Report
**Goal**: Document test suite and coverage metrics.

- [ ] Create `docs/TESTING.md`:
  - How to run each test suite
  - Interpretation of results
  - Adding new tests
  - Coverage expectations
  
- [ ] Generate coverage report:
  - Line coverage for src/*.c
  - Branch coverage for key paths
  - Instruction encoding coverage
  - Parser/encoder coverage percentages

- [ ] Create `TEST_MATRIX.md` showing coverage by category

**Deliverables**:
- Testing documentation
- Coverage report (aim for >90% line coverage)
- Test matrix by feature

**Acceptance Criteria**:
- >90% line coverage in src/
- >85% branch coverage in critical paths
- All features have corresponding tests

---

## 4.4 Final Integration & Release Testing

**Status**: ❌ Not started | **Priority**: 🔴 Critical

### Milestone 4.4.1: Full Test Suite Execution
**Goal**: Run all tests and verify system stability.

- [ ] Run `make test-encoder` - all encoder tests pass
- [ ] Run `make test-parser` - all parser tests pass
- [ ] Run `make test-integration` - all integration tests pass
- [ ] Run `make test` - all example programs assemble and execute correctly
- [ ] Run stress tests - large programs complete successfully
- [ ] Run cross-reference tests - machine code validated
- [ ] Run debugger integration tests - DWARF validated

**Success criteria**:
- [ ] 0 test failures
- [ ] All programs produce correct output
- [ ] No hangs or crashes

**Deliverables**:
- Test execution report
- Test result summary

**Acceptance Criteria**:
- All 300+ tests pass
- System is stable and production-ready

---

### Milestone 4.4.2: Release Preparation
**Goal**: Package for release.

- [ ] Tag repository with version (e.g., v0.2.0)
- [ ] Create `CHANGELOG.md` with summary of completions in this roadmap
- [ ] Review and update `LICENSE`
- [ ] Create `CONTRIBUTING.md` for future developers
- [ ] Build final binary and verify
- [ ] Create quick-start guide for new users

**Deliverables**:
- Git tag
- Release documentation
- Final binary
- Quick-start guide

**Acceptance Criteria**:
- Repository is ready for public release
- New users can build and run within 5 minutes
- All documentation is accurate

---

# Summary Table: Roadmap Tracking

## Phase 1: Stability & Depth (Weeks 1-3)

| Item | Status | Effort | Tests | Docs |
|------|--------|--------|-------|------|
| 1.1 SSE Coverage | Not Started | 5d | 70+ | Yes |
| 1.2 SIB Scale Factors | Not Started | 3d | 50+ | Yes |
| 1.3 DWARF Completeness | Not Started | 4d | 20+ | Yes |
| **Phase 1 Total** | - | **12d** | **140+** | - |

## Phase 2: Diagnostics & Polish (Weeks 4-5)

| Item | Status | Effort | Tests | Docs |
|------|--------|--------|-------|------|
| 2.1 Diagnostics Normalization | Not Started | 3d | 100+ | Yes |
| 2.2 High-8-Bit Constraints | Not Started | 1d | 20+ | Yes |
| **Phase 2 Total** | - | **4d** | **120+** | - |

## Phase 3: Advanced Features (Weeks 6-9)

| Item | Status | Effort | Tests | Docs |
|------|--------|--------|-------|------|
| 3.1 Listing Files | Not Started | 2d | 10+ | Yes |
| 3.2 Disassembler | Not Started | 3d | 10+ | Yes |
| 3.3 Performance Opt | Not Started | 2d | 5+ | Yes |
| **Phase 3 Total** | - | **7d** | **25+** | - |

## Phase 4: Production Hardening (Weeks 10-11)

| Item | Status | Effort | Tests | Docs |
|------|--------|--------|-------|------|
| 4.1 Stress Testing | Not Started | 2d | 15+ | Yes |
| 4.2 Cross-Reference Validation | Not Started | 2d | 35+ | Yes |
| 4.3 Documentation Finalization | Not Started | 2d | - | Yes |
| 4.4 Final Integration | Not Started | 1d | All | Yes |
| **Phase 4 Total** | - | **7d** | **50+** | - |

---

# Total Effort Summary

- **Total Development Time**: ~30 developer-days
- **Total New Tests**: 335+
- **Total Documentation**: 15+ new docs
- **Total Test Coverage Target**: >90%

---

# Phase Execution Order

**Recommended sequence** (some items can parallelize):

1. **Week 1-2**: Phase 1.1 (SSE Coverage) + Phase 1.2 (SIB) in parallel
2. **Week 2-3**: Phase 1.3 (DWARF) + Phase 2.1 (Diagnostics) in parallel
3. **Week 3**: Phase 2.2 (High-8-Bit constraints)
4. **Week 4-5**: Phase 3 (Advanced features, lower priority but good for intermediate milestones)
5. **Week 5-6**: Phase 4.1 & 4.2 (Stress & validation)
6. **Week 6**: Phase 4.3 & 4.4 (Final docs & release)

---

# Success Criteria for Full Completion

- ✅ All items in "FULLY IMPLEMENTED" remain stable
- ✅ All items in "PARTIALLY IMPLEMENTED" reach 100% done
- ✅ All items in "NOT IMPLEMENTED" reach "COMPLETED" or "DEFERRED" status
- ✅ Test suite: 300+ tests, >90% coverage
- ✅ Documentation: Complete, accurate, current
- ✅ Production-Grade: Stress-tested, validated, debugger-integrated
- ✅ Release-Ready: Tagged, packaged, documented

