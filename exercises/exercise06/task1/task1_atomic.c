#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE
// #define _BSD_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

#define THREAD_COUNT 1000
#define THREAD_ITERATIONS 25000


//global vars: :
atomic_int counter = 0;
//_Atomic int counter = 0;
//int counter = 0;  //interestingly  no difference was observable if I used atomic_int or int -> ?(bad) luck?
pthread_t tid[THREAD_COUNT];


//functions:
void pthread_error_funct(int pthread_returnValue);

void *pthreadStartRoutine();

int main() {

    //create 1000pthreads

    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_error_funct(pthread_create(&tid[i], NULL, &pthreadStartRoutine, NULL));
    }

    //wait for 1000 pthreads
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_error_funct(pthread_join(tid[i], NULL));
    }
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

        if (i % 2 == 0) {
            //is even:
            atomic_fetch_add(&counter, 42);
            //counter += 42;    //testing if race-condition is producible https://stackoverflow.com/questions/1790204/in-c-is-i-1-atomic
        } else {
            //is odd:
            atomic_fetch_sub(&counter, 41);
            //counter -=41;
        }


    }


    pthread_exit(NULL);
    //return NULL;
}
