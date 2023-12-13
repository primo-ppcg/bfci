#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "vm.h"

typedef struct {
    size_t length;
    size_t capacity;
    size_t weight;
    bool interpret;
    VmCommand *commands;
} Program;

Program program_init(bool interpret);
void program_deinit(Program program);

void program_concat(Program *program, Program subprog);
void program_append(Program *program, VmCommand command);
