#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

//todo: testing -> change to 1 before commit
#define AMOUNT_THREADS 1


//Write a program that initializes a global variable accumulation of type int64_t and initializes it to 0.
int64_t accumulation = 0;


//for mutex locking:
pthread_mutex_t lock;

//pthread ID array:
pthread_t tid[AMOUNT_THREADS];



int64_t accumulate_fkt() {
    int64_t acc = 0;
    //The child accumulates the values from 1 to N = 10 into accumulation and exits.
    for(int i = 1; i <= 10; i++ ){
        acc += i;
        //testing -> remove before commit
        //printf("%lu: %ld %ld\n",pthread_self(), accumulation, acc);
    }
    return acc;
}

void* pthreadStartRoutine(){
    //The thread does the same thing as the child process, i.e. accumulate_fkt the values from 1to N = 10 into the global variable and immediately exit.

    pthread_mutex_lock(&lock);      //"take key and lock room"

    accumulation = accumulate_fkt();     //"do stuff in the room"
    //testing mutex:
    //accumulation += accumulate_fkt();

    pthread_mutex_unlock(&lock);    //"unlock room; exit room; put key back"

    pthread_exit(NULL);
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

int main(){
    //for mutex locking:
    int mutex_init_retVal = pthread_mutex_init(&lock, NULL);
    pthread_error_funct(mutex_init_retVal);



    //The program begins by printing the value of accumulation and then creates a child process using fork.
    printf("1: accumulation = %ld\n", accumulation);
    pid_t pid = fork();

    switch (pid) {
        case -1:
            perror("fork failed");
            exit (EXIT_FAILURE);
        case 0:
            //this is child-Process
            accumulation = accumulate_fkt();
            //todo: testing
            //accumulate_fkt();

            exit(EXIT_SUCCESS);

        default:
            //this is parent
            //The child accumulates the values from 1 to N = 10 into accumulation and exits. The parent waits for the child to exit and prints the value again.
            wait(NULL);
            printf("2: accumulation = %ld\n", accumulation);

            //Next, the program (now referred to as the main thread) spawns a single POSIX thread.
            //pthread_t pID = 0;    //initialize with 0

            int i = 0;
            int error;
            while(i < AMOUNT_THREADS){
                error = pthread_create(&tid[i], NULL, &pthreadStartRoutine, NULL);
                pthread_error_funct(error);
                i++;
            }
            //int pthread_create_retVal = pthread_create(&pID, NULL, pthreadStartRoutine, &accumulation);




            //The main thread waits for the thread to finish, and prints the value one more time.
            int j = 0;
            while(j < AMOUNT_THREADS){
                //int pthread_join_retVal =
                pthread_join(tid[j], NULL);    //todo: testing with NULL -> else check manpage
                //thread_error_funct(pthread_join_retVal);
                j++;
            }

            printf("3: accumulation = %ld\n", accumulation);

    }

    //mutex cleanup
    pthread_mutex_destroy(&lock);

    return EXIT_SUCCESS;

}

//What do you notice? Explain the behavior.
//the accumulation with the child-PROCESS results in 0 in the global variable, because the value is saved to
//the variable in the child Process not the parent process.
// the child thread on the other hand can access the global variable "accumulation" -> therefore changes the value there
// -> the parent process.

//Make sure to check for possible errors that might occur in the program.
// -> looks for error in fork and pthread_functions.
// Look at return codes of functions like pthread_create and print meaningful error messages to standard error using fprintf