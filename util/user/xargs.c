#include <stddef.h>

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

char argument[MAXARG][512];
char *argument_pointers[MAXARG];

int
main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(2, "usage: xargs command arguments...\n");
        exit();
    }

    if (argc - 1 > MAXARG) {
        fprintf(2, "xargs: number of arguments exceeds the limit\n");
        exit();
    }

    int i, cnt = 0;
    // ignore the first argument("xargs")
    for (i = 1; i < argc; i++) {
        strcpy(argument[cnt++], argv[i]);
    }

    char ch;
    char buf[512];
    int ch_pos = 0;
    int argv_pos = cnt;
    while (read(0, &ch, 1)) {
        if (ch == ' ' || ch == '\n') {
            buf[ch_pos] = '\0';
            if (argv_pos >= MAXARG) {
                fprintf(2, "xargs: number of arguments exceeds the limit\n");
                exit();
            }
            strcpy(argument[argv_pos++], buf);
            ch_pos = 0;
            if (ch == '\n') {
                int pid = fork();
                if (pid == 0) {
                    for (i = 0; i < argv_pos; i++) {
                        argument_pointers[i] = argument[i];
                    }
                    for (i = argv_pos; i < MAXARG; i++) {
                        argument_pointers[i] = NULL;
                    }
                    // the type of 2nd argument of "exec" system call
                    // is char**, and we can not pass char[][]
                    exec(argument[0], argument_pointers);
                    exit();
                } else {
                    argv_pos = cnt;
                    wait();
                }
            }
        } else {
            if (ch_pos >= 512) {
                fprintf(2, "xargs: argument is too long\n");
                exit();
            }
            buf[ch_pos++] = ch;
        }
    }

    exit();
}
