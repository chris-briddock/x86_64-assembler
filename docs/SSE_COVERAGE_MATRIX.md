# SSE Coverage Matrix

This document tracks the currently implemented SSE subset and where test coverage exists.
It is the authoritative matrix used by the roadmap for Phase 1 SSE completeness.

## Scope

- SIMD register width: XMM (128-bit)
- Register set: `xmm0` to `xmm15`
- Source forms currently accepted by SSE families:
  - Register source: `xmmN`
  - Memory source: `[mem]`
- Destination forms currently accepted by SSE families:
  - Most arithmetic/packed/logical: destination must be XMM register
  - SSE move families: destination may be XMM register or memory, with at least one XMM operand

## Instruction-Family Matrix

| Family | Mnemonics | Supported Forms | Parser/Encoder Status | Representative Tests |
| :--- | :--- | :--- | :--- | :--- |
| SSE moves (packed/scalar) | `movaps`, `movups`, `movss`, `movapd`, `movupd`, `movsd` (SSE) | `xmm, xmm`; `xmm, [mem]`; `[mem], xmm` | Implemented | `test_movaps_xmm0_xmm1`, `test_movups_xmm0_mem`, `test_movss_xmm1_xmm2`, `test_movapd_xmm8_complex_addr`, `test_movsd_mem_xmm1_rbp` |
| SSE arithmetic | `addss/addps/addsd/addpd`, `subss/subps/subsd/subpd`, `mulss/mulps/mulsd/mulpd`, `divss/divps/divsd/divpd` | `xmm, xmm`; `xmm, [mem]` | Implemented | `test_addss_xmm0_xmm1`, `test_divps_xmm4_mem`, `test_mulsd_xmm8_xmm9` |
| Packed integer move | `movdqa`, `movdqu` | `xmm, xmm`; `xmm, [mem]`; `[mem], xmm` | Implemented | `test_movdqa_xmm0_xmm1`, `test_movdqu_xmm8_mem` |
| Packed integer arithmetic | `paddb/paddw/paddd/paddq`, `psubb/psubw/psubd/psubq` | `xmm, xmm`; `xmm, [mem]` | Implemented | `test_paddb_xmm1_xmm2`, `test_paddw_xmm0_mem`, `test_psubq_xmm9_xmm10`, `test_psubd_xmm6_mem_disp` |
| Packed compare | `pcmpeqb/pcmpeqw/pcmpeqd/pcmpeqq`, `pcmpgtb/pcmpgtw/pcmpgtd/pcmpgtq` | `xmm, xmm`; `xmm, [mem]` | Implemented | `test_pcmpeqd_xmm7_xmm15`, `test_pcmpeqq_xmm1_xmm2`, `test_pcmpgtb_xmm3_mem_disp`, `test_pcmpgtq_xmm8_mem_disp` |
| Packed logic | `pand`, `pandn`, `por`, `pxor` | `xmm, xmm`; `xmm, [mem]` | Implemented | `test_pand_xmm4_xmm5`, `test_por_xmm2_mem`, `test_pandn_xmm3_xmm4`, `test_pxor_xmm12_mem_disp` |

Integration runtime evidence:
- `test_integration_sse_scalar_to_gpr_runtime`
- `test_integration_sse_all_xmm_registers_runtime`
- `test_integration_sse_sequence_runtime`
- `test_integration_sse_function_call_preserve_runtime`
- `test_integration_sse_packed_arith_runtime`
- `test_integration_sse_packed_logic_runtime`
- `test_integration_sse_packed_logic_sib_runtime`

Additional addressing/SIB parity evidence:
- `test_movups_xmm6_rbx_rcx_scale1_disp8`
- `test_addps_xmm11_r10_r12_scale8_disp8`
- `test_paddq_xmm5_r13_r14_scale2_disp8`
- `test_paddd_xmm1_rbx_rcx_scale4_disp8`
- `test_psubq_xmm3_r8_r9_scale8_disp8`
- `test_psubw_xmm2_r13_r14_scale2_neg_disp8`
- `test_paddb_xmm14_rbx_rsi_scale2_no_disp`
- `test_pxor_xmm7_rbx_rdx_scale1_disp8`
- `test_pxor_xmm1_r12_r15_scale4_no_disp`
- `test_pcmpeqw_xmm4_r11_r12_scale4_disp8`
- `test_pcmpgtd_xmm9_r12_r13_scale4_disp8`
- `test_pcmpeqb_xmm11_r8_r10_scale8_disp32`
- `test_pcmpgtw_xmm0_rdi_rbp_scale4_neg_disp8`
- `test_pcmpeqq_xmm14_r10_r11_scale2_neg_disp8`
- `test_pcmpgtq_xmm6_rbx_r12_scale8_disp32`
- `test_pcmpgtq_xmm10_rip_disp32`
- `test_pcmpgtd_xmm9_rip_disp32`
- `test_pand_xmm10_r8_r9_scale4_disp8`
- `test_pandn_xmm6_r10_r11_scale4_disp8`
- `test_por_xmm13_rbp_rsi_scale2_implicit_zero_disp8`
- `test_pand_xmm15_r9_r12_scale8_disp32`
- `test_por_xmm10_rip_disp32`
- `test_integration_sse_packed_sib_runtime`

Notes:
- `movsd` is disambiguated by operands: string form (`movsd` with no operands) vs SSE scalar form (`movsd` with XMM/memory operands).
- `pcmpeqq` and `pcmpgtq` use the `0F 38` opcode-map extension and are implemented.

## Operand-Form Matrix

| Family | `xmm, xmm` | `xmm, [mem]` | `[mem], xmm` | `[mem], [mem]` | `xmm, imm` | `gpr, xmm` |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| SSE moves | Yes | Yes | Yes | No | No | No |
| SSE arithmetic | Yes | Yes | No | No | No | No |
| Packed integer move | Yes | Yes | Yes | No | No | No |
| Packed integer arithmetic | Yes | Yes | No | No | No | No |
| Packed compare | Yes | Yes | No | No | No | No |
| Packed logic | Yes | Yes | No | No | No | No |

## Addressing-Mode Coverage Snapshot

This table captures current tested depth, not just parser acceptance.

| Addressing mode | Scalar SSE tests | Packed SSE tests | Status |
| :--- | :--- | :--- | :--- |
| Base register `[rbx]` | Yes | Yes | Covered |
| Base + disp8 `[rsp + 8]` | Yes | Yes | Covered |
| Base + disp32 `[rbp + 0x1234]` | Yes | Yes | Covered |
| SIB scale=1 | Yes | Yes | Covered |
| SIB scale=2 | Yes | Yes | Covered |
| SIB scale=4 | Yes | Yes | Covered |
| SIB scale=8 | Yes | Yes | Covered |
| RIP-relative `[rip + disp32]` | Yes | Yes | Covered |

## Register-Band Coverage Snapshot

| Register band | Coverage status | Evidence |
| :--- | :--- | :--- |
| `xmm0` to `xmm7` | Covered | Multiple move/arithmetic/packed tests |
| `xmm8` to `xmm15` | Covered | `test_movaps_xmm8_xmm9`, `test_psubq_xmm9_xmm10`, `test_pxor_xmm12_mem_disp` |
| Mixed low/high in one instruction | Covered | High-register REX-path tests in move/packed families |

## Guaranteed Negative Diagnostics (Current)

| Invalid shape | Expected diagnostic substring |
| :--- | :--- |
| SSE move with no XMM operand | `requires at least one XMM operand` |
| SSE move with mem-to-mem | `cannot have two memory operands` |
| SSE move mem destination without XMM source | `memory destination requires XMM source` |
| SSE move XMM destination with invalid source type | `XMM destination requires XMM or memory source` |
| SSE arithmetic destination not XMM | `destination must be XMM register` |
| SSE arithmetic source not XMM/memory | `source must be XMM or memory` |
| XMM operand size metadata mismatch | `must use xmm register size` |
| Packed/logical/compare destination not XMM | `destination must be XMM register` |
| Packed/logical/compare source not XMM/memory | `source must be XMM or memory` |

## Explicitly Unsupported Forms

| Unsupported form | Current behavior |
| :--- | :--- |
| Any SSE family with immediate source (for example `addps xmm0, $1`) | Rejected with source-type diagnostics |
| Any SSE family with general-purpose register operand (for example `addps rax, xmm0`) | Rejected with destination/source-type diagnostics |
| Memory-to-memory SSE move (for example `movaps [a], [b]`) | Rejected |
| Non-XMM destination for packed compare/logical/arithmetic | Rejected |

## Remaining Matrix Gaps (Phase 1 Targets)

- Continue expanding packed-family SIB parity with additional displacement-pattern and opcode-map combinations.
- Normalize all SSE diagnostics to a common format with optional remediation text.
