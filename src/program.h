#pragma once

#include <stdlib.h>

#include "vm.h"

typedef struct {
    size_t length;
    size_t capacity;
    size_t weight;
    VmCommand *commands;
} Program;

Program program_init();
void program_deinit(Program program);

void program_concat(Program *program, Program subprog);
void program_append(Program *program, VmCommand command);
void program_drop_first(Program *program);
