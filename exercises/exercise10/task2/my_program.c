#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE
// #define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "some_library.c"

void check_argc(int argc) {
    if (argc != 2) {
        printf("usage: ."__FILE__" <number>");
        exit(EXIT_FAILURE);
    }
}

long cast_to_long_with_check(char *string) {
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



int main(int argc, char *argv[]) {
    check_argc(argc);
    long n = cast_to_long_with_check(argv[1]);

    printf("%d\n", fibonacci_of_integer(n));

}