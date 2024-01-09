#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "program.h"

#define bytecode_weight(command) (BYTECODE_WEIGHTS[command.op] + (command.shift != 0 ? 5 : 0))

static const size_t BYTECODE_WEIGHTS[] = {
    10, // OP_JRZ
    9,  // OP_JRNZ
    3,  // OP_ADD
    3,  // OP_SET
    2,  // OP_CPY
    5,  // OP_MUL
    6,  // OP_PUTC
    6,  // OP_GETC
    4,  // OP_END
};

typedef struct {
    size_t length;
    size_t capacity;
    uint8_t *ops;
} ByteCode;

ByteCode compile(Program program);
void bytecode_deinit(ByteCode bytecode);
bool write_executable(ByteCode bytecode, char *path);
