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

#define MAX_USERNAME_LEN 256


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

        if (bytes_received > 0) // * Print received message
        {
            buffer[bytes_received] = '\0';
            printf("%s\n", buffer);
        } else if (bytes_received == 0) // * Server has disconnected -> must clean up and exit client
        {
            printf("Server disconnected.\n");
            interrupted = 1;        // * handle cleanup!
            pthread_exit(NULL);
        }
        else // * negative value of bytes received -> means -1 -> means error
        {
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

    char* username = argv[2];
    unsigned long username_len = strlen(username);
fprintf(stderr, "Username = %s\n", username);


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
    //sends username as first message:
    ssize_t bytes_sent = send(sockfd, username, strlen(username), 0);    // * 0 means no special flags are set
    if(bytes_sent == -1){
        perror("Send");
        fprintf(stderr, "Error from send().\n"
                        "Continuing execution, retrying send() with different value on next entry.\n ");
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
    rec_loop:
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

//Region Shutdown_Client
        if (strcmp(buffer_input, "/shutdown\n") == 0) {

/* TODO: this creates problem, when trying to first send username disconnected. and then send /shutdown. WHY?
            //unsigned long disconnect_message_len = username_len + sizeof(" disconnected.\n") + 1;  //TODO: check if  + 1 for null terminator is needed!
            char discon[] = " disconnected.\n";
            unsigned long disconnect_message_len = username_len + sizeof(discon);

fprintf(stderr, "message length: %lu\n", disconnect_message_len);

            char disconnect_message[disconnect_message_len];
            memset(disconnect_message, '\0', sizeof(disconnect_message));
            //strcpy(disconnect_message, username);
            //strcat(disconnect_message, discon); //to concatenate " disconnected."
            //username disconnected.\n\0
            / ** The  strcat() function appends the src string to the dest string, over‐
            writing the terminating null byte ('\0') at the end of dest,  and  then
            adds  a  terminating  null  byte. * /
            sprintf(disconnect_message, "%s%s", username, discon);
fprintf(stderr, "Disconnect_message: <%s>\n", disconnect_message);


            bytes_sent = send(sockfd, disconnect_message, strlen(disconnect_message), 0);    // * 0 means no special flags are set
            if(bytes_sent == -1){
                perror("Send");
                fprintf(stderr, "Error from send().\n"
                                "Continuing execution, retrying send() with different value on next entry.\n ");
            }
*/
            bytes_sent = send(sockfd, buffer_input, strlen(buffer_input), 0);
            if(bytes_sent == -1){
                perror("Send");
                fprintf(stderr, "Error from send().\n"
                                "Continuing execution, retrying send() with different value on next entry.\n ");
            }
            goto cleanup2;
        }
//End

//Region Timeout
        if ((strncmp(buffer_input, "/timeout ", 9) == 0))   // * if it's /timeout ... just send the msg without adding name!
        {
            bytes_sent = send(sockfd, buffer_input, strlen(buffer_input), 0);
            if(bytes_sent == -1){
                perror("Send");
                fprintf(stderr, "Error from send().\n"
                                "Continuing execution, retrying send() with different value on next entry.\n ");
            }
            goto rec_loop;
        }
//End

//Region Send Normal MSG
        unsigned long message_len = username_len + sizeof(": ") + sizeof(buffer_input) + 1;  // + 1 for null terminator
        char message[message_len];
        strcpy(message, username);
        strcat(message, ": ");
        strcat(message, buffer_input);

        bytes_sent = send(sockfd, message, strlen(message), 0);    // * 0 means no special flags are set
        if(bytes_sent == -1){
            perror("Send");
            fprintf(stderr, "Error from send().\n"
                            "Continuing execution, retrying send() with different value on next entry.\n ");
        }
//End


        // * replace \n with \0 in char buffer[]
        //buffer_input[strcspn(buffer_input, "\n")] = '\0';


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
    if (argc != 3) {
        printf("usage: ."__FILE__" <port> <username>\n");
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