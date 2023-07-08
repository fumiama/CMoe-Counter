#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>

#define THREADCNT 1024

int pipefds[2*THREADCNT];

#define create_or_open(filename) (open(filename, O_CREAT|O_RDWR, 0600))

int main(int argc, char** argv) {
    printf("run: ");
    for (int i = 0; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    puts(".");
    if (argc == 1) {
        for (int i = 0; i < THREADCNT; i++) {
            if (pipe(&pipefds[i*2]) < 0) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            }
            char buf[5];
            sprintf(buf, "%04d", i);
            if (!pid) {
                printf("[%s] start@[%d,%d]=(%d,%d).\n", buf, i*2, i*2+1, pipefds[i*2], pipefds[i*2+1]);
                dup2(pipefds[i*2+1], STDOUT_FILENO);
                close(pipefds[i*2]);
                //printf("[%s] sub close@%d=%d, execl.\n", buf, i*2, pipefds[i*2]);
                execl("./test", "./test", buf, NULL);
                perror("execl");
                exit(EXIT_FAILURE); // a success execl will never return
            } else {
                close(pipefds[i*2+1]);
                //printf("[%s] main close@%d=%d, read.\n", buf, i*2+1, pipefds[i*2+1]);
            }
        }
        if (!fork()) {
            char buf[128]; ssize_t n;
            for (int i = 0; i < THREADCNT; i++) {
                do {
                    n = read(pipefds[i*2], buf, sizeof(buf));
                    if (n < 0) {
                        perror("read");
                        exit(EXIT_FAILURE);
                    }
                    if (n > 0) {
                        buf[n] = 0;
                        printf("%zu> %s", n, buf);
                    }
                } while (n > 0);
            }
        } else waitpid(-1, NULL, 0);
        return 0;
    }
    usleep(rand()%1000);
    int fd = create_or_open("testlog.txt");
    if (fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    if(flock(fd, LOCK_EX)) {
        perror("flock");
        exit(EXIT_FAILURE);
    }
    FILE* fp = fdopen(fd, "rb+");
    fseek(fp, 0, SEEK_END);
    fputs(argv[1], fp);
    putc('\n', fp);
    srand(*(uint32_t*)argv[1]);
    int rndslp = rand()%1000;
    printf("[%s] get lock. sleep %d ms\n", argv[1], rndslp);
    usleep(rndslp);
    printf("[%s] release lock.\n", argv[1]);
    fclose(fp);
    //close(fd);
    return 0;
}
