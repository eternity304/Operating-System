#include <pthread.h>
#include <stdio.h>

void *run(__attribute__((unused)) void* arg) {
    printf("In run\n");
    return NULL; // return NULL is equivalent to calling pthread_exit(NULL) terminating the thread
}

int main(void) {
    pthread_t thread;
    pthread_create(&thread, NULL, &run, NULL);
    // arg1, thread, create a handle to a thread at pointer location
    // arg2, attr, thread attribute (NULL by default)
    // arg3, start_routine, takes in a pointer to a function, and returns a pointer 
    // arg4, the arg we want to pass into the start routine argument
    // return 0 on success and error otherwise
    // NOTE: Since threads and main runs concurrently, there is a chance, that the main
    // run all the way to return 0 and the threads haven't run yet
    // i.e. we will only get  "In main\n" printed since on exit of main all threads are killed
    
    // To Addess This: we wait on it and use pthread_join
    pthread_join(thread, NULL);
    // We wait till the thread is done working
    // arg1, ptr to thread that needs to be waited till termination 
    // arg2, ptr to a ptr that writes the pointer return value to this memory location
    // which stores the exit status of the thread. If we put NULL, won't get the return value
    
    printf("In main\n");

    // Normal threads needs to be acknowledged when exited so they can handle return value and other things
    // If we don't want any of the above, we can actually run detcahed thread that release their resource rightaway when terminated
    // We create a thread like normal and call detach on it 
    pthread_t dthread;
    pthread_create(&dthread, NULL, &run, NULL);
    pthread_detach(dthread); // But we still need to wait on it but we can't join it
    // To solve this, we manually call exit on the main thread
    
    pthread_exit(NULL);
    // when main thread dies, other threads can still exit
    // when only 1 threads remain, the process exits
    return 0;
}