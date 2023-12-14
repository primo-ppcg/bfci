#include <argp.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "src/compiler.h"
#include "src/parser.h"

static char doc[] = "A simple, small, moderately optimizing compiling interpreter for the brainfuck programming language.";
static char args_doc[] = "[file]";

static struct argp_option options[] = {
    { "interpret", 'i', 0, 0, "run as an interpreter (default)", 0 },
    { "execute", 'x', 0, 0, "compile and execute from memory", 0 },
    { "code", 'c', "code", 0, "a string of instructions to be executed\nif present, the file argument will be ignored", 0 },
    { 0 }
};

struct arguments {
    bool interpret;
    char *file;
    char *code;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;
    switch(key) {
        case 'i':
            arguments->interpret = true;
            break;
        case 'x':
            arguments->interpret = false;
            break;
        case 'c':
            arguments->code = arg;
        case ARGP_KEY_ARG:
            if(state->arg_num >= 1) {
                argp_usage(state);
            }
            arguments->file = arg;
            break;
        case ARGP_KEY_END:
            if(arguments->file == NULL && arguments->code == NULL) {
                argp_usage(state);
            }
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static struct argp argp = { .options = options, .parser = parse_opt, .args_doc = args_doc, .doc = doc };

int main(int argc, char *argv[]) {
    struct arguments args = { .interpret = true, .file = NULL, .code = NULL };

    argp_parse(&argp, argc, argv, 0, 0, &args);

    Program program;
    if(args.code != NULL) {
        size_t i = 0;
        program = parse(args.code, strlen(args.code), &i, args.interpret);
    } else if(args.file != NULL) {
        FILE *fp = fopen(args.file, "r");
        if(fp == NULL) {
            perror(args.file);
            return errno;
        }

        fseek(fp, 0, SEEK_END);
        size_t size = ftell(fp);
        rewind(fp);
        char *source = malloc(size);
        fread(source, sizeof(char), size, fp);
        fclose(fp);

        size_t i = 0;
        program = parse(source, size, &i, args.interpret);
        free(source);
    } else {
        return 0;
    }

    if(args.interpret) {
        vm_run(program.commands);
    } else {
        ByteCode bytecode = compile(program);
        off_t pagesize = sysconf(_SC_PAGESIZE);
        void *pagestart = (void *)((off_t)bytecode.ops & ~(pagesize - 1));
        off_t offset = (off_t)bytecode.ops - (off_t)pagestart;
        mprotect(pagestart, offset + bytecode.length, PROT_READ | PROT_WRITE | PROT_EXEC);
        ((void(*)(void))bytecode.ops)();
        bytecode_deinit(bytecode);
    }

    program_deinit(program);

    return 0;
}
