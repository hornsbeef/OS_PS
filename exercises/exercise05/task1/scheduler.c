#include <sys/stat.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <poll.h>


#define NUM_TO_SORT 5

typedef struct numbers{
    int number_arr[NUM_TO_SORT];
}numbers;

volatile bool shutdown = false;     //friendly idea.
int return_stat = EXIT_SUCCESS;

void signal_handler(int sig){
    (void) sig;
    shutdown = true;
}

void bubbleSort(int arr[], int size);


void check_argc(int argc);

int main(int argc, char* argv[]){
    check_argc(argc);

    const char* mq_name = argv[1];

    //signal handling
    struct sigaction act;
    sigfillset(&act.sa_mask);   //all signals blocked while in signal handler
    act.sa_handler = signal_handler;
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGINT, &act, NULL);


    //todo: create mq_
    const int oflag = O_CREAT | O_EXCL;
    const mode_t permissions =  S_IRUSR | S_IWUSR; // 600
    const struct mq_attr attr = { .mq_maxmsg = 4, .mq_msgsize = sizeof(numbers) };
    const mqd_t temp = mq_open(mq_name, oflag, permissions, &attr);
    if(temp == -1){
        perror("mq_open");
        return_stat= EXIT_FAILURE;
        goto cleanup;
    }
    mq_close(temp);

    //todo: open mq_

    //const mqd_t mq = mq_open(mq_name, O_RDONLY | O_NONBLOCK, 0, NULL);
    const mqd_t mq = mq_open(mq_name, O_RDONLY, 0, NULL);

    struct pollfd fds;
    fds.fd = mq;
    fds.events = POLLIN;


    while(!shutdown){

        numbers n;
        errno = 0;
        int ret = poll(&fds, 1, -1); // Wait indefinitely for a message
        if (ret == -1) {
            if(errno != EINTR){
                perror("poll");
            }
            shutdown = true;
            break;  //todo: check if works
        }

        if (fds.revents & POLLIN) {
            unsigned int prio;
            errno = 0;
            ssize_t bytes_read = mq_receive(mq, (char*)&n, sizeof(numbers), &prio);
            if (bytes_read == -1) {
                perror("mq_receive");
                shutdown = true;
                break;
            }
            printf("Sheduling task with priority %d...\n", prio);
            //todo: process_task(&t);

            bubbleSort(n.number_arr, NUM_TO_SORT); //todo check arguments if doesnt work

            printf("\n");


        }
    }


cleanup:
    printf("\nShutting down.\n");
    close(mq);
    mq_unlink(mq_name);
    exit(return_stat);

}

void check_argc(int argc) {
    if (argc != 2) {
        fprintf(stderr, "usage: ."__FILE__ " <msg_queue_name> \n");
        exit(EXIT_FAILURE);
    }
}

void bubbleSort(int arr[], int size) {  //todo: check if works
    int i, j, temp, count = 0;
    int totalComparisons = size * (size - 1) / 2;   //internet

    for (i = 0; i < size - 1; i++) {
        for (j = 0; j < size - i - 1; j++) {
            count++;
            if (arr[j] > arr[j + 1]) {
                temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
            int percentage = (count * 100) / totalComparisons;
            printf("\rSorting progress: %d%%", percentage);
            fflush(stdout);
            usleep(500*1000);
        }
    }

    printf("\nSorted Numbers:");
    for (size_t x = 0; x < NUM_TO_SORT; ++x)
    {
        printf(" %d", arr[x]);  //todo: check if works
    }
    printf("\n");

}