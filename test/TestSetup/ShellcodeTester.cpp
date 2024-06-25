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
#include "TestSetup/ShellcodeTester.h"
#include <catch2/catch.hpp>
#include "Utility/LogSys.h"

llvm::sys::MemoryBlock ShellcodeTester::allocateStack(QBDI::rword size) {
  std::error_code ec;
  llvm::sys::MemoryBlock stackBlock;

  stackBlock = llvm::sys::Memory::allocateMappedMemory(
      size, nullptr, PF::MF_READ | PF::MF_WRITE, ec);
  CHECK(0 == ec.value());
  memset(stackBlock.base(), 0, stackBlock.allocatedSize());
  return stackBlock;
}

void ShellcodeTester::freeStack(llvm::sys::MemoryBlock &memoryBlock) {
  CHECK(0 == llvm::sys::Memory::releaseMappedMemory(memoryBlock).value());
}

void ShellcodeTester::comparedExec(const char *source, QBDI::Context &inputCtx,
                                   QBDI::rword stackSize) {
  InMemoryObject object = compileWithContextSwitch(source);

  QBDI::Context realCtx, jitCtx;
  llvm::sys::MemoryBlock realStack = allocateStack(stackSize);
  llvm::sys::MemoryBlock jitStack = allocateStack(stackSize);
  REQUIRE(realStack.allocatedSize() == jitStack.allocatedSize());

  const llvm::ArrayRef<uint8_t> &code = object.getCode();
  llvm::sys::Memory::InvalidateInstructionCache(code.data(), code.size());

  // use the same stack for the 2 executions
  memset(jitStack.base(), 0, jitStack.allocatedSize());
  realCtx = realExec(code, inputCtx, jitStack);
  memcpy(realStack.base(), jitStack.base(), jitStack.allocatedSize());

  memset(jitStack.base(), 0, jitStack.allocatedSize());
  jitCtx = jitExec(code, inputCtx, jitStack);

  for (uint32_t i = 0; i < QBDI::AVAILABLE_GPR; i++) {
    INFO("The offset is " << i);
    CHECK(QBDI_GPR_GET(&realCtx.gprState, i) ==
          QBDI_GPR_GET(&jitCtx.gprState, i));
  }

#if defined(QBDI_ARCH_AARCH64)
  for (uint32_t i = 0; i < QBDI_NUM_FPR; i++) {
    INFO("The offset is " << i);
    CHECK(((__uint128_t *)&realCtx.fprState)[i] ==
          ((__uint128_t *)&jitCtx.fprState)[i]);
  }
#elif !defined(_QBDI_ASAN_ENABLED_) || !defined(QBDI_ARCH_X86_64)
  for (uint32_t i = 0; i < sizeof(QBDI::FPRState); i++) {
    INFO("The offset is " << i);
    CHECK(((char *)&realCtx.fprState)[i] == ((char *)&jitCtx.fprState)[i]);
  }
#endif
  for (uint32_t i = 0; i < realStack.allocatedSize() - sizeof(QBDI::rword);
       i++) {
    INFO("The offset is " << i << " / " << realStack.allocatedSize());
    CHECK(((char *)realStack.base())[i] == ((char *)jitStack.base())[i]);
  }

  inputCtx = jitCtx;

  freeStack(realStack);
  freeStack(jitStack);
}
