#define  _GNU_SOURCE    //needed for getline:97 to not throw warning: implicit declaration of function ‘getline’ [-Wimplicit-function-declaratio]

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>


#define MAX_FILENAME_LENGTH 256

enum error_codes {
    invalid_number_of_arguments = 13,
    argument_not_a_number = 14,
    over_or_underflow = 16,
    fopen_failed = 19,
};
//mutex standard practice is a global variable...
//pthread_mutex_t lock;
//re-evaluated code -> mutex not needed


void check_argument_count(int argc) {
    if (argc < 2) {
        fprintf(stderr, "usage: ."__FILE__" <path_to_file> \n");
        exit(invalid_number_of_arguments);
    }
}

long cast_to_int_upto_newline(char *string) {
    errno = 0;
    char *end = NULL;
    long operand = strtol(string, &end, 10);
    //check conversion:
    if (*end != '\n') {
        if ((*end != '\0') || (string == end)) {       //conversion interrupted || no conversion happened
            fprintf(stderr, "an argument was not a number!\n");
            fprintf(stderr, "usage: ."__FILE__" <number of files> \n");
            exit(argument_not_a_number);
        }
    }
    if (errno != 0) {        //== ERANGE //as alternative to != 0
        perror("Conversion of argument ended with error");
        fprintf(stderr, "usage: ."__FILE__" <number of files> \n");
        exit(over_or_underflow);
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


// Structure to hold thread data
typedef struct {
    int threadNum;
    long sum;
    char string[MAX_FILENAME_LENGTH];
    pthread_mutex_t lock;
} ThreadData;


FILE *open_file(char *filename) {
    errno = 0;
    FILE *file = fopen(filename, "r");
    if (file == NULL) {   //check ob open funktioniert hat.
        fprintf(stderr, "The following file could not be opened: %s", filename);
        perror("\n");
        exit(fopen_failed);
    }
    return file;
}


long file_reader_and_sum(FILE *file) {
    long sum = 0;   //starting value

    errno = 0;
    char *lineptr = NULL;
    size_t buffer = 0;
    while (getline(&lineptr, &buffer, file) != -1) {    //returns -1 on failure to read a line, sets errno
        if (errno != 0) {
            perror("getline failed");
            free(lineptr);
            pthread_exit(NULL);
        }
        long temp = cast_to_int_upto_newline(lineptr);
        sum += temp;
    }
    free(lineptr);
    return sum;
}

void *pthreadStartRoutine(void *arg) {

    ThreadData *threadData = arg;

    FILE *file = open_file(threadData->string);
    long sum = file_reader_and_sum(file);

    threadData->sum = sum;

    fclose(file);
    pthread_exit(NULL);
    //i have chosen to return the value via the array,
    // but it would be possible to pass the sum as the retval via pthread_exit(sum)
    // and put it in an array in the main()
}


void create_number_pthreads_with_checker(int number, ThreadData *threadData, pthread_t *tid, char **argv) {
    int error;

    for (int i = 1; i <= number; i++) {

        threadData[i].threadNum = i;
        strcpy(threadData[i].string, argv[i]);

        error = pthread_create(&tid[i], NULL, &pthreadStartRoutine, (void *) &threadData[i]);
        pthread_error_funct(error);

    }

}

void print_individual_sums(int number, ThreadData *threadData) {
    for (int x = 1; x <= number; x++) {
        printf("sum %d = %ld\n", x, threadData[x].sum);
    }
}

long total_sum_funct(int number, ThreadData *threadData) {
    long total_sum = 0;
    for (int x = 1; x <= number; x++) {
        total_sum += threadData[x].sum;
    }
    return total_sum;
}

int main(int argc, char *argv[]) {
    check_argument_count(argc);

    int number = argc - 1;

    errno = 0;
    ThreadData threadData[number + 1];
    pthread_t tid[number + 1];

    create_number_pthreads_with_checker(number, threadData, tid, argv);


    for (int x = 1; x <= number; x++) {
        pthread_join(tid[x], NULL);
    }


    print_individual_sums(number, threadData);

    long total_sum = total_sum_funct(number, threadData);
    printf("total sum = %ld\n", total_sum);

}
/*
Investigate how you can pass multiple arguments to a thread function,
 as well as how to receive a result from it.
 The program must not make use of any global variables.
A Struct is a way to pass multiple arguments to a thread function.
 The same way one can receive a result form it.
 Or one can make use of the argument passed by the pthread_exit() and received by
 the pthread_join().

 */

