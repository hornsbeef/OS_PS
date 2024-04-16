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
#include <bits/types/sig_atomic_t.h>
#include <signal.h>
#include <pthread.h>


// Global flag to indicate when the signal has been received
volatile sig_atomic_t signal_received = 0;

typedef struct client {
    char name[256];
    long counter;
    int file_descriptor;
    bool is_connected;
    bool expression_malformed;
    int argc;

} client_t;

// Signal handler function
void signal_handler(int signo) {
    signal_received = 1;
}

// Signal handling thread function
void* signal_thread(void* arg) {
    //TODO: NOT WORKING YET!
    //signal handling in thread:
    struct sigaction act;
    sigfillset(&act.sa_mask);   //all signals blocked while in signal handler
    act.sa_flags = SA_RESTART;

    act.sa_handler = signal_handler;
    sigaction(SIGTERM, &act, NULL);

    //get clients data:
    client_t* client = arg;
    int argc =  client[1].argc;


    while (!signal_received);    // Wait for the signal //busy waiting

    // Perform cleanup task:
    for (int i = 1; i < argc; ++i) {
        close(client[i].file_descriptor);
        unlink(client[i].name);            //<- MUST NOT FORGET!
    }


    // Exit the program
    exit(EXIT_SUCCESS);     //"program failed successfully"
}





void pipe_error_check(int pipe_error);

void check_argc_count(int argc);

long cast_to_int_with_check(client_t *client_i, char* buf);


int main(int argc, char* argv[]) {
    check_argc_count(argc);
    //TODO: LIST:
    // -build in signal handling for sigterm -> clean up fifo mess!

    //general setup:
    client_t clients[argc];
    mode_t client_server_fifo_perm = (S_IRUSR | S_IWUSR);
    struct pollfd fds[argc-1];    //for poll()
    for (int i = 1; i < argc; ++i) {
        clients[i].argc = argc; //needed for thread
    }

    // Block SIGTERM in the main thread
    sigset_t set;
    sigemptyset(&set);      //ensure is empty
    sigaddset(&set, SIGTERM);   //only add SIGTERM
    pthread_sigmask(SIG_BLOCK, &set, NULL); //this now blocks SIGTERM in main thread

    pthread_t thread;
    pthread_create(&thread, NULL, signal_thread, clients);

    //creating all the clients data
    for (int i = 1; i < argc; ++i) {

        strcpy(clients[i].name, argv[i]);
        clients[i].counter = 0;
        clients[i].is_connected = false;


        //check if fifo exist and delete. (cleanup if previous process was interrupted and did not clean up properly
        if(access(clients[i].name, F_OK) == 0){
            unlink(clients[i].name);
        }

        //create fifo for current client with error checking
        //Think about a way of associating each FIFO with a specific client.
        //I have chosen to associate the fifo with its corresponding client by name: both, fifo and client(argument) must be the same.
        //I also have chosen to store the fifo in the directory of the executable, so proper clean-up routine can be checked more easily.
        //for changing this to /tmp an approach might be to use strcat():
        //char tmp_location[256] = "/tmp/";     //<- ideally saved to the struct for later access.
        //strcat(tmp_location, clients[i].name)
        //
        //some testing has revealed that two clients with the same name can open the pipe at the same time and write to it
        errno = 0;
        int fifo_error = mkfifo(clients[i].name, client_server_fifo_perm);
        pipe_error_check(fifo_error);

        //OPEN FIFO FOR READ:
        //There is no need to use fork(2) in this task,
        // you can assume that clients connect in the same order they are specified on the command line.

        //for ease of use without the O_NONBLOCK:
        fprintf(stdout, "Please connect client %s.\n", clients[i].name);
        fflush(stdout);

        //actual opening happening here:
        errno = 0;
        clients[i].file_descriptor = open(clients[i].name, O_RDONLY);
        if(clients[i].file_descriptor < 0){
            perror("Open failed");
        }
        //unfortunately O_NONBLOCK is not allowed. (and I still can't read to the end of instructions...)
        //So the program waits here(@ the open()) until the specified client is connected.
        //code for alternative connection method using O_NONBLOCK removed and commented at the end of the document.

        clients[i].is_connected = true;
        //opening procedure ended.

        fds[i-1].fd = clients[i].file_descriptor;     //for poll()
        fds[i-1].events = POLLIN;        //polling here interested in: POLLIN:
        fprintf(stdout, "%s connected.\n", clients[i].name);
        fflush(stdout);
    }

    //for ease of use -> so user knows the program did not freeze
    fprintf(stdout, "All clients have been connected - now starting routine: \n");
    fflush(stdout);




    char buf[PIPE_BUF];
    //You can assume an expression to be at most PIPE_BUF long. Why is this important?
    //PIPE_BUF : from https://man7.org/linux/man-pages/man7/pipe.7.html
    //POSIX.1 says that writes of less than PIPE_BUF bytes must be
    //atomic: the output data is written to the pipe as a contiguous
    //sequence.  Writes of more than PIPE_BUF bytes may be nonatomic:
    //the kernel may interleave the data with data written by other
    //processes.
    //and it's nice to know how large to make the buffer


    //continuous loop running until all clients are disconnected
    int all_clients_disconnected = 0;
    while(all_clients_disconnected < (argc-1)){

        //wait indefinitely for events via poll:
        int events = poll(fds, argc-1, -1);
        //todo: see what events can be used for
        //todo: error checking

        //todo: check what happens if multiple data is available at once??

        //once data is available -> go through all file descriptors to check which one has data available:
        for (int i = 1; i < argc; ++i) {

            //check for connection ended by the client
            if(fds[i-1].revents & POLLHUP){ //seems to work.

                all_clients_disconnected++;
                printf("%s disconnected.\n", clients[i].name);
                fflush(stdout);

                close(clients[i].file_descriptor);  //could be done below
                unlink(clients[i].name);                //could be done below

                //To remove one entry from the pollfd[] array,
                // you can simply set the fd member of the pollfd struct to -1
                // to indicate that it should no longer be polled by the poll() function.
                fds[i-1].fd = -1;
                //The field fd contains a file descriptor for an open file.
                // If this field is negative, then the corresponding events field is
                // ignored and the revents field returns zero.
                //https://man7.org/linux/man-pages/man2/poll.2.html


            }
            //check if data to read is available -> save to buf.
            if(fds[i-1].revents & POLLIN) {
                memset(buf, 0, PIPE_BUF);
                read(clients[i].file_descriptor,buf, PIPE_BUF);

                clients[i].counter += cast_to_int_with_check(&clients[i], buf);
                if(clients[i].expression_malformed == false){
                    printf("%s: counter = %ld.\n", clients[i].name, clients[i].counter);
                }

            }
        }

    }



    // close & UNLINK!!! all the pipes again
    for (int i = 1; i < argc; ++i) {
        close(clients[i].file_descriptor);
        unlink(clients[i].name);            //<- MUST NOT FORGET!
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
    client_i->expression_malformed = false;
    errno = 0;
    char *end = NULL;
    long operand = strtol(buf, &end, 10);

    //check conversion:
    if ((*end != '\0' && *end != '\n') || (buf == end)) {       //conversion interrupted || no conversion happened
        fprintf(stdout, "%s: ", client_i->name);
        fflush(stdout);
        fprintf(stdout, "is malformed. \n");
        client_i->expression_malformed = true;
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