#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
next(int prev_pipe[])
{
    close(prev_pipe[1]);

    int* prime_num = malloc(4);
    int n;
    if ((n = read(prev_pipe[0], prime_num, 4)) == 0) {
        close(prev_pipe[0]);
        exit(0);
    }
    printf("prime %d\n", *prime_num);

    int next_pipe[2];
    pipe(next_pipe);

    if (fork() == 0) {
        next(next_pipe);
    } else {
        int* p = malloc(4);
        close(next_pipe[0]);
        while ((n = read(prev_pipe[0], p, 4)) > 0) {
            if ((*p) % (*prime_num) != 0) {
                write(next_pipe[1], p, 4);
            }
        }
        close(prev_pipe[0]);
        close(next_pipe[1]);
        wait(0);
    }

    exit(0);
}

int
main(int argc, char *argv[]) 
{
    int next_pipe[2];
    pipe(next_pipe);

    if (fork() == 0) {
        next(next_pipe);
    } else {
        close(next_pipe[0]);
        for (int i = 2; i <= 35; i++) {
            write(next_pipe[1], &i, 4);
        }
        close(next_pipe[1]);
        wait(0);
    }

    exit(0);
}
