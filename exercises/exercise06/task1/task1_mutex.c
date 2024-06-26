#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE
// #define _BSD_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define THREAD_COUNT 1000
#define THREAD_ITERATIONS 25000


//global vars: :
int counter = 0;
pthread_t tid[THREAD_COUNT];
pthread_mutex_t mutex_queue; // = PTHREAD_MUTEX_INITIALIZER;  //NO  error checks are performed.

//functions:
void pthread_error_funct(int pthread_returnValue);

void *pthreadStartRoutine();

int main() {

    pthread_error_funct(pthread_mutex_init(&mutex_queue, NULL));   //here error check are possible


    //create 1000pthreads

    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_error_funct(pthread_create(&tid[i], NULL, &pthreadStartRoutine, NULL));
    }

    //wait for 1000 pthreads
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_error_funct(pthread_join(tid[i], NULL));
    }

    //cleanup:
    pthread_mutex_destroy(&mutex_queue);

    printf("Final value of counter = %d\n", counter);

}

//Helper functs
void pthread_error_funct(int pthread_returnValue) {
    if (pthread_returnValue != 0) {
        char *error_msg = strerror(pthread_returnValue);
        fprintf(stderr, "Error code: %d\n"
                        "Error message: %s\n"
                        "Note that the pthreads functions do not set errno.\n",
                pthread_returnValue, error_msg);
        exit(EXIT_FAILURE);
    }
}

void *pthreadStartRoutine() {
/*
* Each thread should execute a loop of 25000 iterations. In each iteration i, the value of counter is
incremented by 42, if i is even, or
decremented by 41, if i is odd.
*/
    for (int i = 0; i < THREAD_ITERATIONS; i++) {

        pthread_mutex_lock(&mutex_queue);
        if (i % 2 == 0) {
            counter += 42;
        } else {
            counter -= 41;
        }
        pthread_mutex_unlock(&mutex_queue);
    }

    pthread_exit(NULL);

}
