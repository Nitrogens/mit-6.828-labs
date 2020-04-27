#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define COMMAND_EXEC    0
#define COMMAND_REDIR   1
#define COMMAND_PIPE    2

char buf[512];

struct command {
    int type, argument_count, left_child, right_child;
    char argument[64][512];
    char input_file[512];
    char output_file[512];
} command_list[3];

int get_command(char *buf) {
    fprintf(2, "@ ");
    memset(buf, 0, 512);
    gets(buf, 512);
    if (buf[0] == 0) {
        return -1;
    }
    return 0;
}

int get_pipe_pos(char *buf) {
    int len = strlen(buf);
    int i;
    for (i = 0; i < len; i++) {
        if (buf[i] == '|') {
            return i;
        }
    }
    return len;
}

int get_redir_pos(char *buf, int len) {
    int i;
    for (i = 0; i < len; i++) {
        if (buf[i] == '<' || buf[i] == '>') {
            return i;
        }
    }
    return len;
}

void generate_single_command(char *buf, struct command *cmd, int len) {
    int i;
    cmd->type = COMMAND_EXEC;

    int redir_pos[2] = {len, len};
    redir_pos[0] = get_redir_pos(buf, len);
    if (redir_pos[0] < len) {
        redir_pos[1] = redir_pos[0] + 1 + get_redir_pos(buf + redir_pos[0] + 1, len - redir_pos[0] - 1);
    }

    int len_tmp = 0;
    char tmp[512];

    // process arguments
    for (i = 0; i < redir_pos[0]; i++) {
        if (buf[i] == ' ' || buf[i] == '\n' || buf[i] == '\t') {
            if (len_tmp > 0) {
                tmp[len_tmp] = 0;
                strcpy(cmd->argument[(cmd->argument_count)++], tmp);
                len_tmp = 0;
            }
            continue;
        }
        tmp[len_tmp++] = buf[i];
    }
    if (len_tmp > 0) {
        tmp[len_tmp] = 0;
        strcpy(cmd->argument[(cmd->argument_count)++], tmp);
        len_tmp = 0;
    }

    // process redir
    int idx;
    for (idx = 0; idx < 2; idx++) {
        if (redir_pos[idx] < len) {
            cmd->type = COMMAND_REDIR;
            for (i = redir_pos[idx] + 1; i < ((idx == 0) ? redir_pos[1] : len); i++) {
                if (buf[i] == ' ' || buf[i] == '\n' || buf[i] == '\t') {
                    if (len_tmp > 0) {
                        tmp[len_tmp] = 0;
                        if (buf[redir_pos[idx]] == '<') {
                            strcpy(cmd->input_file, tmp);
                        } else {
                            strcpy(cmd->output_file, tmp);
                        }
                        len_tmp = 0;
                    }
                    continue;
                }
                tmp[len_tmp++] = buf[i];
            }
            if (len_tmp > 0) {
                tmp[len_tmp] = 0;
                if (buf[redir_pos[idx]] == '<') {
                    strcpy(cmd->input_file, tmp);
                } else {
                    strcpy(cmd->output_file, tmp);
                }
                len_tmp = 0;
            }
        }
    }
}

void run_command(struct command *cmd) {
    char *argument_pointer[64];
    int i;
    int p[2];
    for (i = 0; i < cmd->argument_count; i++) {
        argument_pointer[i] = cmd->argument[i];
    }
    switch (cmd->type) {
        case COMMAND_EXEC:
            exec(cmd->argument[0], argument_pointer);
            fprintf(2, "exec %s failed\n", cmd->argument[0]);
            exit(-1);
            break;

        case COMMAND_REDIR:
            if (strlen(cmd->input_file) > 0) {
                close(0);
                if (open(cmd->input_file, O_RDONLY) < 0) {
                    fprintf(2, "open %s failed\n", cmd->input_file);
                    exit(-1);
                }
            }
            if (strlen(cmd->output_file) > 0) {
                close(1);
                if (open(cmd->output_file, O_WRONLY | O_CREATE) < 0) {
                    fprintf(2, "open %s failed\n", cmd->output_file);
                    exit(-1);
                }
            }
            exec(cmd->argument[0], argument_pointer);
            fprintf(2, "exec %s failed\n", cmd->argument[0]);
            exit(-1);
            break;

        case COMMAND_PIPE:
            if (pipe(p) < 0) {
                fprintf(2, "create pipe failed\n");
                exit(-1);
            }
            int pid1 = fork();
            if (pid1 < 0) {
                fprintf(2, "fork failed\n");
                exit(-1);
            }
            if (pid1 == 0) {
                close(0);
                dup(p[0]);
                close(p[0]);
                close(p[1]);
                run_command(&command_list[cmd->right_child]);
            }
            int pid2 = fork();
            if (pid2 < 0) {
                fprintf(2, "fork failed\n");
                exit(-1);
            }
            if (pid2 == 0) {
                close(1);
                dup(p[1]);
                close(p[0]);
                close(p[1]);
                run_command(&command_list[cmd->left_child]);
            }
            close(p[0]);
            close(p[1]);
            wait(0);
            wait(0);
            break;
    }
}

int main(int argc, char *argv[]) {

    while (get_command(buf) == 0) {
        if (buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
            // Chdir must be called by the parent, not the child.
            buf[strlen(buf) - 1] = 0;  // chop \n
            if(chdir(buf + 3) < 0)
                fprintf(2, "cannot cd %s\n", buf + 3);
            continue;
        }
        int pid = fork();
        if (pid < 0) {
            fprintf(2, "fork failed\n");
            exit(-1);
        }
        if (pid == 0) {
            memset(command_list, 0, sizeof(command_list));
            int len = strlen(buf);
            int pos_pipe = get_pipe_pos(buf);
            if (pos_pipe < len) {
                command_list[0].type = COMMAND_PIPE;
                command_list[0].left_child = 1;
                command_list[0].right_child = 2;
                generate_single_command(buf, &command_list[1], pos_pipe);
                generate_single_command(buf + pos_pipe + 1, &command_list[2], len - pos_pipe - 1);
            } else {
                generate_single_command(buf, &command_list[0], len);
            }
            run_command(&command_list[0]);
        }
        wait(0);
    }

    exit(0);
}
