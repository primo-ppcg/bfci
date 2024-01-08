#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "src/compiler.h"
#include "src/parser.h"

typedef enum {
    INTERPRET,
    EXECUTE,
    COMPILE
} OperationMode;

typedef struct {
    OperationMode mode;
    char *code;
    char *file;
    char *outfile;
} Arguments;

static struct option long_options[] = {
    { "interpret", no_argument, 0, 'i' },
    { "execute", no_argument, 0, 'x' },
    { "outfile", required_argument, 0, 'o' },
    { "code", required_argument, 0, 'c' },
    { "help", no_argument, 0, 'h' },
    { 0 }
};

static void display_usage(char *name) {
    fprintf(stderr, "Usage: %s [-ixo:] (-c <code> | <file>)\n", name);
}

static void display_help(char *name) {
    display_usage(name);
    fprintf(stderr,
        "\n"
        "A simple, small, moderately optimizing compiling interpreter for the\n"
        "brainfuck programming language.\n"
        "\n"
        "Arguments:\n"
        "  file              a brainfuck script file to execute\n"
        "\n"
        "Options:\n"
        "  -c, --code=       a string of instructions to be executed\n"
        "                    if present, the file argument will be ignored\n"
        "  -i, --interpret   run as an interpreter (default)\n"
        "  -x, --execute     compile and execute from memory\n"
        "  -o, --outfile=    compile and write to file\n"
        "  -h, --help        display this message\n"
    );
}

static bool parse_args(int argc, char *argv[], Arguments *args) {
    int option_index = 0;
    int opt = 0;
    while((opt = getopt_long(argc, argv, "ixo:c:h", long_options, &option_index)) != -1) {
        if(opt == 0) {
            opt = long_options[option_index].val;
        }
        switch(opt) {
            case 'i':
                args->mode = INTERPRET;
                break;
            case 'x':
                args->mode = EXECUTE;
                break;
            case 'o':
                args->mode = COMPILE;
                args->outfile = optarg;
                break;
            case 'c':
                args->code = optarg;
                break;
            case 'h':
                display_help(argv[0]);
                return false;
                break;
            case '?':
                display_usage(argv[0]);
                return false;
                break;
        }
    }

    if(optind == argc - 1) {
        args->file = argv[optind];
    } else if(optind < argc - 1) {
        display_usage(argv[0]);
        return false;
    }

    return true;
}

int main(int argc, char *argv[]) {
    Arguments args = { .mode = INTERPRET, .code = NULL, .file = NULL, .outfile = NULL };
    if(!parse_args(argc, argv, &args)) {
        return 1;
    }

    Program program;
    size_t i = 0;
    int depth = 0;
    if(args.code != NULL) {
        program = parse(args.code, strlen(args.code), &i, &depth, args.mode == INTERPRET);
    } else if(args.file != NULL) {
        FILE *fp = fopen(args.file, "r");
        if(fp == NULL) {
            perror(args.file);
            return 1;
        }

        fseek(fp, 0, SEEK_END);
        size_t size = ftell(fp);
        rewind(fp);
        char *source = malloc(size);
        fread(source, sizeof(char), size, fp);
        fclose(fp);

        program = parse(source, size, &i, &depth, args.mode == INTERPRET);
        free(source);
    } else {
        display_usage(argv[0]);
        return 1;
    }

    if(depth > 0) {
        fprintf(stderr, "Unmatched `[` in source at position %zu\n", i);
        program_deinit(program);
        return 1;
    } else if(depth < 0) {
        fprintf(stderr, "Unmatched `]` in source at position %zu\n", i);
        program_deinit(program);
        return 1;
    }

#ifdef DEBUG
    static const char *OP_NAMES[] = {
        "jrz",
        "jrnz",
        "add",
        "set",
        "cpy",
        "mul",
        "putc",
        "getc",
        "end"
    };

    for(size_t j = 0; j < program.length; j++) {
        VmCommand cmd = program.commands[j];
        printf("(%s %d %d %d)\n", OP_NAMES[cmd.op], (int8_t)cmd.value, (int16_t)cmd.shift, (int32_t)cmd.jump);
    }
    printf("length: %ld; weight: %ld\n", program.length, program.weight);
#endif

    switch(args.mode) {
        case INTERPRET:
            vm_run(program.commands);
            break;
        case EXECUTE: {
            ByteCode bytecode = compile(program);
            off_t pagesize = sysconf(_SC_PAGESIZE);
            void *pagestart = (void *)((off_t)bytecode.ops & ~(pagesize - 1));
            off_t offset = (off_t)bytecode.ops - (off_t)pagestart;
            mprotect(pagestart, offset + bytecode.length, PROT_READ | PROT_WRITE | PROT_EXEC);
            ((void(*)(void))bytecode.ops)();
            bytecode_deinit(bytecode);
            break;
        }
        case COMPILE: {
            ByteCode bytecode = compile(program);
            if(!write_executable(bytecode, args.outfile)) {
                bytecode_deinit(bytecode);
                program_deinit(program);
                perror(args.outfile);
                return 1;
            }
            bytecode_deinit(bytecode);
            break;
        }
    }

    program_deinit(program);
    return 0;
}
