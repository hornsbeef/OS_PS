#define DEBUG 1

#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <pthread.h>
#include "myqueue.h"


pthread_mutex_t mutex_counter;


void check_argc(int argc);
unsigned long long int cast_to_ulli_with_check(char *string);
void pthread_error_funct(int pthread_returnValue);
void *pthreadStartRoutine();

int main(int argc, char *argv[]) {
    check_argc(argc);

    unsigned long long num_consumers = cast_to_ulli_with_check(argv[1]);     //N, an arbitrary integer
    unsigned long long num_elements = cast_to_ulli_with_check(argv[2]);
#if DEBUG
    fprintf(stderr, "consumers: %llu\n", num_consumers);
    fprintf(stderr, "elements: %llu\n",num_elements);
#endif

    int mutex_init_retVal = pthread_mutex_init(&mutex_counter, NULL);   //here error check are possible
    pthread_error_funct(mutex_init_retVal);

    pthread_t tid[num_consumers];
    int error;
    for(unsigned long long i = 0; i<num_consumers; i++){
        error = pthread_create(&tid[i], NULL, &pthreadStartRoutine, NULL);
        pthread_error_funct(error);
    }


    //todo: The main thread acts as the producer.
    // After spawning the c consumers, it feeds n entries into the queue,
    // alternating between 1 and -1 (starting with 1),
    // followed by c entries of value INT_MAX.



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
        if(errno != 0){
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
    //TODO: When a consumer thread successfully reads an element,
    // it adds it to its local sum.
    // When the element is INT_MAX (limits.h), it prints out the sum, returns it to the main thread and exits.

}