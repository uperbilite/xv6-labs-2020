#include "kernel/param.h"
#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    char *args[MAXARG];
    int cnt = 0;
    for (int line = 0; line < MAXARG && cnt < MAXARG; line++) {
        char *buf = malloc(128);
        gets(buf, 128);

        int n = strlen(buf);
        if (buf[n - 1] == '\n') {
            buf[n - 1] = '\0';
            n--;
        }

        for (int i = 0; i < n; i++) {
            while (i < n && buf[i] == ' ') {
                i++;
            }
            int j = i;
            while (j < n && buf[j] != ' ') {
                j++;
            }
            buf[j] = '\0';
            args[cnt] = malloc(strlen(&buf[i]));
            strcpy(args[cnt], &buf[i]);
            i = j;
            cnt++;
        }
    }

    for (int i = 0; i < cnt; i++) {
        char* xargs[argc];
        for (int j = 0; j < argc - 1; j++) {
            xargs[j] = argv[j + 1];
        }
        xargs[argc - 1] = args[i];
        
        if (fork() == 0) {
            exec(argv[1], xargs);
            exit(0);
        } else {
            wait(0);
        }
    }

    exit(0);
}