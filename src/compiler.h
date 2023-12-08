#include <stdint.h>
#include <stdlib.h>

#include "program.h"

typedef struct {
    size_t length;
    size_t capacity;
    uint8_t *ops;
} ByteCode;

ByteCode compile(Program program);
void bytecode_deinit();
