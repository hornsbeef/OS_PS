#define DEBUG 1

#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <pthread.h>
#include "myqueue.h"


pthread_mutex_t mutex_queue;
//bool empty;         //todo: remove unnecessary one.
//int data_available;
//both not necessary because we have the myqueue_is_empty() !!
typedef struct pthread_args {
    myqueue* queue;
    unsigned long long current_tid;
    int sum;
}pthread_args;



void check_argc(int argc);

unsigned long long int cast_to_ulli_with_check(char *string);

void pthread_error_funct(int pthread_returnValue);

void *pthreadStartRoutine(void *arg);

int main(int argc, char *argv[]) {
    check_argc(argc);

    unsigned long long num_consumers = cast_to_ulli_with_check(argv[1]);
    unsigned long long num_elements = cast_to_ulli_with_check(argv[2]);
#if DEBUG
    fprintf(stderr, "consumers: %llu\n", num_consumers);
    fprintf(stderr, "elements: %llu\n", num_elements);
#endif


    myqueue queue;
    myqueue_init(&queue);
#if DEBUG
    fprintf(stderr, "queue is empty: %s ", (myqueue_is_empty(&queue) ? "true" : "false"));
#endif


    int mutex_init_retVal = pthread_mutex_init(&mutex_queue, NULL);   //here error check are possible
    pthread_error_funct(mutex_init_retVal);

    pthread_t tid[num_consumers];
    int error;
    for (unsigned long long i = 0; i < num_consumers; i++) {
        pthread_args my_pthread_args;       //todo: check if logic here cornetto
        my_pthread_args.queue = &queue;
        my_pthread_args.current_tid = i;
        error = pthread_create(&tid[i], NULL, &pthreadStartRoutine, &my_pthread_args);
        pthread_error_funct(error);
    }

//this is main:
    //todo: The main thread acts as the producer.
    // After spawning the c consumers, it feeds n entries into the queue,
    // alternating between 1 and -1 (starting with 1),
    // followed by c entries of value INT_MAX.

    for (unsigned long long i = 0; i < num_elements; i++) {
        int mutex_return = pthread_mutex_trylock(&mutex_queue);
        if(mutex_return != 0){  //mutex was already locked.
            i--;            //todo: check if logic cornetto -> if final sum is incorrect error probably here.
            continue;       //todo: check if logic cornetto
        }
        //mutex acquiring successful:
        myqueue_push(&queue, (i % 2 == 0 ? 1 : -1));      //todo: check if logic cornetto
        //data_available++;
        pthread_mutex_unlock(&mutex_queue);
    }
    for (unsigned long long i = 0; i < num_consumers; i++) {
        int mutex_return = pthread_mutex_trylock(&mutex_queue);
        if(mutex_return != 0){  //mutex was already locked.
            i--;            //todo: check if logic cornetto -> if final sum is incorrect error probably here.
            continue;       //todo: check if logic cornetto
        }
        myqueue_push(&queue, INT_MAX);
        //data_available++;
        pthread_mutex_unlock(&mutex_queue);
    }






}

//helper functs
void check_argc(int argc) {
    if (argc != 3) {
        printf("usage: ."__FILE__" < 2  ints>");
        exit(EXIT_FAILURE);
    }
}

unsigned long long int cast_to_ulli_with_check(char *string) {
    errno = 0;
    char *end = NULL;
    unsigned long long operand = strtoull(string, &end, 10);
    //check conversion:
    if ((*end != '\0') || (string == end)) {       //conversion interrupted || no conversion happened
        fprintf(stderr, "Conversion of argument ended with error.\n");
        if (errno != 0) {
            perror("StrToULL");
        }
        exit(EXIT_FAILURE);
    }
    if (errno != 0) {        //== ERANGE //as alternative to != 0
        perror("Conversion of argument ended with error");
        fprintf(stderr, "Please recheck usage!\n");
        exit(EXIT_FAILURE);
    }
    return operand;
}

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

void *pthreadStartRoutine(void *arg) {
    //TODO: When a consumer thread successfully reads an element,
    // it adds it to its local sum.
    // When the element is INT_MAX (limits.h), it prints out the sum, returns it to the main thread and exits.

    pthread_args* my_pthread_args_ptr = (pthread_args*) arg;

    my_pthread_args_ptr->sum = 0;

    //todo: implement waiting function! ? busy waiting?
    infinity_loop:
    while (!myqueue_is_empty(my_pthread_args_ptr->queue)) {
        int mutex_return = pthread_mutex_trylock(&mutex_queue);
        if(mutex_return != 0){  //mutex was already locked.
            continue;       //todo: check if logic cornetto
        }
        int temp = myqueue_pop(my_pthread_args_ptr->queue);
        if(temp == INT_MAX){
            printf("Consumer %llu sum: %d", my_pthread_args_ptr->current_tid, my_pthread_args_ptr->sum);
            pthread_exit(NULL);
        }
        my_pthread_args_ptr->sum += temp;

    }
    goto infinity_loop;
    //should not get here??

}