#include <stdio.h>
#include <unistd.h>

#include "vm.h"

#define vm_op(op) label_##op:
#define vm_go()   pointer.dx += command.shift; goto *OP_LOOKUP[command.op]
#define vm_next() command = *(++commands); vm_go()

typedef union {
    struct {
        uint8_t dl;
        uint8_t dh;
    };
    uint16_t dx;
    uint32_t edx;
} virtual_register;

void vm_run(VmCommand *commands) {
    static const void *OP_LOOKUP[] = {
        &&label_OP_JRZ,
        &&label_OP_JRNZ,
        &&label_OP_ADD,
        &&label_OP_SET,
        &&label_OP_SHL,
        &&label_OP_SHR,
        &&label_OP_CPY,
        &&label_OP_MUL,
        &&label_OP_PUTC,
        &&label_OP_GETC,
        &&label_OP_END
    };

    virtual_register base = { .edx = 0 };
    virtual_register pointer = { .dx = 0 };
    uint8_t tape[UINT16_MAX] = { 0 };

    VmCommand command = *commands;

    vm_go();

    vm_op(OP_JRZ) {
        base.dl = tape[pointer.dx];
        if(tape[pointer.dx] == 0) {
            commands += command.jump;
        }
        vm_next();
    }

    vm_op(OP_JRNZ) {
        if(tape[pointer.dx] != 0) {
            commands += command.jump - 1;
        }
        vm_next();
    }

    vm_op(OP_ADD) {
        tape[pointer.dx] += command.value;
        vm_next();
    }

    vm_op(OP_SET) {
        tape[pointer.dx] = command.value;
        vm_next();
    }

    vm_op(OP_SHL) {
        base.edx <<= 8;
        base.dl = tape[pointer.dx];
        vm_next();
    }

    vm_op(OP_SHR) {
        base.edx >>= 8;
        vm_next();
    }

    vm_op(OP_CPY) {
        tape[pointer.dx] += base.dl;
        vm_next();
    }

    vm_op(OP_MUL) {
        tape[pointer.dx] += base.dl * command.value;
        vm_next();
    }

    vm_op(OP_PUTC) {
        putchar(tape[pointer.dx]);
        vm_next();
    }

    vm_op(OP_GETC) {
        // NB: EOF leaves cell unmodified
        read(STDIN_FILENO, &tape[pointer.dx], 1);
        vm_next();
    }

    vm_op(OP_END) {
        return;
    }
}
