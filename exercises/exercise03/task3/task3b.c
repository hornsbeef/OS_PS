#define  _GNU_SOURCE    //needed for getline:97 to not throw warning: implicit declaration of function ‘getline’ [-Wimplicit-function-declaratio]

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#define MAX_FILENAME_LENGTH 256
#define BUFFERSIZE_BYTES 100

enum error_codes {
    invalid_number_of_arguments = 13,
    argument_not_a_number = 14,
    argument_too_small = 15,
    over_or_underflow = 16,
    operator_unknown = 17,
    readdir_failed = 18,
    fopen_failed = 19,
    getline_failed = 20,
    malloc_fail = 21,

};
pthread_mutex_t lock;


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
            fprintf(stderr, "operand not a number!\n");
            fprintf(stderr, "usage: ."__FILE__" <number of files> \n");
            exit(argument_not_a_number);
        }
    }
    if (errno != 0) {        //== ERANGE //as alternative to != 0
        //printf("Overflow or underflow occurred.");
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
    // Add any other necessary data here
    long sum;
} ThreadData;

pthread_t *tid;
ThreadData *threadData;
char **global_argv;

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
    long sum = 0;

    errno = 0;
    char *lineptr = NULL;
    size_t buffer = 0;
    while (getline(&lineptr, &buffer, file) != -1) {
        if (errno != 0) {
            perror("getline failed");
            //todo: how to error handle in thread with allocaded memory?
            free(lineptr);
            pthread_exit(NULL);
        }
        long temp = cast_to_int_upto_newline(lineptr);
        //printf("%ld\n", temp);
        sum += temp;
    }
    free(lineptr);
    return sum;
}

void *pthreadStartRoutine(void *arg) {
    //ThreadData* data = (ThreadData* )arg;
    int current_threadNum = ((ThreadData *) arg)->threadNum;

    FILE *file = open_file(global_argv[threadData[current_threadNum].threadNum]);
    long sum = file_reader_and_sum(file);

    //TODO: is mutex usage correct here?
    pthread_mutex_lock(&lock);
    threadData[current_threadNum].sum = sum;
    pthread_mutex_unlock(&lock);

    fclose(file);
    pthread_exit(NULL);
}


void create_number_pthreads_with_checker(int number) {
    int error;
    for (int i = 1; i <= number; i++) {

        pthread_mutex_lock(&lock);
        threadData[i].threadNum = i;
        pthread_mutex_unlock(&lock);

        error = pthread_create(&tid[i], NULL, &pthreadStartRoutine, (void *) &threadData[i]);
        pthread_error_funct(error);
    }
}

void print_individual_sums(int number) {
    for (int x = 1; x <= number; x++) {
        //printf("tid%d: %ld\n",x, tid[x]);
        printf("sum %d = %ld\n", x, threadData[x].sum);
    }
}

long total_sum_funct(int number) {
    long total_sum = 0;
    for (int x = 1; x <= number; x++) {
        total_sum += threadData[x].sum;
    }
    return total_sum;
}

int main(int argc, char *argv[]) {
    check_argument_count(argc);

    //The program creates N threads,
    // each of which is assigned an ID runner_1, with 0 < runner_1 <= N.
    // -> via array
    int number = argc - 1;

    errno = 0;
    threadData = malloc((number + 1) * sizeof(ThreadData));
    if (threadData == NULL) {
        perror("Malloc failed");
        exit(malloc_fail);
    }

    tid = malloc((number + 1) * sizeof(pthread_t));
    if (tid == NULL) {
        free(threadData);
        perror("Malloc failed");
        exit(malloc_fail);
    }

    global_argv = argv;

    create_number_pthreads_with_checker(number);

    for (int x = 1; x <= number; x++) {
        pthread_join(tid[x], NULL);
    }


    print_individual_sums(number);

    long total_sum = total_sum_funct(number);
    printf("total sum = %ld\n", total_sum);


    free(tid);
    free(threadData);

}







//other checking approach:
/*

    //check if all files are valid: exit on invalid
    DIR* dir = opendir(".");
    struct dirent* dirent_struct;
    if(dir == NULL){
        perror("opening working directory failed");
        exit(operator_unknown);
    }
    errno = 0;
    int number_of_files_in_dir = 0;
    while ( ((dirent_struct = readdir(dir)) != NULL) && (errno = 0) ){
        number_of_files_in_dir++;
    }
    errno_check(readdir_failed);


    //: array for all file-names in directory:
    char all_dir_file_names[number_of_files_in_dir][MAX_FILENAME_LENGTH];
    int runner_1 = 0;
    errno = 0;
    while ((dirent_struct = readdir(dir)) != NULL){
        strcpy(all_dir_file_names[runner_1], dirent_struct->d_name);
        runner_1++;
    }
    errno_check(readdir_failed);

    //todo: check if all argv[1-...] are in the all_dir_file_names[]
    for(int runner_2 = 0; runner_2 < runner_1; runner_2++){
        //printf("%s\n", all_dir_file_names[runner_2]);

        //TODO: DEBUG:
        for (int x = 1; x < argc ; x++){
            printf("%s\n", argv[x]);
        }



    }

 */
