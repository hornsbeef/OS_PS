
#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE
// #define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <setjmp.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdatomic.h>
//#include "client_queue.h"

#define MAX_USERNAME_LEN 256
#define MAX_CLIENTS 1024
#define MAX_MESSAGE_LEN 256


void check_argc(int argc);
unsigned long long int cast_to_ulli_with_check(char *string);
//double cast_to_double_with_check(char* buf, jmp_buf msg_rev_loop) ;
void pthread_error_funct(int pthread_returnValue);
void *listener_thread(void *arg);
void* request_handler(void* arg);
double cast_to_double_with_check(char* buf);
void *client_thread(void *arg);


//Region signal handler
volatile sig_atomic_t interrupted = 0;      // ! no idea how to handle signals without a global flag.

void sigint_handler(int sig) {
    (void) sig;
    interrupted = 1;
}
//End

typedef struct {
    int sockfd;
    char username[MAX_USERNAME_LEN];
    int is_admin;
    void* thread_struct_t_PTR;
    //pthread_t listener_thread_id;
    pthread_t client_tid;
    int client_number;      //client number used for the socket_of_clients[]
    atomic_bool timeout;
} client_t;

typedef struct thread_struct{
    int sockfd;
    pthread_mutex_t* mutex_queue_PTR;
    pthread_t listener_tid;
    int numberOfAdmins;
    char** admin_username_array;
    client_t clients[MAX_CLIENTS];
    sig_atomic_t num_clients;        // Number of currently connected clients -> is needed for disconnection message
    int max_num_clients;
    //atomic_int socket_of_clients[MAX_CLIENTS];  // * test if this works!
    int socket_of_clients[MAX_CLIENTS];
}thread_struct_t;





int main(int argc, char *argv[]) {

    //Region mutex
    pthread_mutex_t mutex_queue;

    //mutex and mutex_attr init:
    pthread_mutexattr_t mutex_queue_attr;
    pthread_error_funct(pthread_mutexattr_init(&mutex_queue_attr));
    pthread_error_funct(pthread_mutexattr_setpshared(&mutex_queue_attr, PTHREAD_PROCESS_SHARED));
    pthread_error_funct(pthread_mutexattr_settype(&mutex_queue_attr, PTHREAD_MUTEX_ERRORCHECK));

    pthread_error_funct(pthread_mutex_init(&mutex_queue, &mutex_queue_attr));

    ////cond_data_pushed_to_queue init:
    //pthread_cond_t cond_data_pushed_to_queue;
    //pthread_error_funct(pthread_cond_init(&cond_data_pushed_to_queue, NULL));

    //End


    //Region check args
    int return_param = EXIT_SUCCESS;


    check_argc(argc);

    unsigned long long port = cast_to_ulli_with_check(argv[1]);
    if (port < 1024 || port > 65535) {
        printf("usage: ."__FILE__" < 1 port between 1024 - 65535 >\n");
        exit(EXIT_FAILURE);
    }

    int numberOfAdmins = argc - 2;
    char* admin_username_array[numberOfAdmins];

    for(int i = 0; i<numberOfAdmins; i++){
        admin_username_array[i] = argv[i+2];
        //fprintf(stderr, "%s\n", admin_username_array[i]);
    }


    //End

    // * socket() creates an endpoint for communication and returns a descriptor.
    errno = 0;
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);   //protocol: 0 for default for this , 6 for tcp
    if (sockfd < 0) {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }

    // * safe to Pieces!
    // * this allows for a socket to be bound to an adress that is already in use -> .
    int opt = 1; // * set SO_REUSEADDR to value of opt
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    /**
     * When a socket is created with socket(2),
     * it exists in a name space (address family) but has no address assigned to it.
     * bind() assigns the address specified by addr to the socket referred to by the file descriptor sockfd.
     * addrlen specifies the size, in bytes, of the address structure pointed to by addr.
     * Traditionally, this operation is called "assigning a name to a socket".
     */
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if(bind(sockfd, (const struct sockaddr*) &addr, sizeof(addr) ) != 0 ){
        perror("Bind");
        return_param = EXIT_FAILURE;
        close(sockfd);
        exit(return_param);
    }


    if(listen(sockfd, 1) == -1){
        perror("Listen");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    /**
     *listen() marks the socket referred to by sockfd as a passive
       socket, that is, as a socket that will be used to accept incoming
       connection requests using accept(2).

       The sockfd argument is a file descriptor that refers to a socket
       of type SOCK_STREAM or SOCK_SEQPACKET.

       The backlog argument defines the maximum length to which the
       queue of pending connections for sockfd may grow.  If a
       connection request arrives when the queue is full, the client may
       receive an error with an indication of ECONNREFUSED or, if the
       underlying protocol supports retransmission, the request may be
       ignored so that a later reattempt at connection succeeds.

     */

//Region threadStruct
    thread_struct_t threadStruct;
    threadStruct.numberOfAdmins = numberOfAdmins;
    threadStruct.admin_username_array = admin_username_array; // ! Unsure if correctly assigned //->should be ok

    threadStruct.sockfd = sockfd;
    threadStruct.num_clients = 0;
    threadStruct.max_num_clients = 0;
    threadStruct.mutex_queue_PTR = &mutex_queue;
//End

    printf("Listening on port %llu\n", port);
    fflush(stdout);

    if (pthread_create(&threadStruct.listener_tid, NULL, listener_thread, &threadStruct) != 0) {
        perror("Failed to create listener thread");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    //unnecessary but (should be safer)
    pthread_error_funct(pthread_mutex_lock(threadStruct.mutex_queue_PTR));
    pthread_t listener_tid = threadStruct.listener_tid;
    pthread_mutex_unlock(threadStruct.mutex_queue_PTR);

//Region Join Listener Thread
//fprintf(stderr, "Test before pthread_join(LISTENER)");
    pthread_error_funct(pthread_join(listener_tid, NULL));
//fprintf(stderr, "Listener-Thread ended. \n"); //not reached without pthread_cancel!
//End

//Region: get max_connected_clients -> join all clients!
    // MUST be done after!!! the listener thread is killed, because is only changed in listener thread!!!!
    pthread_error_funct(pthread_mutex_lock(threadStruct.mutex_queue_PTR));
    int max_connected_clients = threadStruct.max_num_clients;
    pthread_mutex_unlock(threadStruct.mutex_queue_PTR);

//fprintf(stderr, "MAX CONNECTD CLIENTS: %d\n", max_connected_clients);

//? needed to remove mutex here, to remove problem with ending server getting stuck after clients sent shutdown command.
// *-> removing mutex here worked!
// ? why did it get stuck in the first place?

    //pthread_join all client_threads
    for (int i = 0; i < max_connected_clients; ++i) {
//fprintf(stderr, "JOIN_LOOP: i = %d\n", i);
        //pthread_error_funct(pthread_mutex_lock(threadStruct.mutex_queue_PTR));
        pthread_t tid = threadStruct.clients[i].client_tid;
        pthread_error_funct(pthread_join(tid, NULL));
//fprintf(stderr, "Joined number %d of %d\n", i, max_connected_clients );
        //pthread_mutex_unlock(threadStruct.mutex_queue_PTR);
    }
//End


    close(sockfd);
    return return_param;
}

//helper functs
void check_argc(int argc) {
    if (argc < 3 || argc >7) {
        printf("usage: ."__FILE__" < 1 port >= 1024 and up to 5 admin names>");
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
        if(errno != 0){
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

void pthread_error_funct(int pthread_returnValue) {
    if (pthread_returnValue != 0) {
        char *error_msg = strerror(pthread_returnValue);
        fprintf(stderr, "Error code: %d\n"
                        "Error message: %s\n"
                        "Note that the pthreads functions do not set errno.\n",
                pthread_returnValue, error_msg);
        exit(EXIT_FAILURE);
    }
}

void *listener_thread(void *arg) {
    thread_struct_t *threadStruct_PTR = (thread_struct_t *) arg;
    int conn_sockfd;

    pthread_error_funct(pthread_mutex_lock(threadStruct_PTR->mutex_queue_PTR));
    int listener_sockfd = threadStruct_PTR->sockfd;
    pthread_mutex_unlock(threadStruct_PTR->mutex_queue_PTR);

    // * The listener thread should terminate once the /shutdown command has been received from an admin.
    //while (true)  //version with pthread_cancel

    while (interrupted == 0) //version with "signal handler flag" -> gets stuck without pthread_cancel
    {
        errno = 0;
        if ((conn_sockfd = accept(listener_sockfd, NULL, NULL)) == -1)
        {
            perror("Accept");
            // ? how to handle failure - cleanup
            exit(EXIT_FAILURE);
        }

        char username[MAX_USERNAME_LEN];
        ssize_t bytes_read = recv(conn_sockfd, username, sizeof(username), 0);
        if (bytes_read <= 0) {
            close(conn_sockfd);
            //continue;
            exit(EXIT_FAILURE);
        }

        //username[bytes_read - 1] = '\0';  // Remove newline character // ! replaced the wrong char
        username[bytes_read] = '\0';  // Remove newline character

        int is_admin = 0;
        for (int i = 0; i < threadStruct_PTR->numberOfAdmins; i++) {
            if (strcmp(username, threadStruct_PTR->admin_username_array[i]) == 0) {
                is_admin = 1;
                break;
            }
        }

        int current_client_num = threadStruct_PTR->max_num_clients; //Cave: with only num_clients -> when one disconnects and then another connects -> overwrites

        //setting all client-specific fields:
        pthread_error_funct(pthread_mutex_lock(threadStruct_PTR->mutex_queue_PTR));
        strncpy(threadStruct_PTR->clients[current_client_num].username, username, MAX_USERNAME_LEN);

        threadStruct_PTR->clients[current_client_num].is_admin = is_admin;
        threadStruct_PTR->clients[current_client_num].sockfd = conn_sockfd;
        threadStruct_PTR->clients[current_client_num].thread_struct_t_PTR = arg;
        threadStruct_PTR->clients[current_client_num].client_number = current_client_num;
        threadStruct_PTR->clients[current_client_num].timeout = false;


        //atomic_store( &threadStruct_PTR->socket_of_clients[current_client_num], conn_sockfd);
        //_Atomic(int)* atomic_ptr = &threadStruct_PTR->socket_of_clients[current_client_num];
        //atomic_store(atomic_ptr, conn_sockfd);
        // ! it seems atomic_store does not want to work here -> back to mutexes
        threadStruct_PTR->socket_of_clients[current_client_num] = conn_sockfd;

        threadStruct_PTR->num_clients++;    //is this even used?
        threadStruct_PTR->max_num_clients++;

        //pthread_create(&((threadStruct_PTR->(clients[current_client_num])).client_tid), NULL, client_thread, &threadStruct_PTR->clients[current_client_num]);
        pthread_create(&((threadStruct_PTR->clients[current_client_num]).client_tid), NULL, client_thread, &threadStruct_PTR->clients[current_client_num]);
        pthread_mutex_unlock(threadStruct_PTR->mutex_queue_PTR);

    }

    //TODO: with pthread_cancel:
    // -> see pthread_cleanup_push!!

    close(listener_sockfd);
    pthread_exit(NULL);
    fprintf(stderr, "should be unreachable");
}

void *client_thread(void *arg) {
    client_t* client = (client_t *)arg;
    thread_struct_t* threadStruct_PTR = (thread_struct_t*) client->thread_struct_t_PTR;


    char username[MAX_USERNAME_LEN] = {0};

    pthread_error_funct(pthread_mutex_lock(threadStruct_PTR->mutex_queue_PTR));
    bool isAdmin = (client->is_admin == 1) ? true : false;
    //strncpy(username, client->username, MAX_USERNAME_LEN - 1);  //works   //gives compiler warning
    strncpy(username, client->username, MAX_USERNAME_LEN);  //works
    int client_sockfd = client->sockfd;
    pthread_mutex_unlock(threadStruct_PTR->mutex_queue_PTR);


    printf("%s connected\n", username);
    fflush(stdout);

    int connection_lost_flag = 0;

    recv_loop:
    while (1) {
        char buffer[MAX_MESSAGE_LEN] = {0};
        ssize_t bytes_read = recv(client_sockfd, buffer, sizeof(buffer), 0);
        //if (bytes_read <= 0) {
        if (bytes_read == -1) // * this means error
        {
            perror("Recv at client_thread"); //TODO: here error with /shutdown
            fprintf(stderr, "with buffer: %s\n\n\n", buffer);

            break; //this ends server
        }
        if (bytes_read == 0)    // * this means peer has performed  shutdown
        {
            //fprintf(stderr, "received 0 bytes\n");
            connection_lost_flag = 1;
            goto shutdown_procedure;
        }




        buffer[bytes_read - 1] = '\0';  // Remove newline character

//Region Shutdown
        if (strcmp(buffer, "/shutdown") == 0) {
//fprintf(stderr, "/shutdown received ");

            //TODO: need to set the  threadStruct_PTR->socket_of_clients[THIS CURRENT CLIENT] to -1
            //must do it in a Mutex...
            //had problem with mutex here last time.
            shutdown_procedure:
            pthread_error_funct(pthread_mutex_lock(threadStruct_PTR->mutex_queue_PTR));
            threadStruct_PTR->socket_of_clients[client->client_number] = -1;      // * to signal that this socket is not in use anymore. -> no send() there!
            pthread_mutex_unlock(threadStruct_PTR->mutex_queue_PTR);

            close(client_sockfd);

            // ! was not getting into a mutex placed around this...
            threadStruct_PTR->num_clients--;    // * solved with an atomic variable! no mutex here


            printf("%s disconnected.\n", username);
            fflush(stdout);

            if(isAdmin && connection_lost_flag==0){
                printf("Server is shutting down.\n");
                printf("Waiting for %d clients to disconnect.\n", threadStruct_PTR->num_clients);

                pthread_cancel(threadStruct_PTR->listener_tid);
                //interrupted = 1;
            }
            pthread_exit(NULL);
        }
//End

//Region Timeout
        //go through all clients and check if username is there.
            //if true-> get its sockfd
                //send : "You have been timeouted by <admin>."
                //set NEW client_timouted flag in client struct
        //this must be checked every time when receiving msg from clients.

        // Check if the input starts with "/timeout "
        if ((strncmp(buffer, "/timeout ", 9) == 0) && isAdmin) {
            //fprintf(stderr, "Received /timeout\n");

            //create timeout_msg:
            char timeout_msg[MAX_USERNAME_LEN + 30] = {0};
            sprintf(timeout_msg, "You have been timeouted by %s.\n", username);
            //fprintf(stderr, "Timeout_MSG:<%s>\n",timeout_msg);

            //get Username to find, from buffer:
            char username_to_timeout[256] = {0};
            strcpy(username_to_timeout, buffer + 9);
            //fprintf(stderr, "Username_to_timeout:<%s>\n", username_to_timeout);

            //compare username_to_timeout against all usernames in array:
            pthread_error_funct(pthread_mutex_lock(threadStruct_PTR->mutex_queue_PTR));
            for (int i = 0; i < threadStruct_PTR->max_num_clients; ++i){
                if(strcmp(threadStruct_PTR->clients[i].username, username_to_timeout) == 0){
                    //fprintf(stderr, "Inside Username Comparison: %s vs %s",threadStruct_PTR->clients[i].username, username_to_timeout );

                    //send timout_msg to user being timeout'd
                    ssize_t bytes_sent = send(threadStruct_PTR->clients[i].sockfd, timeout_msg, strlen(timeout_msg), 0);
                    if(bytes_sent == -1){
                        perror("Send");
                        fprintf(stderr, "Error from send().\n"
                                        "Continuing execution, retrying send() with different value on next entry.\n ");
                    }

                    //set user being timeout'd - timeout_value to True:
                    threadStruct_PTR->clients[i].timeout = true;
                    break; //TODO: check if breaks functionality

                }
            }
            pthread_mutex_unlock(threadStruct_PTR->mutex_queue_PTR);

            //continue; ->  breaks functionality
            goto recv_loop;
        }
//End

//Region Timeout on NON Admin
        if ((strncmp(buffer, "/timeout ", 9) == 0) && !isAdmin){
            char error_msg[] = "You are not an Admin, so you cannot send timeouts.\n";
            ssize_t bytes_sent = send(client_sockfd, error_msg, strlen(error_msg), 0);
            if(bytes_sent == -1){
                perror("Send");
                fprintf(stderr, "Error from send().\n"
                                "Continuing execution, retrying send() with different value on next entry.\n ");
            }
            goto recv_loop;
        }

//End

        if(client->timeout == true){
            continue;
        }
        //if NOT /shutdown:
        //Region Brodcast to all OTHER clients:
        // * exclude the sockfd that this client has!
        // * Mutex should not be necessary, but lets try with it:
        pthread_error_funct(pthread_mutex_lock(threadStruct_PTR->mutex_queue_PTR));
        for (int i = 0; i < threadStruct_PTR->max_num_clients; ++i) {
            int socket_to_broadcast_to = threadStruct_PTR->socket_of_clients[i];
            if(socket_to_broadcast_to == client_sockfd || socket_to_broadcast_to == -1) // * NO send to these sockets
            {
                continue;
            }
            ssize_t bytes_sent = send(socket_to_broadcast_to, buffer, strlen(buffer), 0);
            if(bytes_sent == -1){
                perror("Send");
                fprintf(stderr, "Error from send().\n"
                                "Continuing execution, retrying send() with different value on next entry.\n ");
            }
        }

        pthread_mutex_unlock(threadStruct_PTR->mutex_queue_PTR);

        //End
    }
    return NULL;
}
