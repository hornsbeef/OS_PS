#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

//Write a program that initializes a global variable accumulation of type int64_t and initializes it to 0.
int64_t accumulation = 0;

int64_t accumulate(int64_t global_acc) {
    //The child accumulates the values from 1 to N = 10 into accumulation and exits.
    for(int i = 1; i <= 10; i++ ){
        global_acc += i;
    }
    return global_acc;
}

void* pthreadStartRoutine(void* void_ptr){
    //The thread does the same thing as the child process, i.e. accumulate the values from 1to N = 10 into the global variable and immediately exit.
    accumulation = accumulate(accumulation);
    pthread_exit(NULL);
    return NULL;
}

void pthread_error_funct(int pthread_returnValue) {
    if(pthread_returnValue != 0){
        fprintf(stderr, "pthread creation failed! Error code: %d"
                        "\nNote that the pthreads functions do not set errno.",
                        pthread_returnValue);
        exit(EXIT_FAILURE);
    }
}

int main(){

    //The program begins by printing the value of accumulation and then creates a child process using fork.
    printf("1: accumulation = %ld\n", accumulation);
    pid_t pid = fork();

    switch (pid) {
        case -1:
            perror("fork failed");
            exit (EXIT_FAILURE);
        case 0:
            //this is child-Process
            accumulation = accumulate(accumulation);
            exit(EXIT_SUCCESS);

        default:
            //this is parent
            //The child accumulates the values from 1 to N = 10 into accumulation and exits. The parent waits for the child to exit and prints the value again.
            wait(NULL);
            printf("2: accumulation = %ld\n", accumulation);

            //Next, the program (now referred to as the main thread) spawns a single POSIX thread.
            pthread_t pthreadID = 0;
            int pthread_create_retVal = pthread_create(&pthreadID, NULL, pthreadStartRoutine, &accumulation);
            pthread_error_funct(pthread_create_retVal);

            //The main thread waits for the thread to finish, and prints the value one more time.
            int pthread_join_retVal = pthread_join(pthreadID, NULL);    //todo: testing with NULL -> else check manpage
            pthread_error_funct(pthread_join_retVal);
            printf("3: accumulation = %ld\n", accumulation);

    }

    return EXIT_SUCCESS;

}
