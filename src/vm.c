#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "vm.h"

#define vm_op(op) label_##op:
#define vm_go()   pointer += command.shift; goto *OP_LOOKUP[command.op]
#define vm_next() command = *(++commands); vm_go()

void run(VmCommand *commands) {
    static const void *OP_LOOKUP[8] = {
        &&label_OP_SET,
        &&label_OP_ADD,
        &&label_OP_MUL,
        &&label_OP_JRZ,
        &&label_OP_JRNZ,
        &&label_OP_PUTC,
        &&label_OP_GETC,
        &&label_OP_END
    };

    uint16_t anchor = 0;
    uint16_t pointer = 0;
    uint8_t tape[UINT16_MAX] = { 0 };

    VmCommand command = *commands;

    vm_go();

    vm_op(OP_SET) {
        tape[pointer] = command.value;
        vm_next();
    }

    vm_op(OP_ADD) {
        tape[pointer] += command.value;
        vm_next();
    }

    vm_op(OP_MUL) {
        tape[pointer] += tape[anchor] * command.value;
        vm_next();
    }

    vm_op(OP_JRZ) {
        anchor = pointer;
        if(tape[pointer] == 0) {
            commands += command.jump;
        }
        vm_next();
    }

    vm_op(OP_JRNZ) {
        if(tape[pointer] != 0) {
            commands += command.jump;
        }
        vm_next();
    }

    vm_op(OP_PUTC) {
        write(STDOUT_FILENO, &tape[pointer], 1);
        vm_next();
    }

    vm_op(OP_GETC) {
        // NB: EOF leaves cell unmodified
        read(STDIN_FILENO, &tape[pointer], 1);
        vm_next();
    }

    vm_op(OP_END) {
        return;
    }
}
