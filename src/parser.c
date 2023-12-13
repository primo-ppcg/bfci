#include <assert.h>

#include "bitarray.h"
#include "program.h"
#include "parser.h"

static const uint8_t MOD_INV[256] = {
    0x00, 0xFF, 0xFF, 0x55, 0xFF, 0x33, 0xD5, 0x49, 0xFF, 0xC7, 0x33, 0x5D, 0x15, 0x3B, 0xC9, 0x11,
    0xFF, 0x0F, 0xC7, 0xE5, 0xF3, 0xC3, 0xDD, 0x59, 0xF5, 0xD7, 0x3B, 0xED, 0x09, 0xCB, 0x11, 0x21,
    0xFF, 0x1F, 0x0F, 0x75, 0x07, 0x53, 0xE5, 0x69, 0xF3, 0xE7, 0xC3, 0x7D, 0x1D, 0x5B, 0xD9, 0x31,
    0x05, 0x2F, 0xD7, 0x05, 0xFB, 0xE3, 0xED, 0x79, 0x09, 0xF7, 0xCB, 0x0D, 0x11, 0xEB, 0x21, 0x41,
    0xFF, 0x3F, 0x1F, 0x95, 0x0F, 0x73, 0xF5, 0x89, 0x07, 0x07, 0xD3, 0x9D, 0xE5, 0x7B, 0xE9, 0x51,
    0x03, 0x4F, 0xE7, 0x25, 0x03, 0x03, 0xFD, 0x99, 0xFD, 0x17, 0xDB, 0x2D, 0x19, 0x0B, 0x31, 0x61,
    0xFD, 0x5F, 0x2F, 0xB5, 0x17, 0x93, 0x05, 0xA9, 0xFB, 0x27, 0xE3, 0xBD, 0xED, 0x9B, 0xF9, 0x71,
    0xF9, 0x6F, 0xF7, 0x45, 0x0B, 0x23, 0x0D, 0xB9, 0xF1, 0x37, 0xEB, 0x4D, 0xE1, 0x2B, 0xC1, 0x81,
    0x01, 0x7F, 0x3F, 0xD5, 0x1F, 0xB3, 0x15, 0xC9, 0x0F, 0x47, 0xF3, 0xDD, 0xF5, 0xBB, 0x09, 0x91,
    0x07, 0x8F, 0x07, 0x65, 0x13, 0x43, 0x1D, 0xD9, 0x05, 0x57, 0xFB, 0x6D, 0xE9, 0x4B, 0xD1, 0xA1,
    0x03, 0x9F, 0xCF, 0xF5, 0xE7, 0xD3, 0x25, 0xE9, 0x03, 0x67, 0x03, 0xFD, 0xFD, 0xDB, 0x19, 0xB1,
    0xFD, 0xAF, 0x17, 0x85, 0x1B, 0x63, 0x2D, 0xF9, 0xF9, 0x77, 0x0B, 0x8D, 0xF1, 0x6B, 0xE1, 0xC1,
    0x01, 0xBF, 0xDF, 0x15, 0xEF, 0xF3, 0x35, 0x09, 0xF7, 0x87, 0x13, 0x1D, 0x05, 0xFB, 0x29, 0xD1,
    0xFB, 0xCF, 0x27, 0xA5, 0xE3, 0x83, 0x3D, 0x19, 0x0D, 0x97, 0x1B, 0xAD, 0xF9, 0x8B, 0xF1, 0xE1,
    0x01, 0xDF, 0xEF, 0x35, 0xF7, 0x13, 0xC5, 0x29, 0x0B, 0xA7, 0x23, 0x3D, 0x0D, 0x1B, 0x39, 0xF1,
    0x01, 0xEF, 0x37, 0xC5, 0xEB, 0xA3, 0xCD, 0x39, 0x01, 0xB7, 0x2B, 0xCD, 0x01, 0xAB, 0x01, 0x01
};

static Program unroll(char *source, size_t i, bool interpret, uint8_t mul) {
    Program program = program_init(interpret);
    BitArray zeros = bitarray_init();
    uint16_t shift = 0;
    int16_t total_shift = 0;
    for(;; i++) {
        char c = source[i];
        switch(c) {
            case '>':
                shift++;
                total_shift++;
                break;
            case '<':
                shift--;
                total_shift--;
                break;
            case '[': {
                assert((source[i + 1] == '+' || source[i + 1] == '-') && source[i + 2] == ']' && total_shift != 0);
                i += 2;
                uint8_t value = 0;
                while(source[i + 1] == '+' || source[i + 1] == '-') {
                    i++;
                    value += 44 - source[i];
                }
                VmCommand command = { .op = OP_SET, .value = value, .shift = shift };
                program_append(&program, command);
                shift = 0;
                set_bit(&zeros, total_shift);
                break;
            }
            case ']': {
                assert(total_shift == 0);
                VmCommand command = { .op = OP_SET, .value = 0, .shift = shift };
                program_append(&program, command);
                bitarray_deinit(zeros);
                return program;
            }
            case '+':
            case '-': {
                uint8_t value = 44 - c;
                while(source[i + 1] == '+' || source[i + 1] == '-') {
                    i++;
                    value += 44 - source[i];
                }
                if(total_shift != 0) {
                    if(get_bit(zeros, total_shift)) {
                        VmCommand command = { .op = OP_ADD, .value = value, .shift = shift };
                        program_append(&program, command);
                    } else {
                        VmCommand command = { .op = OP_MUL, .value = value * mul, .shift = shift };
                        program_append(&program, command);
                    }
                    shift = 0;
                }
                break;
            }
            default:
                break;
        }
    }
}

Program parse(char *source, size_t srclen, size_t *i, bool interpret) {
    Program program = program_init(interpret);
    uint16_t shift = 0;
    int16_t total_shift = 0;
    uint8_t base_value = 0;
    size_t base_i = *i;
    bool poison = false;
    for(; *i < srclen; (*i)++) {
        char c = source[*i];
        switch(c) {
            case '>':
                shift++;
                total_shift++;
                break;
            case '<':
                shift--;
                total_shift--;
                break;
            case '.': {
                VmCommand command = { .op = OP_PUTC, .shift = shift };
                program_append(&program, command);
                shift = 0;
                poison = true;
                break;
            }
            case ',': {
                VmCommand command = { .op = OP_GETC, .shift = shift };
                program_append(&program, command);
                shift = 0;
                poison = true;
                break;
            }
            case '[': {
                if(*i + 2 < srclen && (source[*i + 1] == '+' || source[*i + 1] == '-') && source[*i + 2] == ']') {
                    (*i) += 2;
                    uint8_t value = 0;
                    while(*i + 1 < srclen && (source[*i + 1] == '+' || source[*i + 1] == '-')) {
                        (*i)++;
                        value += 44 - source[*i];
                    }
                    if(total_shift == 0) {
                        poison = true;
                    }
                    VmCommand command = { .op = OP_SET, .value = value, .shift = shift };
                    program_append(&program, command);
                    shift = 0;
                } else {
                    (*i)++;
                    Program subprog = parse(source, srclen, i, interpret);
                    VmCommand command = { .op = OP_JRZ, .shift = shift, .jump = subprog.weight };
                    program_append(&program, command);
                    program_concat(&program, subprog);
                    program_deinit(subprog);
                    shift = 0;
                    poison = true;
                }
                break;
            }
            case ']': {
                if(total_shift == 0 && !poison && (base_value & 1) == 1) {
                    program_deinit(program);
                    return unroll(source, base_i, interpret, MOD_INV[base_value]);
                }
                VmCommand command = { .op = OP_JRNZ, .shift = shift, .jump = -program.weight };
                program_append(&program, command);
                return program;
            }
            case '+':
            case '-': {
                uint8_t value = 44 - c;
                while(*i + 1 < srclen && (source[*i + 1] == '+' || source[*i + 1] == '-')) {
                    (*i)++;
                    value += 44 - source[*i];
                }
                if(total_shift == 0) {
                    base_value += value;
                }
                VmCommand command = { .op = OP_ADD, .value = value, .shift = shift };
                program_append(&program, command);
                shift = 0;
                break;
            }
            default:
                break;
        }
    }

    VmCommand command = { .op = OP_END };
    program_append(&program, command);
    return program;
}
