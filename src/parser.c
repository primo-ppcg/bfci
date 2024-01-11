#include "bitarray.h"
#include "program.h"
#include "parser.h"

static uint8_t mod_inv(uint16_t a) {
    uint16_t x = 0;
    uint16_t u = 1;
    uint16_t n = 256;
    while(a != 0) {
        uint16_t tmpx = x;
        x = u;
        u = tmpx - (n / a) * u;
        uint16_t tmpa = a;
        a = n % a;
        n = tmpa;
    }
    return (uint8_t)(x & 0xFF);
}

static Program unroll(Program subprog, uint8_t mul) {
    Program program = program_init();
    BitArray zeros = bitarray_init();
    uint16_t shift = 0;
    int16_t total_shift = 0;
    for(size_t i = 0; i < subprog.length; i++) {
        VmCommand cmd = subprog.commands[i];
        shift += cmd.shift;
        total_shift += cmd.shift;
        switch(cmd.op) {
            case OP_ADD: {
                if(total_shift != 0) {
                    if(get_bit(zeros, total_shift)) {
                        VmCommand command = { .op = OP_ADD, .value = cmd.value, .shift = shift };
                        program_append(&program, command);
                    } else if((uint8_t)(cmd.value * mul) == 1) {
                        VmCommand command = { .op = OP_CPY, .shift = shift };
                        program_append(&program, command);
                    } else {
                        VmCommand command = { .op = OP_MUL, .value = cmd.value * mul, .shift = shift };
                        program_append(&program, command);
                    }
                    shift = 0;
                }
                break;
            }
            case OP_SET: {
                VmCommand command = { .op = OP_SET, .value = cmd.value, .shift = shift };
                program_append(&program, command);
                set_bit(&zeros, total_shift);
                shift = 0;
                break;
            }
            default:
                break;
        }
    }

    VmCommand command = { .op = OP_SET, .value = 0, .shift = shift - total_shift };
    program_append(&program, command);

    bitarray_deinit(zeros);
    return program;
}

Program parse(char *source, size_t srclen, size_t *i, int *depth, bool interpret) {
    Program program = program_init(interpret);
    uint16_t shift = 0;
    int16_t total_shift = 0;
    uint8_t base_value = 0;
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
                    size_t j = *i;
                    (*i)++;
                    (*depth)++;
                    Program subprog = parse(source, srclen, i, depth, interpret);
                    if(subprog.commands[subprog.length - 1].op == OP_END) {
                        *i = j;
                        program_append(&program, subprog.commands[subprog.length - 1]);
                        program_deinit(subprog);
                        return program;
                    }
                    VmCommand command = {
                        .op = OP_JRZ,
                        .shift = shift,
                        .jump = interpret ? subprog.length : subprog.weight
                    };
                    program_append(&program, command);
                    program_concat(&program, subprog);
                    program_deinit(subprog);
                    shift = 0;
                    poison = true;
                }
                break;
            }
            case ']': {
                (*depth)--;
                if(total_shift == 0 && !poison && (base_value & 1) == 1) {
                    Program subprog = unroll(program, mod_inv(256 - (uint16_t)base_value));
                    program_deinit(program);
                    return subprog;
                }
                VmCommand command = {
                    .op = OP_JRNZ,
                    .shift = shift,
                    .jump = -(interpret ? program.length : program.weight)
                };
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
