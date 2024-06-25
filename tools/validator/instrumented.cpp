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
#include <dlfcn.h>
#include <errno.h>
#include <inttypes.h>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "instrumented.h"
#include "pipes.h"

#include <QBDI.h>
#include "Utility/LogSys.h"

int SAVED_ERRNO = 0;

struct Pipes {
  FILE *ctrlPipe;
  FILE *dataPipe;
};

struct CBData {
  Pipes p;
  unsigned countIgnoreInst;
};

static QBDI::VMAction step(QBDI::VMInstanceRef vm, QBDI::GPRState *gprState,
                           QBDI::FPRState *fprState, void *data) {
  COMMAND cmd;
  SAVED_ERRNO = errno;
  CBData *cbdata = (CBData *)data;

  const QBDI::InstAnalysis *instAnalysis = vm->getInstAnalysis(
      QBDI::ANALYSIS_INSTRUCTION | QBDI::ANALYSIS_DISASSEMBLY);
  // Write a new instruction event
  if (writeInstructionEvent(instAnalysis->address, instAnalysis->mnemonic,
                            instAnalysis->disassembly, gprState, fprState,
                            cbdata->countIgnoreInst != 0,
                            cbdata->p.dataPipe) != 1) {
    // DATA pipe failure, we exit
    QBDI_ERROR("Lost the data pipe, exiting!");
    return QBDI::VMAction::STOP;
  }
  if (cbdata->countIgnoreInst > 0) {
    cbdata->countIgnoreInst -= 1;
  }
#if defined(QBDI_ARCH_ARM)
  if (strcmp(instAnalysis->mnemonic, "t2IT") == 0) {
    instAnalysis = vm->getInstAnalysis(QBDI::ANALYSIS_INSTRUCTION |
                                       QBDI::ANALYSIS_DISASSEMBLY |
                                       QBDI::ANALYSIS_OPERANDS);
    if (instAnalysis->numOperands < 2 or
        instAnalysis->operands[1].type != QBDI::OperandType::OPERAND_IMM) {

      QBDI_ERROR("Fail to parse IT instructions");
      return QBDI::VMAction::STOP;
    }
    QBDI::rword itType = instAnalysis->operands[1].value;
    if ((itType & 1) != 0) {
      cbdata->countIgnoreInst += 4;
    } else if ((itType & 2) != 0) {
      cbdata->countIgnoreInst += 3;
    } else if ((itType & 4) != 0) {
      cbdata->countIgnoreInst += 2;
    } else if ((itType & 8) != 0) {
      cbdata->countIgnoreInst += 1;
    } else {
      QBDI_ERROR("Invalid IT instruction");
      return QBDI::VMAction::STOP;
    }
  }
#endif
  // Read next command
  if (readCommand(&cmd, cbdata->p.ctrlPipe) != 1) {
    // CTRL pipe failure, we exit
    QBDI_ERROR("Lost the control pipe, exiting!");
    return QBDI::VMAction::STOP;
  }

  errno = SAVED_ERRNO;
  if (cmd == COMMAND::CONTINUE) {
    // Signaling the VM to continue the execution
    return QBDI::VMAction::CONTINUE;
  } else if (cmd == COMMAND::STOP) {
    // Signaling the VM to stop the execution
    return QBDI::VMAction::STOP;
  }
  // FATAL
  QBDI_ERROR("Did not recognize command from the control pipe, exiting!");
  return QBDI::VMAction::STOP;
}

static QBDI::VMAction verifyMemoryAccess(QBDI::VMInstanceRef vm,
                                         QBDI::GPRState *gprState,
                                         QBDI::FPRState *fprState, void *data) {
  SAVED_ERRNO = errno;
  CBData *cbdata = (CBData *)data;

  const QBDI::InstAnalysis *instAnalysis =
      vm->getInstAnalysis(QBDI::ANALYSIS_INSTRUCTION);

  bool mayRead = instAnalysis->mayLoad_LLVM;
  bool mayWrite = instAnalysis->mayStore_LLVM;

  bool doRead = false;
  bool doWrite = false;
  std::vector<QBDI::MemoryAccess> accesses = vm->getInstMemoryAccess();
  for (auto &m : accesses) {
    if ((m.type & QBDI::MEMORY_READ) != 0) {
      doRead = true;
    }
    if ((m.type & QBDI::MEMORY_WRITE) != 0) {
      doWrite = true;
    }
  }

  // llvm API mayRead and mayWrite are incomplete.
  bool bypassRead = false;
  bool bypassWrite = false;

  if (doRead and !mayRead) {
#if defined(QBDI_ARCH_X86_64) || defined(QBDI_ARCH_X86)
    // all return instructions read the return address.
    bypassRead |= instAnalysis->isReturn;
    const std::set<std::string> shouldReadInsts{
        "ARPL16mr", "BOUNDS16rm", "BOUNDS32rm",    "CMPSB",   "CMPSL",
        "CMPSQ",    "CMPSW",      "LODSB",         "LODSL",   "LODSQ",
        "LODSW",    "LRETI32",    "LRETI64",       "LRETI16", "LRET32",
        "LRET64",   "LRET16",     "MOVDIR64B16",   "MOVSB",   "MOVSL",
        "MOVSQ",    "MOVSW",      "OR32mi8Locked", "RCL16m1", "RCL16mCL",
        "RCL16mi",  "RCL32m1",    "RCL32mCL",      "RCL32mi", "RCL64m1",
        "RCL64mCL", "RCL64mi",    "RCL8m1",        "RCL8mCL", "RCL8mi",
        "RCR16m1",  "RCR16mCL",   "RCR16mi",       "RCR32m1", "RCR32mCL",
        "RCR32mi",  "RCR64m1",    "RCR64mCL",      "RCR64mi", "RCR8m1",
        "RCR8mCL",  "RCR8mi",     "RETI32",        "RETI64",  "RETI16",
        "RET32",    "RET64",      "RET16",         "SCASB",   "SCASL",
        "SCASQ",    "SCASW",
    };
    bypassRead |= (shouldReadInsts.count(instAnalysis->mnemonic) == 1);
#elif defined(QBDI_ARCH_AARCH64)
    const std::set<std::string> shouldReadInsts{
        "LD64B",
    };
    bypassRead |= (shouldReadInsts.count(instAnalysis->mnemonic) == 1);
#endif
  } else if (!doRead and mayRead) {
#if defined(QBDI_ARCH_X86_64) || defined(QBDI_ARCH_X86)
    const std::set<std::string> noReadInsts{
        "CLDEMOTE",    "CLFLUSH",      "CLFLUSHOPT", "CLWB",
        "FEMMS",       "FXSAVE",       "FXSAVE64",   "INT",
        "INT3",        "LFENCE",       "LOADIWKEY",  "MFENCE",
        "MMX_EMMS",    "MMX_MOVNTQmr", "MOVDIRI32",  "MOVDIRI64",
        "MWAITrr",     "PAUSE",        "PREFETCH",   "PREFETCHNTA",
        "PREFETCHT0",  "PREFETCHT1",   "PREFETCHT2", "PREFETCHW",
        "PREFETCHWT1", "PTWRITE64r",   "PTWRITEr",   "RDFSBASE",
        "RDFSBASE64",  "RDGSBASE",     "RDGSBASE64", "RDPID32",
        "SERIALIZE",   "SFENCE",       "STTILECFG",  "TILERELEASE",
        "TRAP",        "UMONITOR16",   "UMONITOR32", "UMONITOR64",
        "WRFSBASE",    "WRFSBASE64",   "WRGSBASE",   "WRGSBASE64",
        "XSETBV",
    };
    bypassRead |= (noReadInsts.count(instAnalysis->mnemonic) == 1);
#elif defined(QBDI_ARCH_AARCH64)
    const std::set<std::string> noReadInsts{
        "CLREX",  "STLXPW", "STLXPX", "STLXRB", "STLXRH", "STLXRW",
        "STLXRX", "STXPW",  "STXPX",  "STXRB",  "STXRH",  "STXRW",
        "STXRX",  "DMB",    "DSB",    "HINT",   "ISB",
    };
    bypassRead |= (noReadInsts.count(instAnalysis->mnemonic) == 1);
#endif
  }

  if (doWrite and !mayWrite) {
#if defined(QBDI_ARCH_X86_64) || defined(QBDI_ARCH_X86)
    // all call instructions write the return address.
    bypassWrite |= instAnalysis->isCall;
    const std::set<std::string> shouldWriteInsts{
        "CALL16m",     "CALL16m_NT",    "CALL16r",       "CALL16r_NT",
        "CALL32m",     "CALL32m_NT",    "CALL32r",       "CALL32r_NT",
        "CALL64m",     "CALL64m_NT",    "CALL64pcrel32", "CALL64r",
        "CALL64r_NT",  "CALLpcrel16",   "CALLpcrel32",   "ENTER",
        "MOVDIR64B16", "MOVSB",         "MOVSL",         "MOVSQ",
        "MOVSW",       "OR32mi8Locked", "STOSB",         "STOSL",
        "STOSQ",       "STOSW",
    };
    bypassWrite |= (shouldWriteInsts.count(instAnalysis->mnemonic) == 1);
#elif defined(QBDI_ARCH_AARCH64)
    const std::set<std::string> shouldWriteInsts{
        "ST64B",
        "ST64BV",
        "ST64BV0",
    };
    bypassWrite |= (shouldWriteInsts.count(instAnalysis->mnemonic) == 1);
#endif
  } else if (!doWrite and mayWrite) {
#if defined(QBDI_ARCH_X86_64) || defined(QBDI_ARCH_X86)
    const std::set<std::string> noWriteInsts{
        "CLDEMOTE",    "CLFLUSH",    "CLFLUSHOPT", "CLWB",        "FEMMS",
        "FXRSTOR",     "FXRSTOR64",  "INT",        "INT3",        "LDMXCSR",
        "LDTILECFG",   "LFENCE",     "LOADIWKEY",  "MFENCE",      "MMX_EMMS",
        "MWAITrr",     "PAUSE",      "PREFETCH",   "PREFETCHNTA", "PREFETCHT0",
        "PREFETCHT1",  "PREFETCHT2", "PREFETCHW",  "PREFETCHWT1", "PTWRITE64m",
        "PTWRITE64r",  "PTWRITEm",   "PTWRITEr",   "RDFSBASE",    "RDFSBASE64",
        "RDGSBASE",    "RDGSBASE64", "RDPID32",    "SERIALIZE",   "SFENCE",
        "TILERELEASE", "UMONITOR16", "UMONITOR32", "UMONITOR64",  "VLDMXCSR",
        "WRFSBASE",    "WRFSBASE64", "WRGSBASE",   "WRGSBASE64",  "XRSTOR",
        "XRSTOR64",    "XRSTORS",    "XRSTORS64",  "XSETBV",
    };
    bypassWrite |= (noWriteInsts.count(instAnalysis->mnemonic) == 1);
#elif defined(QBDI_ARCH_AARCH64)
    const std::set<std::string> noWriteInsts{
        "LDXRB",  "LDXRH",  "LDXRW", "LDXRX", "LDAXRB", "LDAXRH",
        "LDAXRW", "LDAXRX", "LDXPW", "LDXPX", "LDAXPW", "LDAXPX",
        "CLREX",  "DMB",    "DSB",   "HINT",  "ISB",
    };
    bypassWrite |= (noWriteInsts.count(instAnalysis->mnemonic) == 1);
#endif
  }

  if ((doRead == mayRead || bypassRead) &&
      (doWrite == mayWrite || bypassWrite)) {
    errno = SAVED_ERRNO;
    return QBDI::VMAction::CONTINUE;
  }

  // Write a new instruction event
  if (writeMismatchMemAccessEvent(
          instAnalysis->address, doRead, instAnalysis->mayLoad, doWrite,
          instAnalysis->mayStore, accesses, cbdata->p.dataPipe) != 1) {
    // DATA pipe failure, we exit
    QBDI_ERROR("Lost the data pipe, exiting!");
    return QBDI::VMAction::STOP;
  }
  errno = SAVED_ERRNO;
  // Continue the execution
  return QBDI::VMAction::CONTINUE;
}

#if defined(QBDI_ARCH_X86_64) || defined(QBDI_ARCH_X86)
static QBDI::VMAction logSyscall(QBDI::VMInstanceRef vm,
                                 QBDI::GPRState *gprState,
                                 QBDI::FPRState *fprState, void *data) {
  CBData *cbdata = (CBData *)data;
  // We don't have the address, it just need to be different from 0
  writeExecTransferEvent(1, cbdata->p.dataPipe);
  return QBDI::VMAction::CONTINUE;
}
#endif

static QBDI::VMAction logTransfer(QBDI::VMInstanceRef vm,
                                  const QBDI::VMState *state,
                                  QBDI::GPRState *gprState,
                                  QBDI::FPRState *fprState, void *data) {
  CBData *cbdata = (CBData *)data;
  writeExecTransferEvent(state->basicBlockStart, cbdata->p.dataPipe);
  return QBDI::VMAction::CONTINUE;
}

static QBDI::VMAction saveErrno(QBDI::VMInstanceRef vm,
                                const QBDI::VMState *state,
                                QBDI::GPRState *gprState,
                                QBDI::FPRState *fprState, void *data) {
  SAVED_ERRNO = errno;
  return QBDI::VMAction::CONTINUE;
}

static QBDI::VMAction restoreErrno(QBDI::VMInstanceRef vm,
                                   const QBDI::VMState *state,
                                   QBDI::GPRState *gprState,
                                   QBDI::FPRState *fprState, void *data) {
  errno = SAVED_ERRNO;
  return QBDI::VMAction::CONTINUE;
}

QBDI::VM *VM;
CBData CBDATA = {{NULL, NULL}, 0};

void cleanup_instrumentation() {
  static bool cleaned_up = false;
  if (cleaned_up == false) {
    writeEvent(EVENT::EXIT, CBDATA.p.dataPipe);
    fclose(CBDATA.p.ctrlPipe);
    fclose(CBDATA.p.dataPipe);
    delete VM;
    cleaned_up = true;
  }
}

void start_instrumented(QBDI::VM *vm, QBDI::rword start, QBDI::rword stop,
                        int ctrlfd, int datafd) {

  VM = vm;
  if (getenv("QBDI_DEBUG") != NULL) {
    QBDI::setLogPriority(QBDI::LogPriority::DEBUG);
  } else {
    QBDI::setLogPriority(QBDI::LogPriority::ERROR);
  }
  // Opening communication FIFO
  CBDATA.p.ctrlPipe = fdopen(ctrlfd, "rb");
  CBDATA.p.dataPipe = fdopen(datafd, "wb");
  if (CBDATA.p.ctrlPipe == nullptr || CBDATA.p.dataPipe == nullptr) {
    QBDI_ERROR("Could not open communication pipes with master, exiting!");
    return;
  }

  vm->addCodeCB(QBDI::PREINST, step, (void *)&CBDATA);
#if defined(QBDI_ARCH_X86_64) || defined(QBDI_ARCH_X86) || \
    defined(QBDI_ARCH_AARCH64)
  // memory Access are not supported for ARM now
  vm->recordMemoryAccess(QBDI::MEMORY_READ_WRITE);
  vm->addCodeCB(QBDI::POSTINST, verifyMemoryAccess, (void *)&CBDATA);
#endif

#if defined(QBDI_ARCH_X86_64) || defined(QBDI_ARCH_X86)
  vm->addMnemonicCB("syscall", QBDI::POSTINST, logSyscall, (void *)&CBDATA);
#endif
  vm->addVMEventCB(QBDI::VMEvent::EXEC_TRANSFER_CALL, logTransfer,
                   (void *)&CBDATA);
  vm->addVMEventCB(QBDI::VMEvent::EXEC_TRANSFER_CALL |
                       QBDI::VMEvent::BASIC_BLOCK_ENTRY,
                   restoreErrno, nullptr);
  vm->addVMEventCB(QBDI::VMEvent::EXEC_TRANSFER_RETURN |
                       QBDI::VMEvent::BASIC_BLOCK_EXIT,
                   saveErrno, nullptr);

  vm->run(start, stop);

  cleanup_instrumentation();
}
