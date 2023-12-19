#include <elf.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "compiler.h"

#define emit(...) { \
    uint8_t opcodes[] = { __VA_ARGS__ }; \
    bytecode_append(&bytecode, sizeof(opcodes), opcodes); \
}

#define imm8(value) (value) & 0xFF
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
        /* push %rbx                */  0x53,
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
                    /* pop %rbx                 */  0x5B,
                    /* ret                      */  0xC3
                )
                break;
        }
    }

    return bytecode;
}

bool write_executable(ByteCode bytecode, char *path) {
    uint8_t opcodes[] = {
        /* call $9                  */  0xE8, 0x09, 0x00, 0x00, 0x00,
        /* mov $60, %eax            */  0xB8, 0x3C, 0x00, 0x00, 0x00,
        /* xor %edi, %edi           */  0x31, 0xFF,
        /* syscall                  */  0x0F, 0x05
    };
    ByteCode entry = { .length = sizeof(opcodes), .ops = opcodes };

    const Elf64_Addr vaddr = 0x800000;
    const Elf64_Off hsize = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr);
    const Elf64_Xword filesz = hsize + entry.length + bytecode.length;

    Elf64_Ehdr fileheader = {
        .e_ident = { ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, ELFCLASS64, ELFDATA2LSB, EV_CURRENT, ELFOSABI_NONE, 0 },
        .e_type = ET_EXEC,
        .e_machine = EM_X86_64,
        .e_version = EV_CURRENT,
        .e_entry = vaddr + hsize,
        .e_phoff = sizeof(Elf64_Ehdr),
        .e_shoff = 0,
        .e_flags = 0,
        .e_ehsize = sizeof(Elf64_Ehdr),
        .e_phentsize = sizeof(Elf64_Phdr),
        .e_phnum = 1,
        .e_shentsize = sizeof(Elf64_Shdr),
        .e_shnum = 0,
        .e_shstrndx = SHN_UNDEF
    };
    Elf64_Phdr progheader = {
        .p_type = PT_LOAD,
        .p_flags = PF_R | PF_X,
        .p_offset = 0,
        .p_vaddr = vaddr,
        .p_paddr = vaddr,
        .p_filesz = filesz,
        .p_memsz = filesz,
        .p_align = 0x1000
    };

    FILE *fp = fopen(path, "w");
    if(fp == NULL) {
        return false;
    }

    fwrite((void *)&fileheader, sizeof(Elf64_Ehdr), 1, fp);
    fwrite((void *)&progheader, sizeof(Elf64_Phdr), 1, fp);
    fwrite(entry.ops, sizeof(uint8_t), entry.length, fp);
    fwrite(bytecode.ops, sizeof(uint8_t), bytecode.length, fp);
    fclose(fp);

    chmod(path, 0744);

    return true;
}
