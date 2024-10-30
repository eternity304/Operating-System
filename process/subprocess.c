#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>
#include <fcntl.h>    
#include <unistd.h> 
#include <string.h>  

void check(int ret, const char* message) {
    // take ret, a return value from a system call
    if (ret != -1) {
        return;
    }
    // if an error occur save errono, set perror message and exit with the err
    int err = errno;
    perror(message);
    exit(err);
}

static void parent(int in_pipefd[2], int out_pipefd[2], pid_t child_pid) {
    check(close(in_pipefd[0]), "message"); // close read end of in pipe
    check(close(out_pipefd[1]), "close"); // parents does not write

    const char* str = "Testing\n";
    int len = strlen(str);
    int bytes_written = write(in_pipefd[1], str, len);
    check(bytes_written, "write");
    check(close(in_pipefd[1]), "message"); // close write end of in pipe after written

    char buffer[4096];
    int bytes_read = read(out_pipefd[0], buffer, sizeof(buffer));
    check(bytes_read, "read");
    printf("read: %.*s", bytes_read, buffer);
    check(close(out_pipefd[0]), "close"); // close it after reading

    int wstatus = 0;
    pid_t pid = waitpid(child_pid, &wstatus, 0);
    check(pid, "waitpid");
    assert(WIFEXITED(wstatus));
    assert(WEXITSTATUS(wstatus) == 0);

}

static void child(int in_pipefd[2], int out_pipefd[2], const char *program) {
    check(dup2(out_pipefd[1], 1), "dup2");
    check(close(out_pipefd[0]), "close");
    check(close(out_pipefd[1]), "close");
    check(dup2(in_pipefd[0], 0), "dup2");
    check(close(in_pipefd[0]), "close");
    check(close(in_pipefd[1]), "close");
    check(
        execlp(program, program, NULL),
        "execlp"
    );
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        return EINVAL;
    }

    int in_pipefd[2] = {0}; // zero initialize the entire array
    int out_pipefd[2] = {0};

    check(pipe(in_pipefd), "pipe");
    check(pipe(out_pipefd), "pipe");

    pid_t pid = fork();
    if (pid > 0) {
        // parent call parent function
        parent(in_pipefd, out_pipefd, pid);
    } else {
        // child call child function 
        child(in_pipefd, out_pipefd, argv[1]);
    }

    return 0;
}