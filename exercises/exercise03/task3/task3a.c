#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

//Write a program that initializes a global variable accumulation of type int64_t and initializes it to 0.
int64_t accumulation = 0;

int main(){

    //The program begins by printing the value of accumulation and then creates a child process using fork.
    printf("1: accumulation = %ld\n", accumulation);
    pid_t pid = fork();

    switch (pid) {
        case -1:
            perror("fork failed");
            exit (EXIT_FAILURE);
        case 0:
            //this is child
            //The child accumulates the values from 1 to N = 10 into accumulation and exits.

            for(int i = 1; i<=10; i++ ){
                accumulation += i;
            }
            //TODO:DEBUG
            //printf("|DEBUG: %ld|\n", accumulation);

            exit(EXIT_SUCCESS);
            break;

        default:
            //this is parent
            wait(NULL);
            printf("2: accumulation = %ld\n", accumulation);
    }

    return EXIT_SUCCESS;

}
