/*=============================================================================
| Copyright 2012 Matthew D. Steele <mdsteele@alum.mit.edu>                    |
|                                                                             |
| This file is part of Azimuth.                                               |
|                                                                             |
| Azimuth is free software: you can redistribute it and/or modify it under    |
| the terms of the GNU General Public License as published by the Free        |
| Software Foundation, either version 3 of the License, or (at your option)   |
| any later version.                                                          |
|                                                                             |
| Azimuth is distributed in the hope that it will be useful, but WITHOUT      |
| ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       |
| FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   |
| more details.                                                               |
|                                                                             |
| You should have received a copy of the GNU General Public License along     |
| with Azimuth.  If not, see <http://www.gnu.org/licenses/>.                  |
=============================================================================*/

#include <string.h>

#include "azimuth/state/script.h"
#include "azimuth/util/misc.h"
#include "test/test.h"

/*===========================================================================*/

static const char *script_string = "push-23.5,beqz/@,nop,bnez/A,halt,A#push0;";

void test_script_print(void) {
  az_instruction_t instructions[] = {
    { .opcode = AZ_OP_PUSH, .immediate = -23.5 },
    { .opcode = AZ_OP_BEQZ, .immediate = 5 },
    { .opcode = AZ_OP_NOP, .immediate = 1 },
    { .opcode = AZ_OP_BNEZ, .immediate = 2 },
    { .opcode = AZ_OP_HALT },
    { .opcode = AZ_OP_PUSH }
  };
  az_script_t script = { .num_instructions = AZ_ARRAY_SIZE(instructions),
                         .instructions = instructions };
  char buffer[100];
  ASSERT_TRUE(az_sprint_script(&script, buffer, sizeof(buffer)));
  EXPECT_STRING_EQ(script_string, buffer);
}

void test_script_scan(void) {
  az_script_t *script = az_sscan_script(script_string, strlen(script_string));
  ASSERT_TRUE(script != NULL);
  EXPECT_INT_EQ(6, script->num_instructions);
  if (script->num_instructions >= 1) {
    EXPECT_INT_EQ(AZ_OP_PUSH, script->instructions[0].opcode);
    EXPECT_APPROX(-23.5, script->instructions[0].immediate);
  }
  if (script->num_instructions >= 2) {
    EXPECT_INT_EQ(AZ_OP_BEQZ, script->instructions[1].opcode);
    EXPECT_APPROX(5, script->instructions[1].immediate);
  }
  if (script->num_instructions >= 3) {
    EXPECT_INT_EQ(AZ_OP_NOP, script->instructions[2].opcode);
    EXPECT_APPROX(0, script->instructions[2].immediate);
  }
  if (script->num_instructions >= 4) {
    EXPECT_INT_EQ(AZ_OP_BNEZ, script->instructions[3].opcode);
    EXPECT_APPROX(2, script->instructions[3].immediate);
  }
  if (script->num_instructions >= 5) {
    EXPECT_INT_EQ(AZ_OP_HALT, script->instructions[4].opcode);
    EXPECT_APPROX(0, script->instructions[4].immediate);
  }
  if (script->num_instructions >= 6) {
    EXPECT_INT_EQ(AZ_OP_PUSH, script->instructions[5].opcode);
    EXPECT_APPROX(0.0, script->instructions[5].immediate);
  }
  az_free_script(script);
}

void test_script_clone(void) {
  EXPECT_TRUE(az_clone_script(NULL) == NULL);
  az_script_t *script1 = az_sscan_script(script_string, strlen(script_string));
  ASSERT_TRUE(script1 != NULL);
  EXPECT_INT_EQ(6, script1->num_instructions);
  if (script1->num_instructions >= 2) {
    EXPECT_INT_EQ(AZ_OP_BEQZ, script1->instructions[1].opcode);
  }
  az_script_t *script2 = az_clone_script(script1);
  az_free_script(script1);
  script1 = NULL;
  ASSERT_TRUE(script2 != NULL);
  EXPECT_INT_EQ(6, script2->num_instructions);
  if (script2->num_instructions >= 2) {
    EXPECT_INT_EQ(AZ_OP_BEQZ, script2->instructions[1].opcode);
  }
  az_free_script(script2);
}

/*===========================================================================*/
