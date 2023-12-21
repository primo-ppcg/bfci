#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "program.h"

#define bytecode_weight(command) (BYTECODE_WEIGHTS[command.op] + (command.shift != 0 ? 5 : 0))

static const size_t BYTECODE_WEIGHTS[] = {
    4,  // OP_SET
    4,  // OP_ADD
    6, // OP_MUL
    11, // OP_JRZ
    10, // OP_JRNZ
    15, // OP_PUTC
    15, // OP_GETC
    9, // OP_END
};

typedef struct {
    size_t length;
    size_t capacity;
    uint8_t *ops;
} ByteCode;

ByteCode compile(Program program);
void bytecode_deinit(ByteCode bytecode);
bool write_executable(ByteCode bytecode, char *path);
