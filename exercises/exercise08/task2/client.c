#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE
// #define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <setjmp.h>
#include <pthread.h>
#include <signal.h>


void check_argc(int argc);

unsigned long long int cast_to_ulli_with_check(char *string);

double cast_to_double_with_check(char *buf, jmp_buf msg_rev_loop);


// ! no global variables must be used -> no idea how to handle signals without a global flag.
volatile sig_atomic_t interrupted = 0;

void sigint_handler(int sig) {
    (void) sig;
    interrupted = 1;
}


// Function to handle receiving messages from the server
void *listener_thread(void *arg) {
    int sockfd = *(int *) arg;

    while (1) {
        char buffer[1024];
        ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer), 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("Donations: %s\n", buffer);
        } else if (bytes_received == 0) {
            printf("Server disconnected.\n");
            interrupted = 1;        // * handle cleanup!
            pthread_exit(NULL);
        } else {
            perror("Error receiving data");
            interrupted = 1;        // * handle cleanup!
            pthread_exit(NULL);
        }
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    //Region check args
    int return_param = EXIT_SUCCESS;

    check_argc(argc);

    unsigned long long port = cast_to_ulli_with_check(argv[1]);
    if (port < 1024 || port > 65535) {
        printf("usage: ."__FILE__" < 1 port between 1024 - 65535 >\n");
        exit(EXIT_FAILURE);
    }
    //End

    //Region socket init

    errno = 0;
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);   //protocol: 0 for default for this , 6 for
    if (sockfd < 0) {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }
    int opt = 1; // * set SO_REUSEADDR to value of opt
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        goto cleanup1;
    }

    //const struct sockaddr_in addr = {       //posted struct did NOT work
    //        .sin_family = AF_INET,
    //        //.sin_addr = htonl(INADDR_ANY),
    //        .sin_addr = htonl(INADDR_LOOPBACK), // on client
    //        .sin_port = htons(port),
    //};
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    //End

    //Region connect:
    if (connect(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("Connection failed");
        goto cleanup1;
    }
    //End

    //Region signal handler

    // catch sigint to clean up
    struct sigaction act = {0};
    act.sa_handler = &sigint_handler;
    sigfillset(&act.sa_mask);   //all signals blocked while in signal handler
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT, &act, NULL);

    //End

    //Region spawn thread:

    pthread_t listener_tid;
    if (pthread_create(&listener_tid, NULL, listener_thread, &sockfd) != 0) {
        perror("Failed to create listener thread");
        goto cleanup1;
    }
    //End

    //Region work:
    /* *
     * Note that you don't need to use ntohl() or ntohs() here because the recv() function
     * automatically converts the received data from network byte order to host byte order.
     */


    while (interrupted == 0) {        // *: need to implement CTRL + C handling
        //char buffer_from_server[1024] = {0};
        char buffer_input[1024] = {0};
        if (fgets(buffer_input, sizeof(buffer_input) - sizeof(char), stdin))    // * terminates input with \0
        {
            if (ferror(stdin)) {
                fprintf(stderr, "Error reading input: %s\n", strerror(errno));
            } else if (feof(stdin)) {
                fprintf(stderr, "End of file reached\n");
            }
        }

        //fprintf(stderr, "buffer_input = %s", buffer_input);

        if (strcmp(buffer_input, "/quit\n") == 0) {
            goto cleanup2;
        }

        send(sockfd, buffer_input, strlen(buffer_input), 0);    // * 0 means no special flags are set

        // * replace \n with \0 in char buffer[]
        //buffer_input[strcspn(buffer_input, "\n")] = '\0';
        if (strcmp(buffer_input, "/shutdown\n") == 0) {
            goto cleanup2;
        }

    }





    //Region cleanup
    //cleanup2:
    //close(conn_sockfd);
    //printf("Connection closed.\n");

    cleanup2:
    pthread_cancel(listener_tid);
    pthread_join(listener_tid, NULL);

    cleanup1:
    close(sockfd);
    exit(return_param);

    //End


}

//Region helperfunctions

void check_argc(int argc) {
    if (argc != 2) {
        printf("usage: ."__FILE__" <port>");
        exit(EXIT_FAILURE);
    }
}

unsigned long long int cast_to_ulli_with_check(char *string) {
    errno = 0;
    char *end = NULL;
    unsigned long long operand = strtoull(string, &end, 10);
    //check conversion:
    if ((*end != '\0') || (string == end)) {       //conversion interrupted || no conversion happened
        fprintf(stderr, "Conversion of argument ended with error.\n");
        if (errno != 0) {
            perror("StrToULL");
        }
        exit(EXIT_FAILURE);
    }
    if (errno != 0) {        //== ERANGE //as alternative to != 0
        perror("Conversion of argument ended with error");
        fprintf(stderr, "Please recheck usage!\n");
        exit(EXIT_FAILURE);
    }
    return operand;
}




//End