#include <string.h>

#include "compiler.h"

#define emit(...) { \
    uint8_t opcodes[] = { __VA_ARGS__ }; \
    bytecode_append(&bytecode, sizeof(opcodes), opcodes); \
}

#define imm8(value) value & 0xFF
#define imm16(value) imm8(value), imm8((value) >> 8)
#define imm32(value) imm16(value), imm16((value) >> 16)

void bytecode_deinit(ByteCode bytecode) {
    free(bytecode.ops);
}

static void bytecode_append(ByteCode *bytecode, size_t argc, uint8_t *argv) {
    if(bytecode->length + argc > bytecode->capacity) {
        bytecode->capacity += 256;
        bytecode->ops = realloc(bytecode->ops, bytecode->capacity * sizeof(uint8_t));
    }
    memcpy(&bytecode->ops[bytecode->length], argv, argc * sizeof(uint8_t));
    bytecode->length += argc;
}

ByteCode compile(Program program) {
    ByteCode bytecode = { .length = 0, .capacity = 256, .ops = malloc(256 * sizeof(uint8_t)) };

    emit(
        /* sub $65536, %rsp         */  0x48, 0x81, 0xEC, 0x00, 0x00, 0x01, 0x00,
        /* xor %eax, %eax           */  0x31, 0xC0,
        /* mov %eax, %ebx           */  0x89, 0xC3,
        /* mov $8192, %ecx          */  0xB9, 0x00, 0x20, 0x00, 0x00,
        /* mov %rsp, %rdi           */  0x48, 0x89, 0xE7,
        /* rep stosq                */  0xF3, 0x48, 0xAB
    )

    for(size_t i = 0; i < program.length; i++) {
        VmCommand command = program.commands[i];
        emit(
            /* addw imm16, %bx          */  0x66, 0x81, 0xC3, imm16(command.shift)
        )

        switch(command.op) {
            case OP_SET:
                emit(
                    /* movb imm8, (%rsp,%rbx)   */  0xC6, 0x04, 0x1C, imm8(command.value)
                )
                break;
            case OP_ADD:
                emit(
                    /* addb imm8, (%rsp,%rbx)   */  0x80, 0x04, 0x1C, imm8(command.value)
                )
                break;
            case OP_MUL:
                emit(
                    /* movb imm8, %al           */  0xB0, imm8(command.value),
                    /* mulb (%rsp,%rcx)         */  0xF6, 0x24, 0x0C,
                    /* addb %al, (%rsp,%rbx)    */  0x00, 0x04, 0x1C
                )
                break;
            case OP_JRZ:
                emit(
                    /* mov %ebx, %ecx           */  0x89, 0xD9,
                    /* testb $255, (%rsp,%rbx)  */  0xF6, 0x04, 0x1C, 0xFF,
                    /* jrz imm32                */  0x0F, 0x84, imm32(command.jump)
                )
                break;
            case OP_JRNZ:
                emit(
                    /* testb $255, (%rsp,%rbx)  */  0xF6, 0x04, 0x1C, 0xFF,
                    /* jrnz imm32               */  0x0F, 0x85, imm32(command.jump - BYTECODE_WEIGHTS[OP_JRNZ])
                )
                break;
            case OP_PUTC:
                emit(
                    /* mov $1, %eax             */  0xB8, 0x01, 0x00, 0x00, 0x00,
                    /* mov %eax, %edi           */  0x89, 0xC7,
                    /* lea (%rsp,%rbx), %rsi    */  0x48, 0x8D, 0x34, 0x1C,
                    /* mov %eax, %edx           */  0x89, 0xC2,
                    /* syscall                  */  0x0F, 0x05
                )
                break;
            case OP_GETC:
                emit(
                    /* xor %eax, %eax           */  0x31, 0xC0,
                    /* mov %eax, %edi           */  0x89, 0xC7,
                    /* lea (%rsp,%rbx), %rsi    */  0x48, 0x8D, 0x34, 0x1C,
                    /* mov $1, %edx             */  0xBA, 0x01, 0x00, 0x00, 0x00,
                    /* syscall                  */  0x0F, 0x05
                )
                break;
            case OP_END:
                emit(
                    /* add $65536, %rsp         */  0x48, 0x81, 0xC4, 0x00, 0x00, 0x01, 0x00,
                    /* xor %eax, %eax           */  0x31, 0xC0,
                    /* ret                      */  0xC3
                )
                break;
        }
    }

    return bytecode;
}
