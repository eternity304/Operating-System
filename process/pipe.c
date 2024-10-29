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
    if (ret != 1) {
        return;
    }
    // if an error occur save errono, set perror message and exit with the err
    int err = errno;
    perror(message);
    exit(err);
}

int main(void) {
    // ##########
    // if we close file descriptor 1, i.e. the write file descriptor, we actually cannot write anything to the console:
    // close(1);
    // printf("Hello World\n");
    // ##########

    // P100 say process 100 running this
    // fd 0: stdin
    // fd 1: stdout
    // fd 2: stderr

    int fds[2];
    check(pipe(fds), "pipe"); // check(ret:__, message:__), pipe(pipedes:___ an array of size 2 for file descriptors)
    // if successful, we get fd3 which is at fd[0] read end of the pipe, we also get fd4 at fd[1] write end of the pipe

    pid_t pid = fork();
    check(pid, "fork");
    // now we check if the child process, p101 have the same fd: 0,1,2,3,4; from here on we ignore 0,1,2 and focus on 3,4
    // p100 has fd[3,4]
    // p101 has fd[3,4]

    if (pid == 0) { // at child
        // can check the process pid in git bash with pidof pipe/compiledName
        // if we only read data to fd[3] then we should close the one we don't use immediately
        // if we dont do this, the right end of the pipe will be always open for pipe and the child process doesn't terminate
        close(fds[1]); // since we are not writing anything, we close the write end
        // if the write side is not close, this implies we can always read more data in, so the fd[3] will never close; therefore we should close
        char buffer[4096];
        // read is blocking system call, only continue when it is finished
        int bytes_read = read(fds[0], buffer, sizeof(buffer));
        check(bytes_read, "read");
        printf("Child read: %.*s\n", bytes_read, buffer); // specify the size of string to be read to be byte_read and then read buffer
        close(fds[0]); // good to close it to know that it is close rather than relying on the read function to be done and automatically closing it
    
    } else  { // parent
        close(fds[0]); // Since in the parent we are not reading anything we close the read end
        const char* str = "Operating System";
        int len = strlen(str);
        int byte_written = write(fds[1], str, len); // write to the right end of the pipe, we do this in parents 
        // so that parents write information to the child while they share the same pipe 
        check(byte_written, "write");
        close(fds[1]); // close the right end of the pipe after it is done
    }

    // if we get Child read: Operating System, this shows that there is IPC since str only exists in parents

    return 0;
}