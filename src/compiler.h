#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "program.h"

static const size_t BYTECODE_WEIGHTS[] = {
    9,  // OP_SET
    9,  // OP_ADD
    13, // OP_MUL
    17, // OP_JRZ
    15, // OP_JRNZ
    20, // OP_PUTC
    20, // OP_GETC
    14, // OP_END
};

typedef struct {
    size_t length;
    size_t capacity;
    uint8_t *ops;
} ByteCode;

ByteCode compile(Program program);
void bytecode_deinit(ByteCode bytecode);
bool write_executable(ByteCode bytecode, char *path);
