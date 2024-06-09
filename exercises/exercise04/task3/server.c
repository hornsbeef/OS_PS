/*
 * snprintf() -> for fifo??
 * strcspn() -> for cropping ??
 */


#define _GNU_SOURCE     //<<-essential for compiler...
#include <signal.h>
#include <sys/types.h>
//#include <sys/signal.h>
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
#include <pthread.h>



// Global flag to indicate when the signal has been received
volatile sig_atomic_t signal_received = 0;

typedef struct client {
    char name[256];
    double counter;
    int file_descriptor;
    bool is_connected;
    bool expression_malformed;
    int argc;

} client_t;

// Signal handler function
void signal_handler(int signo) {
    (void) signo;
    signal_received = 1;
}

// Signal handling thread function
void* signal_thread(void* arg) {


    //signal handling in thread:
    struct sigaction act;
    sigfillset(&act.sa_mask);   //all signals blocked while in signal handler
    //act.sa_flags = SA_RESTART;      //compiler: error: ‘SA_RESTART’ undeclared (first use in this function); did you mean ‘ERESTART’? ??????

    act.sa_handler = signal_handler;
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGHUP, &act, NULL);


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

double cast_to_double_with_check(client_t *client_i, char* buf);


int main(int argc, char* argv[]) {
    check_argc_count(argc);
    //TODO: LIST:
    // - check what other signals to handle!
    //  SIGHUP,

    //DONE:
    // -build in signal handling for sigterm -> clean up fifo mess!

    //general setup:
    client_t clients[argc];
    mode_t client_server_fifo_perm = (S_IRUSR | S_IWUSR);   //sys/stat.h   https://pubs.opengroup.org/onlinepubs/7908799/xsh/sysstat.h.html
    // this is the permissions parameter used for creating the FIFOs.
    // The mode parameter is modified by the process's file creation mask,
    // which is set using the umask() system call.
    // Example: if the the file mode creation mask is set to 022 ,
    // and the mkfifo() function is called with a mode parameter of 0666,
    // the actual permissions of the newly created FIFO file will be rw-rw-rw- (0666 & ~022 = 0644).
    //
    //When executing a C program that uses mkfifo(), the permissions of the user that executes the file are used.
    //therefore I chose to only give the user that executes the program read and write permissions, as
    // I dont want other users potentially messing with the pipes.




    for (int i = 1; i < argc; ++i) {
        clients[i].argc = argc; //needed for thread
    }

    //create signal handling thread
    pthread_t thread;
    pthread_create(&thread, NULL, signal_thread, clients);

    // Block SIGTERM in the main thread (but if done before pthread_create, the other threads will inherit this!)
    sigset_t set;   //complier: error: unknown type name ‘sigset_t’; did you mean ‘size_t’?
    sigemptyset(&set);      //ensure is empty
    sigaddset(&set, SIGTERM);   //only add SIGTERM
    pthread_sigmask(SIG_BLOCK, &set, NULL); //this now blocks SIGTERM in main thread
    //-> compiler still throws warning even if signal.h is included....


    //creating all the clients data
    struct pollfd fds[argc-1];    //for poll()
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
            //goto cleanup;
            signal_received = 1;    //signal handling thread takes care of cleanup
            sleep(1);
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
        //poll()  waits for one of a set of file descriptors to become ready to perform I/O.
        //this set of fd is struct pollfd fds[]
        errno = 0;
        int events = poll(fds, argc-1, -1);
        if(events < 0 ){
            //poll has thrown error
            perror("Poll");
            //goto cleanup;
            signal_received = 1;    //signal handling thread takes care of cleanup
            sleep(1);
        }


        //once data is available -> go through all file descriptors to check which one has data available:
        for (int i = 1; i < argc; ++i) {

            //check for connection ended by the client
            if(fds[i-1].revents & POLLHUP){ //seems to work.

                all_clients_disconnected++;
                printf("%s disconnected.\n", clients[i].name);
                fflush(stdout);


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

                clients[i].counter += cast_to_double_with_check(&clients[i], buf);
                if(clients[i].expression_malformed == false){
                    //printf("%s: counter = %ld.\n", clients[i].name, clients[i].counter);
                    printf("%s: counter = %g\n", clients[i].name, clients[i].counter);  //now uses %g
                }

            }
        }

    }


    // close & UNLINK!!! all the pipes again
    for (int i = 1; i < argc; ++i) {
        errno = 0;
        int close_error = close(clients[i].file_descriptor);
        if(close_error < 0){
            perror("Close");
        }
        unlink(clients[i].name);            //<- MUST NOT FORGET!
        if(close_error < 0){
            perror("Unlink");
        }
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
        //goto cleanup;
        signal_received = 1;    //signal handling thread takes care of cleanup
        sleep(1);

        //should not be reachable
        exit(EXIT_FAILURE);
    }
}


double cast_to_double_with_check(client_t *client_i, char* buf) {
    client_i->expression_malformed = false;
    errno = 0;
    char *end = NULL;
    double operand = strtod(buf, &end);

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

        //goto cleanup;
        signal_received = 1;    //signal handling thread takes care of cleanup
        sleep(1);


        exit(EXIT_FAILURE);
    }
    return operand;
}

/* CHECKLIST:
Server
    The server can be started with a list of clients that may connect to it, e.g. after starting with ./server jacobi riemann,
    two clients named jacobi and riemann can connect. -> DONE

    The server then creates a FIFO for each client.
    Think about a way of associating each FIFO with a specific client.
    A good location to store FIFOs is somewhere inside /tmp (when working on ZID-GPL, be sure to avoid file naming collisions with other students).
    ->Done

    The server then waits for clients to connect by opening each FIFO for reading.
    There is no need to use fork(2) in this task, you can assume that clients connect in the same order they are specified on the command line.
    -> Done

    Once a client connects, the server prints the message "<client name> connected.". ->Done

    The server continuously monitors all FIFOs for incoming math expressions using poll(2). ->Done
    ( I guess math expressions means numbers, not something like 4+5 or (4+5)*4³ ... this would need parsing -> could be done with something like tinyexpr (github))

    Once an expression is received, it is added to the counter variable and printed to the console like so "<client name>: counter = <counter>.". ->Done

    In the beginning, the server initializes the counter variable with 0. -> Done
    If an expression does not conform to the expected format, the server instead prints "<client name>: <expression> is malformed.". -> Done
    If a client disconnects, the server prints the message "<client name> disconnected.". -> Done
    Once all clients are disconnected, the server cleans up the FIFOs and exits. ->Done


Additional notes and hints:

    You can assume that all clients are connected before handling their messages.

    Your server doesn't need to support clients re-connecting, i.e. they can (and must) connect exactly once.

    Your solution must use poll(2), not select(2). -> Done

    Make sure to use appropriate file permissions (which we discussed in connection to chmod) to create and open your FIFOs. Justify your choice. ->Done

    Your FIFOs should be blocking, i.e. you must not use O_NONBLOCK in your calls to open(2). ->Done

    You can use the %g format specifier to let printf(3) decide on the number of significant digits to print.
    As always, make sure to properly release all allocated resources (e.g. FIFOs) upon exiting.
    On macOS, poll(2) is bugged and will not return when you close the pipe from the client, run on ZID-GPL to ensure the correct behaviour.
    If you want to create multiple processes side-by-side for testing, there are several different options available to you:
        Open multiple terminal windows or tabs.
        Use shell jobs to switch between different processes: CTRL + Z, &, fg, bg, jobs, etc.
        Use a terminal multiplexer such as tmux or screen



 */





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