/*
 * This file is part of QBDI.
 *
 * Copyright 2017 - 2024 Quarkslab
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <catch2/catch.hpp>
#include <set>
#include <stdio.h>

#include "X86InstrInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrDesc.h"

#include "Patch/InstInfo.h"
#include "Patch/MemoryAccessTable.h"

namespace {

using namespace llvm::X86;

const std::set<unsigned> unsupportedInst{
    // clang-format off
    // codeGenOnly
    // alias for other instruction, will never be dissassemble by LLVM
    LOCK_BTC_RM16rm,
    LOCK_BTC_RM32rm,
    LOCK_BTC_RM64rm,
    LOCK_BTR_RM16rm,
    LOCK_BTR_RM32rm,
    LOCK_BTR_RM64rm,
    LOCK_BTS_RM16rm,
    LOCK_BTS_RM32rm,
    LOCK_BTS_RM64rm,
    LXADD16,
    LXADD32,
    LXADD64,
    LXADD8,
    MAXCPDrm,
    MAXCPSrm,
    MAXCSDrm,
    MAXCSSrm,
    MINCPDrm,
    MINCPSrm,
    MINCSDrm,
    MINCSSrm,
    MMX_MOVD64from64mr,
    MMX_MOVD64to64rm,
    MOV64toPQIrm,
    MOVPQIto64mr,
    MOVSX16rm16,
    MOVSX16rm32,
    MOVSX32rm32,
    MOVZX16rm16,
    RDSSPD,
    RDSSPQ,
    REP_MOVSB_32,
    REP_MOVSB_64,
    REP_MOVSD_32,
    REP_MOVSD_64,
    REP_MOVSQ_32,
    REP_MOVSQ_64,
    REP_MOVSW_32,
    REP_MOVSW_64,
    REP_STOSB_32,
    REP_STOSB_64,
    REP_STOSD_32,
    REP_STOSD_64,
    REP_STOSQ_32,
    REP_STOSQ_64,
    REP_STOSW_32,
    REP_STOSW_64,
    SBB8mi8,
    VMAXCPDYrm,
    VMAXCPDrm,
    VMAXCPSYrm,
    VMAXCPSrm,
    VMAXCSDrm,
    VMAXCSSrm,
    VMINCPDYrm,
    VMINCPDrm,
    VMINCPSYrm,
    VMINCPSrm,
    VMINCSDrm,
    VMINCSSrm,
    VMOV64toPQIrm,
    VMOVPQIto64mr,
    // priviledged instruction
    INVPCID32,
    RDMSRLIST,
    VMREAD32mr,
    VMREAD64mr,
    VMWRITE32rm,
    VMWRITE64rm,
    WBINVD,
    WBNOINVD,
    WRMSRLIST,
    // CET feature (shadow stack)
    CLRSSBSY,
    INCSSPD,
    INCSSPQ,
    RSTORSSP,
    SAVEPREVSSP,
    SETSSBSY,
    WRSSD,
    WRSSQ,
    WRUSSD,
    WRUSSQ,
    // RTM feature unsupported
    XABORT,
    XBEGIN,
    XEND,
    // AVX512 unsupported
    KMOVBkm,
    KMOVBmk,
    KMOVDkm,
    KMOVDmk,
    KMOVQkm,
    KMOVQmk,
    KMOVWkm,
    KMOVWmk,
    // complex & conditionnal memory access (SIB access)
    TILELOADD,
    TILELOADDT1,
    TILESTORED,
    VGATHERDPDYrm,
    VGATHERDPDrm,
    VGATHERDPSYrm,
    VGATHERDPSrm,
    VGATHERQPDYrm,
    VGATHERQPDrm,
    VGATHERQPSYrm,
    VGATHERQPSrm,
    VPGATHERDDYrm,
    VPGATHERDDrm,
    VPGATHERDQYrm,
    VPGATHERDQrm,
    VPGATHERQDYrm,
    VPGATHERQDrm,
    VPGATHERQQYrm,
    VPGATHERQQrm,
    // farcall
    FARCALL16m,
    FARCALL32m,
    FARCALL64m,
    FARJMP16m,
    FARJMP32m,
    FARJMP64m,
    // UD1 (trap instruction)
    UD1Lm,
    UD1Lr,
    UD1Qm,
    UD1Qr,
    UD1Wm,
    UD1Wr,
    // TSXLDTRK
    XRESLDTRK,
    XSUSLDTRK,
    // UINTR
    CLUI,
    SENDUIPI,
    STUI,
    UIRET,
    // clang-format on
};

// instruction that reads memory/stack but without mayLoad
const std::set<unsigned> fixupRead{
    // clang-format off
    AADD32mr,
    AADD64mr,
    AAND32mr,
    AAND64mr,
    AOR32mr,
    AOR64mr,
    AXOR32mr,
    AXOR64mr,
    ARPL16mr,
    BOUNDS16rm,
    BOUNDS32rm,
    CMPSB,
    CMPSL,
    CMPSQ,
    CMPSW,
    LODSB,
    LODSL,
    LODSQ,
    LODSW,
    LRETI32,
    LRETI64,
    LRETI16,
    LRET32,
    LRET64,
    LRET16,
    MOVDIR64B16,
    MOVSB,
    MOVSL,
    MOVSQ,
    MOVSW,
    OR32mi8Locked,
    RETI32,
    RETI64,
    RETI16,
    RET32,
    RET64,
    RET16,
    SCASB,
    SCASL,
    SCASQ,
    SCASW,
    // clang-format on
};
// instruction that writes memory/stack but without mayStore
const std::set<unsigned> fixupWrite{
    // clang-format off
    CALL16m,
    CALL16m_NT,
    CALL16r,
    CALL16r_NT,
    CALL32m,
    CALL32m_NT,
    CALL32r,
    CALL32r_NT,
    CALL64m,
    CALL64m_NT,
    CALL64pcrel32,
    CALL64r,
    CALL64r_NT,
    CALLpcrel16,
    CALLpcrel32,
    ENTER,
    MOVDIR64B16,
    MOVSB,
    MOVSL,
    MOVSQ,
    MOVSW,
    OR32mi8Locked,
    STOSB,
    STOSL,
    STOSQ,
    STOSW,
    // clang-format on
};
// instruction with mayLoad but don't reads memory/stack
const std::set<unsigned> fixupNoRead{
    // clang-format off
    CLDEMOTE,
    CLFLUSH,
    CLFLUSHOPT,
    CLWB,
    FEMMS,
    FXSAVE,
    FXSAVE64,
    INT,
    INT3,
    LFENCE,
    LOADIWKEY,
    MFENCE,
    MMX_EMMS,
    MMX_MOVNTQmr,
    MOVDIRI32,
    MOVDIRI64,
    MWAITrr,
    PAUSE,
    PREFETCH,
    PREFETCHIT0,
    PREFETCHIT1,
    PREFETCHNTA,
    PREFETCHT0,
    PREFETCHT1,
    PREFETCHT2,
    PREFETCHW,
    PREFETCHWT1,
    PTWRITE64r,
    PTWRITEr,
    RDFSBASE,
    RDFSBASE64,
    RDGSBASE,
    RDGSBASE64,
    RDPID32,
    SERIALIZE,
    SFENCE,
    STTILECFG,
    TILERELEASE,
    TRAP,
    UMONITOR16,
    UMONITOR32,
    UMONITOR64,
    WRFSBASE,
    WRFSBASE64,
    WRGSBASE,
    WRGSBASE64,
    XSETBV,
    // clang-format on
};
// instruction with mayStore but don't writes memory/stack
const std::set<unsigned> fixupNoWrite{
    // clang-format off
    CLDEMOTE,
    CLFLUSH,
    CLFLUSHOPT,
    CLWB,
    FEMMS,
    FXRSTOR,
    FXRSTOR64,
    INT,
    INT3,
    LDMXCSR,
    LDTILECFG,
    LFENCE,
    LOADIWKEY,
    MFENCE,
    MMX_EMMS,
    MWAITrr,
    PAUSE,
    PREFETCH,
    PREFETCHIT0,
    PREFETCHIT1,
    PREFETCHNTA,
    PREFETCHT0,
    PREFETCHT1,
    PREFETCHT2,
    PREFETCHW,
    PREFETCHWT1,
    PTWRITE64m,
    PTWRITE64r,
    PTWRITEm,
    PTWRITEr,
    RDFSBASE,
    RDFSBASE64,
    RDGSBASE,
    RDGSBASE64,
    RDPID32,
    SERIALIZE,
    SFENCE,
    TILERELEASE,
    UMONITOR16,
    UMONITOR32,
    UMONITOR64,
    VLDMXCSR,
    WRFSBASE,
    WRFSBASE64,
    WRGSBASE,
    WRGSBASE64,
    XRSTOR,
    XRSTOR64,
    XRSTORS,
    XRSTORS64,
    XSETBV,
    // clang-format on
};

} // namespace

TEST_CASE_METHOD(MemoryAccessTable, "MemoryAccessTable-CrossCheck") {

  const QBDI::LLVMCPU &llvmcpu = getCPU(QBDI::CPUMode::DEFAULT);
  const llvm::MCInstrInfo &MCII = llvmcpu.getMCII();

  for (unsigned opcode = 0; opcode < llvm::X86::INSTRUCTION_LIST_END;
       opcode++) {
    if (unsupportedInst.count(opcode) == 1)
      continue;

    const llvm::MCInstrDesc &desc = MCII.get(opcode);
    const char *mnemonic = MCII.getName(opcode).data();

    // InstInfo_X86_64.cpp only use inst->getOpcode(). The MCInst doesn't need
    // to have his operand
    llvm::MCInst inst;
    inst.setOpcode(opcode);

    bool doRead = (QBDI::getReadSize(inst, llvmcpu) != 0);
    bool doWrite = (QBDI::getWriteSize(inst, llvmcpu) != 0);
    bool mayRead = desc.mayLoad();
    bool mayWrite = desc.mayStore();

    // the opcode is a pseudo instruction used by LLVM internally
    if (desc.isPseudo()) {
      if (doRead or doWrite) {
        WARN("Pseudo instruction " << mnemonic << " in InstInfo");
      }
      continue;
    }

    // some no pseudo instructions are also pseudo ...
    if ((desc.TSFlags & llvm::X86II::FormMask) == llvm::X86II::Pseudo)
      continue;

    // not support AVX512. discard all instruction with the encodage EVEX
    // introduce with AVX512
    if ((desc.TSFlags & llvm::X86II::EncodingMask) == llvm::X86II::EVEX)
      continue;

    // not support XOP. (AMD eXtended Operations)
    if ((desc.TSFlags & llvm::X86II::EncodingMask) == llvm::X86II::XOP)
      continue;

    bool bypassRead = false;
    bool bypassWrite = false;

    // llvm mayLoad and mayStore fixup
    if (fixupRead.count(opcode) == 1) {
      if (doRead && !mayRead)
        bypassRead = true;
      else
        WARN("Unneeded instruction " << mnemonic << " in fixupRead");
    }

    if (fixupNoRead.count(opcode) == 1) {
      if (!doRead && mayRead)
        bypassRead = true;
      else
        WARN("Unneeded instruction " << mnemonic << " in fixupNoRead");
    }

    if (fixupWrite.count(opcode) == 1) {
      if (doWrite && !mayWrite)
        bypassWrite = true;
      else
        WARN("Unneeded instruction " << mnemonic << " in fixupWrite");
    }

    if (fixupNoWrite.count(opcode) == 1) {
      if (!doWrite && mayWrite)
        bypassWrite = true;
      else
        WARN("Unneeded instruction " << mnemonic << " in fixupNoWrite");
    }

    if (!bypassRead && doRead != mayRead) {
      if (doRead && !mayRead) {
        FAIL_CHECK("Unexpected read for " << mnemonic);
      } else if (!doRead && mayRead) {
        FAIL_CHECK("Missing read for "
                   << mnemonic << " type "
                   << (desc.TSFlags & llvm::X86II::FormMask));
      }
    }

    if (!bypassWrite && doWrite != mayWrite) {
      if (doWrite && !mayWrite) {
        FAIL_CHECK("Unexpected write for " << mnemonic);
      } else if (!doWrite && mayWrite) {
        FAIL_CHECK("Missing write for "
                   << mnemonic << " type "
                   << (desc.TSFlags & llvm::X86II::FormMask));
      }
    }
  }
}
