#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

char* get_process_state(pid_t pid) {
    char state;
    char path[256];
    FILE *fp;

    // build path to the status file location
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    // open file
    fp = fopen(path, "r");
    if (fp == NULL) {
        perror("Failed to open process status file\n");
        return '\0'; // return null for failure
    }

    int skip;
    fscanf(fp, "%d %*s %c", &skip, &state);

    // close state
    fclose(fp);
    if (state == 'R') {
        return "Running";
    } else if (state == 'Z') {
        return "Zombie";
    } else if (state == 'S') {
        return "Sleeping";
    } else {
        return "Other";
    }
}

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
        printf("Parent's Child pid: %d\n", returned_pid);
    }
}

int proper_proc_create_exit() {
    pid_t pid = fork();
    if (pid == -1) { return errno; }
    if (pid == 0) { // in the child process, wait for 2 sec then exit 
        printf("Process %d is waiting for 1 seconds\n", getpid());
        sleep(1); 
        exit(42);
    } else { // in the parent process
        printf("Parent pid: %d\n", getpid());
        printf("Child pid: %d\n", pid);
        printf("Calling wait\n");
        // wait_pid stores the pid of the 1st child process that terminates if multiple children 
        // wstatus stores the exit status i.e. val returned for child process when exited
        int wstatus;
        pid_t wait_pid = wait(&wstatus); // wait for child to exit
        if (WIFEXITED(wstatus)) { // WIFEXITED(.) check if 1st child process exits returns bool
            printf("Child of pid %d exited with exit status %d\n", wait_pid, WEXITSTATUS(wstatus));
        } else {
            return ECHILD; 
        }
    }
    return 0;
}

int zombie_proc_create_exit() {
    pid_t pid = fork();
    if (pid == -1) { return errno; }
    if (pid == 0) {
        sleep(2);
    } else {
        sleep(1);
        printf("Child process status after 1 sec: %s\n", get_process_state(pid));
        sleep(2);
        printf("Child process status after 3 sec: %s\n", get_process_state(pid));
    }
    return 0;
}

int main() {
    // proper_proc_create();
    // proper_proc_create_exit();
    zombie_proc_create_exit();
    return 0;
}