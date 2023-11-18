#include "program.h"

Program program_init() {
    Program program = { .length = 0, .capacity = 8, .commands = malloc(8 * sizeof(VmCommand)) };
    return program;
}

void program_deinit(Program program) {
    free(program.commands);
}

void program_concat(Program *program, Program subprog) {
    if(program->length + subprog.length > program->capacity) {
        program->capacity += subprog.length;
        program->commands = realloc(program->commands, program->capacity * sizeof(VmCommand));
    }
    memcpy(&program->commands[program->length], subprog.commands, subprog.length * sizeof(VmCommand));
    program->length += subprog.length;
}

void program_append(Program *program, VmCommand command) {
    if(program->length + 1 > program->capacity) {
        program->capacity += 8;
        program->commands = realloc(program->commands, program->capacity * sizeof(VmCommand));
    }
    program->commands[program->length] = command;
    program->length++;
}
