#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char **argv)
{
    int i;
    int input_fd[2];
    int output_fd[2];
    pipe(input_fd);
    pipe(output_fd);

    for (i = 2; i <= 35; i++) {
        // Feed all integers between 2 to 35 into the pipe(input_fd)
        write(input_fd[1], &i, 1);
    }
    close(input_fd[1]);

    int pid = fork();
    if (pid > 0) {
        close(input_fd[0]);
        close(output_fd[1]);

        char data[2];
        // read primes from the pipe(output_fd) and print them
        while (read(output_fd[0], data, 1)) {
            int number = (int)data[0];
            fprintf(1, "prime %d\n", number);
        }
        close(output_fd[0]);
    } else {
        close(output_fd[0]);
        char data[2];
        // read numbers from the pipe(input_fd) and create a child process to process them
        while (read(input_fd[0], data, 1)) {
            int number = (int)data[0];
            int sub_pid = fork();
            if (sub_pid > 0) {
                // if the process is parent process, continue loop
                continue;
            } else {
                // if the process is child process, process the current number
                int is_prime = 1;
                for (i = 2; i < number; i++) {
                    if (number % i == 0) {
                        is_prime = 0;
                        break;
                    }
                }
                if (is_prime) {
                    write(output_fd[1], &number, 1);
                }
                close(output_fd[1]);
                // DO NOT FORGET to exit for the child process
                exit();
            }
        }
        close(input_fd[0]);
        close(output_fd[1]);
    }

    exit();
}
