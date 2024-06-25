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
#include "Patch/Instr_Test.h"

TEST_CASE_METHOD(Instr_Test, "Instr_Test-STLDMIA") {
  INFO("TEST_SEED=" << seed_random());
  uint64_t count1 = 0;
  uint64_t count2 = 0;

  QBDI::Context inputState;
  initContext(inputState);

  vm.deleteAllInstrumentations();
  vm.addCodeCB(QBDI::PREINST, increment, (void *)&count1);
  vm.addCodeCB(QBDI::POSTINST, increment, (void *)&count2);

  comparedExec(STLDMIA_s, inputState, 4096);

  REQUIRE((uint64_t)0 < count1);
  REQUIRE(count1 == count2);

  INFO("Took " << count1 << " instructions\n");
}

TEST_CASE_METHOD(Instr_Test, "Instr_Test-STLDMIB") {
  INFO("TEST_SEED=" << seed_random());
  uint64_t count1 = 0;
  uint64_t count2 = 0;

  QBDI::Context inputState;
  initContext(inputState);

  vm.deleteAllInstrumentations();
  vm.addCodeCB(QBDI::PREINST, increment, (void *)&count1);
  vm.addCodeCB(QBDI::POSTINST, increment, (void *)&count2);

  comparedExec(STLDMIB_s, inputState, 4096);

  REQUIRE((uint64_t)0 < count1);
  REQUIRE(count1 == count2);

  INFO("Took " << count1 << " instructions\n");
}

TEST_CASE_METHOD(Instr_Test, "Instr_Test-STLDMDA") {
  INFO("TEST_SEED=" << seed_random());
  uint64_t count1 = 0;
  uint64_t count2 = 0;

  QBDI::Context inputState;
  initContext(inputState);

  vm.deleteAllInstrumentations();
  vm.addCodeCB(QBDI::PREINST, increment, (void *)&count1);
  vm.addCodeCB(QBDI::POSTINST, increment, (void *)&count2);

  comparedExec(STLDMDA_s, inputState, 4096);

  REQUIRE((uint64_t)0 < count1);
  REQUIRE(count1 == count2);

  INFO("Took " << count1 << " instructions\n");
}

TEST_CASE_METHOD(Instr_Test, "Instr_Test-STLDMDB") {
  INFO("TEST_SEED=" << seed_random());
  uint64_t count1 = 0;
  uint64_t count2 = 0;

  QBDI::Context inputState;
  initContext(inputState);

  vm.deleteAllInstrumentations();
  vm.addCodeCB(QBDI::PREINST, increment, (void *)&count1);
  vm.addCodeCB(QBDI::POSTINST, increment, (void *)&count2);

  comparedExec(STLDMDB_s, inputState, 4096);

  REQUIRE((uint64_t)0 < count1);
  REQUIRE(count1 == count2);

  INFO("Took " << count1 << " instructions\n");
}

TEST_CASE_METHOD(Instr_Test, "Instr_Test-STMDB_LDMIA_post") {
  INFO("TEST_SEED=" << seed_random());
  uint64_t count1 = 0;
  uint64_t count2 = 0;

  QBDI::Context inputState;
  initContext(inputState);

  vm.deleteAllInstrumentations();
  vm.addCodeCB(QBDI::PREINST, increment, (void *)&count1);
  vm.addCodeCB(QBDI::POSTINST, increment, (void *)&count2);

  comparedExec(STMDB_LDMIA_post_s, inputState, 4096);

  REQUIRE((uint64_t)0 < count1);
  REQUIRE(count1 == count2);

  INFO("Took " << count1 << " instructions\n");
}

TEST_CASE_METHOD(Instr_Test, "Instr_Test-STMDA_LDMIB_post") {
  INFO("TEST_SEED=" << seed_random());
  uint64_t count1 = 0;
  uint64_t count2 = 0;

  QBDI::Context inputState;
  initContext(inputState);

  vm.deleteAllInstrumentations();
  vm.addCodeCB(QBDI::PREINST, increment, (void *)&count1);
  vm.addCodeCB(QBDI::POSTINST, increment, (void *)&count2);

  comparedExec(STMDA_LDMIB_post_s, inputState, 4096);

  REQUIRE((uint64_t)0 < count1);
  REQUIRE(count1 == count2);

  INFO("Took " << count1 << " instructions\n");
}

TEST_CASE_METHOD(Instr_Test, "Instr_Test-STMIB_LDMDA_post") {
  INFO("TEST_SEED=" << seed_random());
  uint64_t count1 = 0;
  uint64_t count2 = 0;

  QBDI::Context inputState;
  initContext(inputState);

  vm.deleteAllInstrumentations();
  vm.addCodeCB(QBDI::PREINST, increment, (void *)&count1);
  vm.addCodeCB(QBDI::POSTINST, increment, (void *)&count2);

  comparedExec(STMIB_LDMDA_post_s, inputState, 4096);

  REQUIRE((uint64_t)0 < count1);
  REQUIRE(count1 == count2);

  INFO("Took " << count1 << " instructions\n");
}

TEST_CASE_METHOD(Instr_Test, "Instr_Test-STMIA_LDMDB_post") {
  INFO("TEST_SEED=" << seed_random());
  uint64_t count1 = 0;
  uint64_t count2 = 0;

  QBDI::Context inputState;
  initContext(inputState);

  vm.deleteAllInstrumentations();
  vm.addCodeCB(QBDI::PREINST, increment, (void *)&count1);
  vm.addCodeCB(QBDI::POSTINST, increment, (void *)&count2);

  comparedExec(STMIA_LDMDB_post_s, inputState, 4096);

  REQUIRE((uint64_t)0 < count1);
  REQUIRE(count1 == count2);

  INFO("Took " << count1 << " instructions\n");
}

TEST_CASE_METHOD(Instr_Test, "Instr_Test-LDREXTest") {
  INFO("TEST_SEED=" << seed_random());
  uint64_t count1 = 0;
  uint64_t count2 = 0;
  uint8_t buffer[4096] = {0};

  QBDI::Context inputState;
  initContext(inputState);
  inputState.gprState.r11 = (QBDI::rword)&buffer;

  vm.deleteAllInstrumentations();
  vm.addCodeCB(QBDI::PREINST, increment, (void *)&count1);
  vm.addCodeCB(QBDI::POSTINST, increment, (void *)&count2);

  comparedExec(LDREXTest_s, inputState, 4096);

  REQUIRE((uint64_t)0 < count1);
  REQUIRE(count1 == count2);

  INFO("Took " << count1 << " instructions\n");
}
