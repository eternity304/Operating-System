#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

void* run(void* arg) {
    int id = *((int *)arg); // cast as int pointer and dereference
    // this is the code the thread executes so the int id is allocated on the stack
    free(arg);
    for (int i = 0; i < 10; i++) {
        printf("Thread %d: %d\n", id, i);
        usleep(1000);
    }
    return NULL;
}

int new_thread(int id) {
    // here we pass data to a new thread by malloc
    // before threads created, its stack space does not exist yet, s
    // so we need to use some other for of memory like the heap to pass memory to it 
    // 
    int * arg = (int *)malloc(sizeof(int));
    *arg = id;
    pthread_t thread;
    int rc = pthread_create(&thread, NULL, &run, arg);
    pthread_detach(thread);
    return rc;   
}

int main() {
    for (int i = 1; i <= 4; i++) {
        int err = new_thread(i);
        if (err != 0) {
            return err;
        }
    }
    pthread_exit(0);
    return 0;
}