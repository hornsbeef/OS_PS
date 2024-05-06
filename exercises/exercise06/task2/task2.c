#define LOCK_TYPE 1
//LOCK_TYPES: mutex:
//1 == trylock
//2 == lock

#define DEBUG 0
//DEBUG:
//1 for additional usage info
//2 for sum queue_init checking
//10 for

#define ASSERT 0
//1 for assertion that final_sum is correct.
//>1 for other Assertions

//todo: volatile trying to understand optimization

#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE
// #define _BSD_SOURCE
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <pthread.h>
#include "myqueue.h"


pthread_mutex_t mutex_queue;

typedef struct pthread_args {
    myqueue* queue;
    unsigned long long consumer_number;
    int sum;
    pthread_t tid;
}pthread_args;



void check_argc(int argc);

unsigned long long int cast_to_ulli_with_check(char *string);

void pthread_error_funct(int pthread_returnValue);

void *pthreadStartRoutine(void *arg);

int main(int argc, char *argv[]) {
    check_argc(argc);

    volatile unsigned long long num_consumers = cast_to_ulli_with_check(argv[1]);    //c -> optimized out
    volatile unsigned long long num_elements = cast_to_ulli_with_check(argv[2]);     //n -> optimized out
#if DEBUG >1
    fprintf(stderr, "consumers: %llu\nelements: %llu\n", num_consumers,num_elements);
#endif

    //form the supplied header:
    myqueue queue;
    myqueue_init(&queue);
#if DEBUG >10
    fprintf(stderr, "queue is empty: %s ", (myqueue_is_empty(&queue) ? "true" : "false"));
#endif



//mutex and mutex_attr init:
    pthread_mutexattr_t mutex_queue_attr;
    pthread_error_funct(pthread_mutexattr_init(&mutex_queue_attr));
    pthread_error_funct(pthread_mutexattr_setpshared(&mutex_queue_attr, PTHREAD_PROCESS_SHARED));
    pthread_error_funct(pthread_mutexattr_settype(&mutex_queue_attr, PTHREAD_MUTEX_ERRORCHECK));

    pthread_error_funct(pthread_mutex_init(&mutex_queue, &mutex_queue_attr));

//pthread creation loop with relevant info
    pthread_args arg_struct_array[num_consumers];
    //volatile int error = 0;
    for (unsigned long long i = 0; i < num_consumers; i++) {
        arg_struct_array[i].queue = &queue;
        arg_struct_array[i].consumer_number = i;
        pthread_error_funct(pthread_create(&(arg_struct_array[i].tid), NULL, &pthreadStartRoutine, &arg_struct_array[i]));
    }

//this is main:
    //The main thread acts as the producer.
    // After spawning the c consumers, it feeds n entries into the queue,
    // alternating between 1 and -1 (starting with 1),
    // followed by c entries of value INT_MAX.

//adding elements
    for (unsigned long long i = 0; i < num_elements; i++) { // num_elements -> optimized out
#if LOCK_TYPE == 1
        int mutex_return = pthread_mutex_trylock(&mutex_queue);
        if(mutex_return != 0){
            //mutex was already locked.
#if DEBUG >1
            fprintf(stderr, "Main thread: @number trylock failed\n");
#endif
            if(mutex_return != EBUSY){  //just checking, if it wasn't EBUSY -> something must have gone wrong...
                pthread_error_funct(mutex_return);
            }

            i--;    //so the counter of the for loop is not increased!
            continue;
        }
        //mutex acquiring successful:
#if DEBUG >1
        fprintf(stderr, "Main thread: @number trylock successful\n");
#endif
#elif LOCK_TYPE == 2
        pthread_error_funct(pthread_mutex_lock(&mutex_queue));
#endif

        myqueue_push(&queue, (i % 2 == 0 ? 1 : -1));
        pthread_mutex_unlock(&mutex_queue);
    }


//adding INT_MAX:s
    for (unsigned long long i = 0; i < num_consumers; i++) {
#if LOCK_TYPE ==1
        int mutex_return = pthread_mutex_trylock(&mutex_queue);
        if(mutex_return != 0){
            //mutex was already locked.
#if DEBUG >1
            fprintf(stderr, "Main thread: @INT_MAX trylock failed\n");
#endif
            if(mutex_return != EBUSY){
                pthread_error_funct(mutex_return);
            }
            i--;
            continue;
        }
        //mutex acquiring was successful.
#if DEBUG >1
        fprintf(stderr, "Main thread: @INTMAX trylock successful\n");
#endif
#elif LOCK_TYPE ==2
        int mutex_return = pthread_mutex_lock(&mutex_queue);
        pthread_error_funct(mutex_return);
#endif
        myqueue_push(&queue, INT_MAX);
        pthread_error_funct(pthread_mutex_unlock(&mutex_queue));
    }

//waiting for threads to finish and join

 //The main thread then waits until all consumers have finished
 // and computes the final sum from all the partial results,
 // prints it to the console and exits.
 // Note that the final sum should be 0 if n is even and 1 if n is odd.

    int finalsum = 0;
    for (unsigned long long i = 0; i < num_consumers; i++) {    //num_consumers -> optimized out
#if DEBUG >1
        fprintf(stderr, "Main thread: @Joining \n");
#endif
        pthread_error_funct(pthread_join(arg_struct_array[i].tid, NULL));
        finalsum += arg_struct_array[i].sum;
    }

#if DEBUG
    fprintf(stdout, "Note that the final sum should be %s.\n", num_elements % 2 == 0 ? " 0 " : " 1 ");
#endif
    printf("Final sum: %d\n", finalsum);

#if ASSERT >= 1
    assert(finalsum == (num_elements % 2 == 0 ? 0:1));
#endif


    pthread_error_funct(pthread_mutex_destroy(&mutex_queue));
    exit(EXIT_SUCCESS);

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
    //When a consumer thread successfully reads an element,
    // it adds it to its local sum.
    // When the element is INT_MAX (limits.h), it prints out the sum, returns it to the main thread and exits.

    pthread_args* my_pthread_args_ptr = (pthread_args*) arg;
#if DEBUG > 2
    fprintf(stderr, "in Thread: %lu\n",my_pthread_args_ptr->tid );
#endif

    my_pthread_args_ptr->sum = 0;

    infinity_loop:
    //while (!myqueue_is_empty(my_pthread_args_ptr->queue)) {   //todo: why is this not working with optimization? (works without optimization)
    while (true) {
#if LOCK_TYPE == 1
        int mutex_return = pthread_mutex_trylock(&mutex_queue);
        if(mutex_return != 0){
            //mutex was already locked.
#if DEBUG >1
            fprintf(stderr, "Consumer: %lld trylock failed\n", my_pthread_args_ptr->consumer_number);
#endif
            if(mutex_return != EBUSY){
                pthread_error_funct(mutex_return);
            }
            continue;
        }
        //got the lock:
#if DEBUG >1
        fprintf(stderr, "Consumer: %lld trylock successful\n", my_pthread_args_ptr->consumer_number);
#endif
#elif LOCK_TYPE == 2
        int mutex_return = pthread_mutex_lock(&mutex_queue);
        pthread_error_funct(mutex_return);


#endif
        if(myqueue_is_empty(my_pthread_args_ptr->queue)){
#if DEBUG >1
            fprintf(stderr, "Consumer: %lld QUEUE EMPTY\n", my_pthread_args_ptr->consumer_number);
#endif
            pthread_mutex_unlock(&mutex_queue);
            continue;
        }

        int temp = myqueue_pop(my_pthread_args_ptr->queue);
        pthread_mutex_unlock(&mutex_queue);

        if(temp == INT_MAX){
            //INT_MAX == Shutdown_signal receive:
#if DEBUG > 1
            fprintf(stderr, "Consumer: %lld received shudown signal\n", my_pthread_args_ptr->consumer_number);
#endif
            printf("Consumer %llu sum: %d\n", my_pthread_args_ptr->consumer_number, my_pthread_args_ptr->sum);
            fflush(stdout);
            pthread_exit(NULL); //this function always succeeds -> no need for error checking
        }

        my_pthread_args_ptr->sum += temp;
#if DEBUG >1
        fprintf(stderr, "Consumer %llu : current sum = %d\n", my_pthread_args_ptr->consumer_number, my_pthread_args_ptr->sum);
#endif

    }
    goto infinity_loop;
    //should not get here??
#if ASSERT == 2
    assert(false)
#endif
}

