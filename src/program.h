#include <stdlib.h>
#include <string.h>

#include "vm.h"

typedef struct {
    size_t length;
    size_t capacity;
    VmCommand *commands;
} Program;

Program program_init();
void program_deinit();

void program_concat(Program *program, Program subprog);
void program_append(Program *program, VmCommand command);
