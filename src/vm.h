#pragma once

#include <stdint.h>

#define OP_SET  0x00
#define OP_ADD  0x01
#define OP_MUL  0x02
#define OP_JRZ  0x03
#define OP_JRNZ 0x04
#define OP_PUTC 0x05
#define OP_GETC 0x06
#define OP_END  0x07

typedef struct {
    uint8_t op;
    uint8_t value;
    uint16_t shift;
    int32_t jump;
} VmCommand;

void vm_run(VmCommand *commands);
