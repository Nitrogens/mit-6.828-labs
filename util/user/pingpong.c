#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char **argv)
{
    int parent_fd[2];
    int child_fd[2];
    pipe(parent_fd);
    pipe(child_fd);

    int pid = fork();
    if (pid == 0) {
        char data[2];
        close(parent_fd[1]);
        read(parent_fd[0], data, 1);
        close(parent_fd[0]);
        fprintf(1, "%d: received ping\n", getpid());

        close(child_fd[0]);
        write(child_fd[1], "1", 1);
        close(child_fd[1]);
    } else {
        close(parent_fd[0]);
        write(parent_fd[1], "1", 1);
        close(parent_fd[1]);

        char data[2];
        close(child_fd[1]);
        read(child_fd[0], data, 1);
        close(child_fd[0]);
        fprintf(1, "%d: received pong\n", getpid());
    }

    exit();
}
