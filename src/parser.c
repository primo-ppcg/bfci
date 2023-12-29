#include "annotator.h"
#include "bitarray.h"
#include "compiler.h"
#include "program.h"
#include "parser.h"

static Program unroll(char *source, size_t *i, Annotation annotation) {
    Program program = program_init();
    BitArray zeros = bitarray_init();

    int depth = 0;
    uint16_t shift = 0;
    int16_t inner_shift = 0;
    int16_t total_shift = 0;
    size_t child = 0;
    uint8_t mul_value = annotation.value;

    for(;; (*i)++) {
        char c = source[*i];
        switch(c) {
            case '<':
            case '>':
                shift += c - 61;
                inner_shift += c - 61;
                total_shift += c - 61;
                break;
            case '+':
            case '-': {
                uint8_t value = 44 - c;
                while(source[*i + 1] == '+' || source[*i + 1] == '-') {
                    (*i)++;
                    value += 44 - source[*i];
                }
                if(total_shift != 0 && inner_shift != 0) {
                    if(get_bit(zeros, total_shift)) {
                        VmCommand command = { .op = OP_ADD, .value = value, .shift = shift };
                        program_append(&program, command);
                    } else if((uint8_t)(value * mul_value) == 1) {
                        VmCommand command = { .op = OP_CPY, .shift = shift };
                        program_append(&program, command);
                    } else {
                        VmCommand command = { .op = OP_MUL, .value = value * mul_value, .shift = shift };
                        program_append(&program, command);
                    }
                    shift = 0;
                }
                break;
            }
            case '[': {
                mul_value = annotation.child_values[child++];
                if(mul_value != 0) {
                    VmCommand command = { .op = OP_SHL, .shift = shift };
                    program_append(&program, command);
                    shift = 0;
                }
                depth++;
                inner_shift = 0;
                break;
            }
            case ']': {
                if(depth > 0 && mul_value != 0) {
                    VmCommand command1 = { .op = OP_SHR, .value = 0, .shift = shift };
                    VmCommand command2 = { .op = OP_SET, .value = 0, .shift = 0 };
                    program_append(&program, command1);
                    program_append(&program, command2);
                    shift = 0;
                } else {
                    VmCommand command = { .op = OP_SET, .value = 0, .shift = shift };
                    program_append(&program, command);
                    shift = 0;
                }
                if(depth == 0) {
                    bitarray_deinit(zeros);
                    return program;
                }
                depth--;
                break;
            }
            default:
                break;
        }
    }
}

Program parse(char *source, size_t srclen, size_t *i, int *depth, bool interpret) {
    Program program = program_init();
    uint16_t shift = 0;
    for(; *i < srclen; (*i)++) {
        char c = source[*i];
        switch(c) {
            case '<':
            case '>':
                shift += c - 61;
                break;
            case '+':
            case '-': {
                uint8_t value = 44 - c;
                while(*i + 1 < srclen && (source[*i + 1] == '+' || source[*i + 1] == '-')) {
                    (*i)++;
                    value += 44 - source[*i];
                }
                // TODO: set+add
                // unrolled loops must not jump over set
                if(shift == 0 && program.length > 0 && program.commands[program.length - 1].op == OP_ADD) {
                    program.commands[program.length - 1].value += value;
                } else {
                    VmCommand command = { .op = OP_ADD, .value = value, .shift = shift };
                    program_append(&program, command);
                    shift = 0;
                }
                break;
            }
            case '.': {
                VmCommand command = { .op = OP_PUTC, .shift = shift };
                program_append(&program, command);
                shift = 0;
                break;
            }
            case ',': {
                VmCommand command = { .op = OP_GETC, .shift = shift };
                program_append(&program, command);
                shift = 0;
                break;
            }
            case '[': {
                size_t j = *i;
                (*i)++;
                Annotation annotation = annotate(source, srclen, *i);
                if(annotation.elidable) {
                    Program subprog = unroll(source, i, annotation);
                    if(annotation.value != 0) {
                        VmCommand command = {
                            .op = OP_JRZ,
                            .shift = shift,
                            .jump = interpret ? subprog.length : subprog.weight
                        };
                        program_append(&program, command);
                    } else {
                        subprog.weight -= bytecode_weight(subprog.commands[0]);
                        subprog.commands[0].shift += shift;
                        subprog.weight += bytecode_weight(subprog.commands[0]);
                    }
                    program_concat(&program, subprog);
                    program_deinit(subprog);
                } else {
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
                }
                annotation_deinit(annotation);
                shift = 0;
                break;
            }
            case ']': {
                (*depth)--;
                // cosolidate multiple closing brackets (e.g. ]])
                if(shift != 0 || program.commands[program.length - 1].op != OP_JRNZ) {
                    VmCommand command = {
                        .op = OP_JRNZ,
                        .shift = shift,
                        .jump = -(interpret ? program.length : program.weight)
                    };
                    program_append(&program, command);
                }
                return program;
            }
            default:
                break;
        }
    }

    VmCommand command = { .op = OP_END };
    program_append(&program, command);
    return program;
}
