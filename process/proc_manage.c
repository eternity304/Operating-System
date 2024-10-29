#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int proper_proc_create() {
    pid_t returned_pid = fork();
    if (returned_pid == -1) {
        int err = errno; // capture error code
        perror("fork failed");
        return err;
    } if (returned_pid == 0) { // child process
        printf("Child returned pid from fork: %d\n", returned_pid);
        printf("Child process' own pid from get_pid(): %d\n", getpid());
        printf("Child's parent's pid from get_ppid(): %d\n", getppid());
    } else { // parent process
        sleep(2);
        printf("Parent pid: %d\n", getpid());
        printf("Parent'sChild pid: %d\n", returned_pid);
    }
}

int main() {
    printf("Main pid: %d\n", getpid());
    proper_proc_create();
    return 0;
}