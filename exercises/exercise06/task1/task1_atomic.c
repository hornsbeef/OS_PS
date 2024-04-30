
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

#define THREAD_COUNT 1000
#define THREAD_ITERATIONS 25000


//global vars: :
atomic_int counter = 0;     //TODO: understand atomic!!
pthread_t tid[THREAD_COUNT];


//functions:
void pthread_error_funct(int pthread_returnValue);
void *pthreadStartRoutine();

int main(){

    //create 1000pthreads
    int error;

    for(int i = 0; i<THREAD_COUNT; i++){
        error = pthread_create(&tid[i], NULL, &pthreadStartRoutine, NULL);
        pthread_error_funct(error);
    }

    //wait for 1000 pthreads
    for(int i = 0; i<THREAD_COUNT; i++){
        pthread_join(tid[i], NULL);
    }






}

//Helper functs
void pthread_error_funct(int pthread_returnValue) {
    if(pthread_returnValue != 0){
        char* error_msg = strerror(pthread_returnValue);
        fprintf(stderr, "Error code: %d\n"
                        "Error message: %s\n"
                        "Note that the pthreads functions do not set errno.\n",
                pthread_returnValue, error_msg);
        exit(EXIT_FAILURE);
    }
}

void *pthreadStartRoutine() {

    for(int i = 0; i<THREAD_ITERATIONS; i++){

        //maybe not...??atomics -> problem how to fetch+compute+write atomically ?
        /*
         * Each thread should execute a loop of 25000 iterations. In each iteration i, the value of counter is
        incremented by 42, if i is even, or
        decremented by 41, if i is odd.
         */
        if(i % 2 == 0){

        }else{

        }


    }


    pthread_exit(NULL);
    //return NULL;
}
