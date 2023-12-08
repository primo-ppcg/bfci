#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include "src/compiler.h"
#include "src/parser.h"

int main(int argc, char *args[]) {
    if(argc > 1) {
        FILE *fp;
        fp = fopen(args[1], "r");
        fseek(fp, 0, SEEK_END);
        size_t size = ftell(fp);
        rewind(fp);
        char *source = malloc(size);
        fread(source, 1, size, fp);
        fclose(fp);

        size_t i = 0;
        Program program = parse(source, size, &i);
        free(source);

        //run(program.commands);

        ByteCode bytecode = compile(program);
        off_t pagesize = sysconf(_SC_PAGESIZE);
        void *pagestart = (void *)((off_t)bytecode.ops & ~(pagesize - 1));
        off_t offset = (off_t)bytecode.ops - (off_t)pagestart;
        mprotect(pagestart, offset + bytecode.length, PROT_READ | PROT_WRITE | PROT_EXEC);
        ((void(*)(void))bytecode.ops)();
        bytecode_deinit(bytecode);

        program_deinit(program);
    }

    return 0;
}
