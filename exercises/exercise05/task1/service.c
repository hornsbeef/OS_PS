#include <sys/stat.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#define NUM_TO_SORT 5
#define BUFFERSIZE 30

typedef struct numbers{
    int number_arr[NUM_TO_SORT];
}numbers;




void check_argc(int argc);
long cast_to_int_with_check(char* string);
long cast_to_int_upto_newline(char *string);


int main(int argc, char *argv[]) {
    check_argc(argc);

    char* mq_name = argv[1];
    long priority = cast_to_int_with_check(argv[2]);

    numbers n;

    char buffer[BUFFERSIZE];
    for(int i = 0; i< NUM_TO_SORT; i++){
        printf("Please enter number[%d]: ", (i+1));
        fgets(buffer, BUFFERSIZE, stdin);
        n.number_arr[i] = cast_to_int_upto_newline(buffer);
    }


    const mqd_t mq = mq_open(mq_name, O_WRONLY);    //todo: check if works with only 2 args
    if (mq < 0)
    {
        perror("failed to open message queue");
        return EXIT_FAILURE;
    }

    int send_ret = mq_send(mq, (const char*)&n, sizeof(numbers), priority);
    if (send_ret != 0){
        perror("mq_send");
        mq_close(mq);
    }

    mq_close(mq);

    return EXIT_SUCCESS;




}

void check_argc(int argc) {
    if (argc != 3) {
        fprintf(stderr, "usage: ."__FILE__ " <msg_queue_name> <priority>\n");
        exit(EXIT_FAILURE);
    }
}

long cast_to_int_with_check(char* string) {
    errno = 0;
    char *end = NULL;
    long operand = strtol(string, &end, 10);
    //check conversion:
    if ((*end != '\0') || (string == end)) {       //conversion interrupted || no conversion happened
        fprintf(stderr, "operand not a number!\n");
        exit(EXIT_FAILURE);
    }

    if (errno != 0) {        //== ERANGE //as alternative to != 0
        //printf("Overflow or underflow occurred.");
        perror("Conversion of argument ended with error");
        exit(EXIT_FAILURE);
    }
    return operand;
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
            exit(EXIT_FAILURE);
        }
    }
    if (errno != 0) {        //== ERANGE //as alternative to != 0
        //printf("Overflow or underflow occurred.");
        perror("Conversion of argument ended with error");
        fprintf(stderr, "usage: ."__FILE__" <number of files> \n");
        exit(EXIT_FAILURE);
    }

    return operand;
}

