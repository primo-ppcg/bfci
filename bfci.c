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

        /*
        for(int j = 0; j < program.length; j++) {
            VmCommand cmd = program.commands[j];
            printf("op: %d; value: %d; shift: %d; jump: %d\n", cmd.op, cmd.value, cmd.shift, cmd.jump);
        }
        printf("len: %d\n", program.length);
        */

        run(program.commands);
        program_deinit(program);
    }
    return 0;
}
