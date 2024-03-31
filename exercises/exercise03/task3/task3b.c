#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>


enum error_codes {
    invalid_number_of_arguments = 13,
    argument_not_a_number = 14,
    argument_too_small = 15,
    over_or_underflow = 16,
    operator_unknown = 42,

};



void check_argument_count(int argc) {
    if (argc < 2) {
        fprintf(stderr, "usage: ."__FILE__" <path_to_file> \n");
        exit(invalid_number_of_arguments);
    }
}

long cast_to_int_with_check(char* string) {
    errno = 0;
    char *end = NULL;
    long operand = strtol(string, &end, 10);
    //check conversion:
    if ((*end != '\0') || (string == end)) {       //conversion interrupted || no conversion happened
        fprintf(stderr, "operand not a number!\n");
        fprintf(stderr, "usage: ."__FILE__" <number of files> \n");
        exit(argument_not_a_number);
    }

    if (errno != 0) {        //== ERANGE //as alternative to != 0
        //printf("Overflow or underflow occurred.");
        perror("Conversion of argument ended with error");
        fprintf(stderr, "usage: ."__FILE__" <number of files> \n");
        exit(over_or_underflow);
    }
    return operand;
}

void argument_too_small_checker(long number) {
    if(number <= 0){
        fprintf(stderr, "argument too small!\n");
        fprintf(stderr, "usage: ."__FILE__" <number of files> \n");
        exit(argument_too_small);
    }
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

void* pthreadStartRoutine(void* void_ptr){



    pthread_exit(NULL);
}


pthread_t* tid;

int main(int argc, char* argv[]){
    check_argument_count(argc);

    //The program creates N threads,
    // each of which is assigned an ID i, with 0 < i <= N.
    // -> via array
    int number = argc - 1;

    //hacky way to make tid-array available globally
    pthread_t main_tid[number+1]; //todo: find way to make this available globally to all threads
    tid = main_tid; //TODO: HOW TO BETTER?!

    int error;
    for(int i = 1; i <= number; i++){
        error = pthread_create(&tid[i], NULL, &pthreadStartRoutine, NULL);
        pthread_error_funct(error);
    }

//    //debug/test:
//    main_tid[number - 1] = 12345;
//    printf("test: %lu", tid[0]);
//    //end


}