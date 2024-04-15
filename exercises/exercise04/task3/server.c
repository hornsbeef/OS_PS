#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <poll.h>
#include <linux/limits.h>

typedef struct client {
    char name[256];
    int counter;
    int file_descriptor;
    bool is_connected;

} client_t;
void pipe_error_check(int pipe_error);

void close_all_pipes(int argc);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: ."__FILE__ " <client names>");
        exit(EXIT_FAILURE);
    }

    client_t clients[argc];
    mode_t client_server_fifo_perm = (S_IRUSR | S_IWUSR);
    struct pollfd fds[argc-1];    //for poll()

    //creating all the clients data
    for (int i = 1; i < argc; ++i) {
        strcpy(clients[i].name, argv[i]);
        clients[i].counter = 0;
        clients[i].is_connected = false;
        //fprintf(stderr, "%s", clients[i].name);

        errno = 0;
        int fifo_error = mkfifo(clients[i].name, client_server_fifo_perm);
        pipe_error_check(fifo_error);
        //fprintf(stderr, "%s : makefifo error code = %d\n", clients[i].name, fifo_error);


        //for ease of use without the O_NONBLOCK:
        fprintf(stdout, "Please connect client %s.\n", clients[i].name);
        fflush(stdout);

        errno = 0;
        clients[i].file_descriptor = open(clients[i].name, O_RDONLY);

        if(clients[i].file_descriptor < 0){
            perror("Open failed");
        }
        //unfortunately O_NONBLOCK is not allowed.
        //So the program waits here until the specified client is connected.
        fflush(stdout);
        clients[i].is_connected = true;

        fds[i-1].fd = clients[i].file_descriptor;     //for poll()
        fds[i-1].events = POLLIN;        //polling here interested in: POLLIN:
        fprintf(stdout, "%s connected.\n", clients[i].name);

    }

    //todo: DEBUG
    fprintf(stdout, "all clients have been connected\n");




    char buf[PIPE_BUF];


    int all_clients_disconnected = 0;
    while(all_clients_disconnected != (argc-1)){
        printf("waiting to receive data...\n");
        fflush(stdout);

        //wait indefinitly for events:
        int events = poll(fds, argc-1, -1);

        for (int i = 1; i < argc; ++i) {    //go through all fd to check which one has data available:
            if(fds[i-1].revents & POLLHUP){
                all_clients_disconnected++;
                printf("%s disconnected.", clients[i].name);    //TODO: why is this not printed????
                fflush(stdout);
                //continue; or goto poll ??
            }
            if(fds[i-1].revents & POLLIN) {
                memset(buf, 0, PIPE_BUF);
                read(clients[i].file_descriptor,buf, PIPE_BUF);
                fprintf(stdout, "%s was received\n", buf);
            }


        }
    }


    // close all the pipes again
    for (int all_the_pipes = 0; all_the_pipes < argc; ++all_the_pipes) {
        close(all_the_pipes);
        unlink(clients[all_the_pipes].name);            //<- MUST NOT FORGET!
    }
    return EXIT_SUCCESS;
}





void pipe_error_check(int pipe_error) {
    if(pipe_error < 0){
        //todo: set errno = 0 before pipe() call!
        perror("Pipe failed");
        exit(EXIT_FAILURE);
    }
}


/*  Code for usage with open(fd, O_NONBLOCK) for waiting for clients to connect.

    int all_clients_connected = 0;
    while(all_clients_connected != (argc-1)){
        for (int i = 1; i < argc; ++i) {
            fds[i].events = POLLIN;        //polling here interested in: POLLIN:

            if(clients[i].is_connected){
                continue;
            }else
            if(poll(&fds[i], 1, 1)> 0){     //little workaround with nfds 1 and &fds[i] ... maybe it works
                all_clients_connected++;
                clients[i].is_connected = true;
                fprintf(stdout, "%s connected.\n", clients[i].name);
                fflush(stdout);
                if(all_clients_connected == (argc-1)){
                    goto after_loop;    //goto is not nice, but useful here to break out of 2 loops
                }
            }
        }
        fprintf(stdout, "waiting for all clients to connect");
        fflush(stdout);
        usleep(1300000);
        fprintf(stdout, ".");
        fflush(stdout);
        usleep(1300000);
        fprintf(stdout, ".");
        fflush(stdout);
        usleep(1300000);
        fprintf(stdout, ".\n");
        fflush(stdout);
        usleep(3500000);

    }
    after_loop:    //for goto: to jump out of 2 loops...



 */