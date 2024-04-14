#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

typedef struct client {
    char name[256];
    int counter;
    int file_descriptor;
    bool is_connected;

} client_t;
void pipe_error_check(int pipe_error);

void close_all_pipes(int argc);

int main(int argc, char* argv[]) {
    //close_all_pipes(argc);    //for debugging to close pipes

    if (argc < 2) {
        fprintf(stderr, "usage: ."__FILE__ " <client names>");
        exit(EXIT_FAILURE);
    }

    client_t clients[argc];
    mode_t client_server_fifo_perm = (S_IRUSR | S_IWUSR);

    //creating all the clients data
    for (int i = 1; i < argc; ++i) {
        strcpy(clients[i].name, argv[i]);
        clients[i].counter = 0;
        clients[i].is_connected = false;
        //fprintf(stderr, "%s", clients[i].name);

        errno = 0;
        int fifo_error = mkfifo(clients[i].name, client_server_fifo_perm);
        pipe_error_check(fifo_error);
        fprintf(stderr, "%s : makefifo error code = %d\n", clients[i].name, fifo_error);

        errno = 0;
        clients[i].file_descriptor = open(clients[i].name, O_RDONLY | O_NONBLOCK);  //maybe O_NONBLOCK is stupid here...
        fprintf(stderr, "%s : file_descriptor = %d\n", clients[i].name, clients[i].file_descriptor);
        if(clients[i].file_descriptor < 0){
            perror("Open failed");
        }

    }




    //todo: check if clients have connected: maybe use access(fifoPath, F_OK)

    int all_clients_connected = 0;
    while(all_clients_connected != argc){
        for (int i = 0; i < argc; ++i) {
            if(clients[i].is_connected){
                continue;
            }else
            if(access(clients[i].name, F_OK) == 0){
                all_clients_connected++;
                clients[i].is_connected = true;
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
    //todo: DEBUG
    fprintf(stderr, "this should not be printed until all clients have been connected");



    usleep(100000);

    //todo: close all the pipes again:
    close_all_pipes(argc);
    return EXIT_SUCCESS;
}

void close_all_pipes(int argc) {
    for (int all_the_pipes = 0; all_the_pipes < argc; ++all_the_pipes) {
        close(all_the_pipes);
    }
}

void pipe_error_check(int pipe_error) {
    if(pipe_error < 0){
        //todo: set errno = 0 before pipe() call!
        perror("Pipe failed");
        exit(EXIT_FAILURE);
    }
}