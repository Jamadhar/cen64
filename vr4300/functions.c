//
// vr4300/functions.c: VR4300 execution functions.
//
// CEN64: Cycle-Accurate Nintendo 64 Simulator.
// Copyright (C) 2014, Tyler J. Stachecki.
//
// This file is subject to the terms and conditions defined in
// 'LICENSE', which is part of this source code package.
//

#define VR4300_BUILD_FUNCS

#include "common.h"
#include "cpu.h"
#include "decoder.h"
#include "pipeline.h"

// Mask to negate second operand if subtract operation.
cen64_align(static const uint64_t vr4300_addsub_lut[2], 16) = {
  0x0ULL, ~0x0ULL
};

// Mask to select outputs for bitwise operations.
cen64_align(static const uint64_t vr4300_bitwise_lut[4][2], 64) = {
  {~0ULL,  0ULL}, // AND
  {~0ULL, ~0ULL}, // OR
  { 0ULL, ~0ULL}, // XOR
  { 0ULL,  0ULL}, // -
};

// Mask to kill the instruction word if "likely" branch.
cen64_align(static const uint32_t vr4300_branch_lut[2], 8) = {
  ~0U, 0U
};

// Mask to selectively sign-extend loaded values.
cen64_align(static const uint64_t vr4300_load_sex_mask[2], 16) = {
  ~0ULL, 0ULL
};

//
// ADD
// SUB
//
void VR4300_ADD_SUB(struct vr4300 *vr4300, uint64_t rs, uint64_t rt) {
  struct vr4300_rfex_latch *rfex_latch = &vr4300->pipeline.rfex_latch;
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  uint32_t iw = rfex_latch->iw;
  uint64_t mask = vr4300_addsub_lut[iw >> 1 & 0x1];

  unsigned dest;
  uint64_t rd;

  dest = GET_RD(iw);
  rt = (rt ^ mask) - mask;
  rd = rs + rt;

  assert(((rd >> 31) == (rd >> 32)) && "Overflow exception.");

  exdc_latch->result = (int32_t) rd;
  exdc_latch->dest = dest;
}

//
// ADDI
// SUBI
//
void VR4300_ADDI_SUBI(struct vr4300 *vr4300, uint64_t rs, uint64_t rt) {
  struct vr4300_rfex_latch *rfex_latch = &vr4300->pipeline.rfex_latch;
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  uint32_t iw = rfex_latch->iw;
  uint64_t mask = 0; //vr4300_addsub_lut[iw & 0x1];

  unsigned dest;

  dest = GET_RT(iw);
  rt = (int16_t) iw;
  rt = (rt ^ mask) - mask;
  rt = rs + rt;

  assert(((rt >> 31) == (rt >> 32)) && "Overflow exception.");

  exdc_latch->result = (int32_t) rt;
  exdc_latch->dest = dest;
}

//
// ADDIU
// SUBIU
//
void VR4300_ADDIU_SUBIU(struct vr4300 *vr4300, uint64_t rs, uint64_t rt) {
  struct vr4300_rfex_latch *rfex_latch = &vr4300->pipeline.rfex_latch;
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  uint32_t iw = rfex_latch->iw;
  uint64_t mask = 0; //vr4300_addsub_lut[iw & 0x1];

  unsigned dest;

  dest = GET_RT(iw);
  rt = (int16_t) iw;
  rt = (rt ^ mask) - mask;
  rt = rs + rt;

  exdc_latch->result = (int32_t) rt;
  exdc_latch->dest = dest;
}

//
// ADDU
// SUBU
//
void VR4300_ADDU_SUBU(struct vr4300 *vr4300, uint64_t rs, uint64_t rt) {
  struct vr4300_rfex_latch *rfex_latch = &vr4300->pipeline.rfex_latch;
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  uint32_t iw = rfex_latch->iw;
  uint64_t mask = vr4300_addsub_lut[iw >> 1 & 0x1];

  unsigned dest;
  uint64_t rd;

  dest = GET_RD(iw);
  rt = (rt ^ mask) - mask;
  rd = rs + rt;

  exdc_latch->result = (int32_t) rd;
  exdc_latch->dest = dest;
}

//
// AND
// OR
// XOR
//
void VR4300_AND_OR_XOR(struct vr4300 *vr4300, uint64_t rs, uint64_t rt) {
  struct vr4300_rfex_latch *rfex_latch = &vr4300->pipeline.rfex_latch;
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  uint32_t iw = rfex_latch->iw;
  uint64_t and_mask = vr4300_bitwise_lut[iw & 0x3][0];
  uint64_t xor_mask = vr4300_bitwise_lut[iw & 0x3][1];

  unsigned dest;
  uint64_t rd;

  dest = GET_RD(iw);
  rd = ((rs & rt) & and_mask) | ((rs ^ rt) & xor_mask);

  exdc_latch->result = rd;
  exdc_latch->dest = dest;
}

//
// ANDI
// ORI
// XORI
//
void VR4300_ANDI_ORI_XORI(struct vr4300 *vr4300, uint64_t rs, uint64_t rt) {
  struct vr4300_rfex_latch *rfex_latch = &vr4300->pipeline.rfex_latch;
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  uint32_t iw = rfex_latch->iw;
  uint64_t and_mask = vr4300_bitwise_lut[iw >> 26 & 0x3][0];
  uint64_t xor_mask = vr4300_bitwise_lut[iw >> 26 & 0x3][1];

  unsigned dest;

  dest = GET_RT(iw);
  rt = (uint16_t) iw;
  rt = ((rs & rt) & and_mask) | ((rs ^ rt) & xor_mask);

  exdc_latch->result = rt;
  exdc_latch->dest = dest;
}

//
// BEQ
// BEQL
// BNE
// BNEL
//
void VR4300_BEQ_BEQL_BNE_BNEL(struct vr4300 *vr4300, uint64_t rs, uint64_t rt) {
  struct vr4300_icrf_latch *icrf_latch = &vr4300->pipeline.icrf_latch;
  struct vr4300_rfex_latch *rfex_latch = &vr4300->pipeline.rfex_latch;

  uint32_t iw = rfex_latch->iw;
  uint32_t mask = vr4300_branch_lut[iw >> 30 & 0x1];
  uint64_t offset = (int16_t) iw << 2;

  bool is_ne = iw >> 26 & 0x1;
  bool cmp = rs == rt;

  if (cmp == is_ne) {
    rfex_latch->iw_mask = mask;
    return;
  }

  icrf_latch->pc = rfex_latch->common.pc + (offset + 4);
}

//
// BGEZ
// BGEZL
// BLTZ
// BLTZL
//
void VR4300_BGEZ_BGEZL_BLTZ_BLTZL(
  struct vr4300 *vr4300, uint64_t rs, uint64_t unused(rt)) {
  struct vr4300_icrf_latch *icrf_latch = &vr4300->pipeline.icrf_latch;
  struct vr4300_rfex_latch *rfex_latch = &vr4300->pipeline.rfex_latch;

  uint32_t iw = rfex_latch->iw;
  uint32_t mask = vr4300_branch_lut[iw >> 17 & 0x1];
  uint64_t offset = (int16_t) iw << 2;

  bool is_ge = iw >> 16 & 0x1;
  bool cmp = (int64_t) rs < 0;

  if (cmp == is_ge) {
    rfex_latch->iw_mask = mask;
    return;
  }

  icrf_latch->pc = rfex_latch->common.pc + (offset + 4);
}

//
// BGEZAL
// BGEZALL
// BLTZAL
// BLTZALL
//
void VR4300_BGEZAL_BGEZALL_BLTZAL_BLTZALL(
  struct vr4300 *vr4300, uint64_t rs, uint64_t unused(rt)) {
  struct vr4300_icrf_latch *icrf_latch = &vr4300->pipeline.icrf_latch;
  struct vr4300_rfex_latch *rfex_latch = &vr4300->pipeline.rfex_latch;
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  uint32_t iw = rfex_latch->iw;
  uint32_t mask = vr4300_branch_lut[iw >> 17 & 0x1];
  uint64_t offset = (int16_t) iw << 2;

  bool is_ge = iw >> 16 & 0x1;
  bool cmp = (int64_t) rs < 0;

  exdc_latch->result = rfex_latch->common.pc + 4;
  exdc_latch->dest = VR4300_REGISTER_RA;

  if (cmp == is_ge) {
    rfex_latch->iw_mask = mask;
    return;
  }

  icrf_latch->pc = rfex_latch->common.pc + (offset + 4);
}

//
// BGTZ
// BGTZL
// BLEZ
// BLEZL
//
void VR4300_BGTZ_BGTZL_BLEZ_BLEZL(
  struct vr4300 *vr4300, uint64_t rs, uint64_t unused(rt)) {
  struct vr4300_icrf_latch *icrf_latch = &vr4300->pipeline.icrf_latch;
  struct vr4300_rfex_latch *rfex_latch = &vr4300->pipeline.rfex_latch;

  uint32_t iw = rfex_latch->iw;
  uint32_t mask = vr4300_branch_lut[iw >> 30 & 0x1];
  uint64_t offset = (int16_t) iw << 2;

  bool is_gt = iw >> 26 & 0x1;
  bool cmp = (int64_t) rs <= 0;

  if (cmp == is_gt) {
    rfex_latch->iw_mask = mask;
    return;
  }

  icrf_latch->pc = rfex_latch->common.pc + (offset + 4);
}

//
// INV
//
void VR4300_INV(struct vr4300 *vr4300,
  uint64_t unused(rs), uint64_t unused(rt)) {
  struct vr4300_rfex_latch *rfex_latch = &vr4300->pipeline.rfex_latch;
  enum vr4300_opcode_id opcode = rfex_latch->opcode.id;

#ifndef NDEBUG
  fprintf(stderr, "Unimplemented instruction: %s [0x%.8X] @ 0x%.16llX\n",
    vr4300_opcode_mnemonics[opcode], rfex_latch->iw, (long long unsigned)
    rfex_latch->common.pc);
#endif

  assert(0 && "Unimplemented instruction encountered.");
}

//
// JALR
// JR
//
void VR4300_JALR_JR(struct vr4300 *vr4300, uint64_t rs, uint64_t unused(rt)) {
  struct vr4300_icrf_latch *icrf_latch = &vr4300->pipeline.icrf_latch;
  struct vr4300_rfex_latch *rfex_latch = &vr4300->pipeline.rfex_latch;
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  uint32_t iw = rfex_latch->iw;
  uint32_t mask = vr4300_branch_lut[iw & 0x1];

  exdc_latch->result = rfex_latch->common.pc + 4;
  exdc_latch->dest = VR4300_REGISTER_RA & mask;

  icrf_latch->pc = rs;
}

//
// LB
// LBU
// LH
// LHU
// LW
// LWU
//
// TODO/FIXME: Check for unaligned addresses.
//
void VR4300_LOAD(struct vr4300 *vr4300, uint64_t rs, uint64_t unused(rt)) {
  struct vr4300_rfex_latch *rfex_latch = &vr4300->pipeline.rfex_latch;
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  uint32_t iw = rfex_latch->iw;
  uint64_t sex_mask = vr4300_load_sex_mask[iw >> 28 & 0x1];
  unsigned dest = GET_RT(iw);

  exdc_latch->request.address = rs + (int16_t) iw;
  exdc_latch->request.type = VR4300_BUS_REQUEST_READ;
  exdc_latch->request.size = (iw >> 26 & 0x3) + 1;

  exdc_latch->result = sex_mask;
  exdc_latch->dest = dest;
}

//
// LUI
//
void VR4300_LUI(struct vr4300 *vr4300,
  uint64_t unused(rs), uint64_t unused(rt)) {
  struct vr4300_rfex_latch *rfex_latch = &vr4300->pipeline.rfex_latch;
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  uint32_t iw = rfex_latch->iw;
  uint64_t imm = (int16_t) iw << 16;
  unsigned dest = GET_RT(iw);

  exdc_latch->result = imm;
  exdc_latch->dest = dest;
}

//
// MTC0
// TODO/FIXME: Combine with MTC{1,2}?
//
void VR4300_MTCx(struct vr4300 *vr4300, uint64_t unused(rs), uint64_t rt) {
  struct vr4300_rfex_latch *rfex_latch = &vr4300->pipeline.rfex_latch;
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  uint32_t iw = rfex_latch->iw;
  unsigned dest = GET_RD(iw) + 32;

  // TODO/FIXME: Sign extend, or...?
  // Would make sense for EPC, etc.
  exdc_latch->result = (int32_t) rt;
  exdc_latch->dest = dest;
}

//
// SLL
//
void VR4300_SLL(struct vr4300 *vr4300, uint64_t unused(rs), uint64_t rt) {
  struct vr4300_rfex_latch *rfex_latch = &vr4300->pipeline.rfex_latch;
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  uint32_t iw = rfex_latch->iw;
  unsigned dest = GET_RD(iw);
  unsigned sa = iw >> 6 & 0x1F;

  exdc_latch->result = (int32_t) (rt << sa);
  exdc_latch->dest = dest;
}

//
// SRL
//
void VR4300_SRL(struct vr4300 *vr4300, uint64_t unused(rs), uint64_t rt) {
  struct vr4300_rfex_latch *rfex_latch = &vr4300->pipeline.rfex_latch;
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  uint32_t iw = rfex_latch->iw;
  unsigned dest = GET_RD(iw);
  unsigned sa = iw >> 6 & 0x1F;

  exdc_latch->result = (int32_t) (rt >> sa);
  exdc_latch->dest = dest;
}

//
// SB
// SH
// SW
//
// TODO/FIXME: Check for unaligned addresses.
//
void VR4300_STORE(struct vr4300 *vr4300, uint64_t rs, uint64_t rt) {
  struct vr4300_rfex_latch *rfex_latch = &vr4300->pipeline.rfex_latch;
  struct vr4300_exdc_latch *exdc_latch = &vr4300->pipeline.exdc_latch;

  uint32_t iw = rfex_latch->iw;
  unsigned request_size = (iw >> 26 & 0x3) + 1;
  uint32_t mask = (~0U >> (4 - request_size));

  exdc_latch->request.address = rs + (int16_t) iw;
  exdc_latch->request.dqm = mask << (iw & 0x3);
  exdc_latch->request.type = VR4300_BUS_REQUEST_WRITE;
  exdc_latch->request.size = request_size;
  exdc_latch->request.word = rt & mask;
}

// Function lookup table.
cen64_align(const vr4300_function
  vr4300_function_table[NUM_VR4300_OPCODES], CACHE_LINE_SIZE) = {
#define X(op) op,
#include "vr4300/opcodes.md"
#undef X
};
