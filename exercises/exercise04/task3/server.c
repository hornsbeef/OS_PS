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
    long counter;
    int file_descriptor;
    bool is_connected;
    bool expression_malformed;

} client_t;

void pipe_error_check(int pipe_error);

void check_argc_count(int argc);

long cast_to_int_with_check(client_t *client_i, char* buf);


int main(int argc, char* argv[]) {
    check_argc_count(argc);
    //todo: check if fifo exist and delete.

    //general setup:
    client_t clients[argc]; //todo:
    mode_t client_server_fifo_perm = (S_IRUSR | S_IWUSR);
    struct pollfd fds[argc-1];    //for poll()


    //creating all the clients data
    for (int i = 1; i < argc; ++i) {

        strcpy(clients[i].name, argv[i]);
        clients[i].counter = 0;
        clients[i].is_connected = false;

        //create fifo for current client with error checking
        errno = 0;
        int fifo_error = mkfifo(clients[i].name, client_server_fifo_perm);
        pipe_error_check(fifo_error);

        //OPEN FIFO FOR READ:
        //for ease of use without the O_NONBLOCK:
        fprintf(stdout, "Please connect client %s.\n", clients[i].name);
        fflush(stdout);
        //actual opening happening here:
        errno = 0;
        clients[i].file_descriptor = open(clients[i].name, O_RDONLY);
        if(clients[i].file_descriptor < 0){
            perror("Open failed");
        }
        //unfortunately O_NONBLOCK is not allowed.
        //So the program waits here until the specified client is connected.
        fflush(stdout);
        clients[i].is_connected = true;
        //opening procedure ended.

        fds[i-1].fd = clients[i].file_descriptor;     //for poll()
        fds[i-1].events = POLLIN;        //polling here interested in: POLLIN:
        fprintf(stdout, "%s connected.\n", clients[i].name);

    }

    //todo: DEBUG ONLY
    fprintf(stdout, "all clients have been connected\n");




    char buf[PIPE_BUF]; //todo: why is PIPE_BUF important?


    int all_clients_disconnected = 0;
    while(all_clients_disconnected < (argc-1)){
        //todo: debug only
        //printf("waiting to receive data...\n");
        //fflush(stdout);
        //todo: end

        //wait indefinitly for events via poll:
        int events = poll(fds, argc-1, -1);

        //once data is available -> go through all fd to check which one has data available:
        for (int i = 1; i < argc; ++i) {

            //check for connection ended by the client
            if(fds[i-1].revents & POLLHUP){ //seems to work.
                all_clients_disconnected++;
                printf("%s disconnected.", clients[i].name);
                fflush(stdout);

            }
            //check if data to read is available -> save to buf.
            if(fds[i-1].revents & POLLIN) {
                memset(buf, 0, PIPE_BUF);
                read(clients[i].file_descriptor,buf, PIPE_BUF);
                //fprintf(stdout, "%s was received\n", buf);    //todo: DEBUG ONLY

                //todo: try to convert to long -> check both cases: number | asdf

                clients[i].counter = cast_to_int_with_check(&clients[i], buf);
                if(clients[i].expression_malformed == false){
                    printf("%s: counter = %ld.\n", clients[i].name, clients[i].counter);
                }

            }
        }

    }


    // close & UNLINK!!! all the pipes again
    for (int all_the_pipes = 0; all_the_pipes < argc; ++all_the_pipes) {
        close(all_the_pipes);
        unlink(clients[all_the_pipes].name);            //<- MUST NOT FORGET!
    }
    return EXIT_SUCCESS;
}

//Helper functions below here:

void check_argc_count(int argc) {
    if (argc < 2) {
        fprintf(stderr, "usage: ."__FILE__ " <client names>");
        exit(EXIT_FAILURE);
    }
}


void pipe_error_check(int pipe_error) {
    if(pipe_error < 0){
        //todo: set errno = 0 before pipe() call!
        perror("Pipe failed");
        exit(EXIT_FAILURE);
    }
}


long cast_to_int_with_check(client_t *client_i, char* buf) {
    client_i.expression_malformed = false;
    errno = 0;
    char *end = NULL;
    long operand = strtol(buf, &end, 10);
    //check conversion:
    if(*end=='\n'){
        fprintf(stderr, "newline was here");
    }

    //TODO: ERROR HERE -> newline somehow allways here
    if ((*end != '\0' && *end != '\n') || (buf == end)) {       //conversion interrupted || no conversion happened
        fprintf(stdout, "%s: ", client_i.name);
        fflush(stdout);
        //TODO: print buf here: character by character until \n is encountered.
        fprintf(stdout, "is malformed. \n");
        client_i.expression_malformed = true;
        return 0;
    }

    if (errno != 0) {        //== ERANGE //as alternative to != 0
        //printf("Overflow or underflow occurred.");
        perror("Conversion of argument ended with error");
        fprintf(stderr, "usage: ."__FILE__" <number of files> \n");
        exit(EXIT_FAILURE);
    }
    return operand;
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