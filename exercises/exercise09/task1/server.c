
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
} client_t;

typedef struct thread_struct{
    int sockfd;
    pthread_mutex_t* mutex_queue_PTR;
    pthread_t listener_tid;
    int numberOfAdmins;
    char** admin_username_array;
    client_t clients[MAX_CLIENTS];
    int num_clients;        // ? Number of currently connected clients
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
        fprintf(stderr, "%s\n", admin_username_array[i]);
    }


    //pthread_t request_handler_tid[num_of_request_handlers];

    //End




    errno = 0;
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);   //protocol: 0 for default for this , 6 for tcp
    if (sockfd < 0) {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }

    // * safe to Pieces!
    int opt = 1; // * set SO_REUSEADDR to value of opt
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

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
//Region threadStruct
    thread_struct_t threadStruct;
    threadStruct.numberOfAdmins = numberOfAdmins;
    threadStruct.admin_username_array = admin_username_array; // ! Unsure if correctly assigned

    threadStruct.sockfd = sockfd;
    threadStruct.num_clients = 0;
    threadStruct.mutex_queue_PTR = &mutex_queue;
//End


    if (pthread_create(&threadStruct.listener_tid, NULL, listener_thread, &threadStruct) != 0) {
        perror("Failed to create listener thread");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    pthread_error_funct(pthread_mutex_lock(threadStruct.mutex_queue_PTR));
    pthread_t listener_tid = threadStruct.listener_tid;
    pthread_mutex_unlock(threadStruct.mutex_queue_PTR);

    pthread_join(listener_tid, NULL);

    //TODO: pthread_join all client_threads



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

    while (true) // * The listener thread should terminate once the /shutdown command has been received from an admin.
    {
        errno = 0;
        if ((conn_sockfd = accept(threadStruct_PTR->sockfd, NULL, NULL)) == -1) {
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
        username[bytes_read - 1] = '\0';  // Remove newline character

        int is_admin = 0;
        for (int i = 0; i < threadStruct_PTR->numberOfAdmins; i++) {
            if (strcmp(username, threadStruct_PTR->admin_username_array[i]) == 0) {
                is_admin = 1;
                break;
            }
        }
        int current_client_num = threadStruct_PTR->num_clients;
        pthread_error_funct(pthread_mutex_lock(threadStruct_PTR->mutex_queue_PTR));
        strncpy(threadStruct_PTR->clients[current_client_num].username, username, MAX_USERNAME_LEN - 1);
        threadStruct_PTR->clients[current_client_num].is_admin = is_admin;
        threadStruct_PTR->clients[current_client_num].sockfd = conn_sockfd;
        threadStruct_PTR->clients[current_client_num].thread_struct_t_PTR = arg;

        threadStruct_PTR->num_clients++;

        pthread_create(&threadStruct_PTR->clients[current_client_num].client_tid, NULL, client_thread, &threadStruct_PTR->clients[current_client_num]);
        pthread_mutex_unlock(threadStruct_PTR->mutex_queue_PTR);

    }
}

void *client_thread(void *arg) {
    client_t* client = (client_t *)arg;
    thread_struct_t* threadStruct_PTR = (thread_struct_t*) client->thread_struct_t_PTR;

    char buffer[MAX_MESSAGE_LEN];
    bool isAdmin = (client->is_admin == 1) ? true : false;
    char username[MAX_USERNAME_LEN];
    strncpy(username, client->username, MAX_USERNAME_LEN - 1);  //TODO: check if works

    while (1) {
        ssize_t bytes_read = recv(client->sockfd, buffer, sizeof(buffer), 0);
        if (bytes_read <= 0) {
            perror("Recv");
            break; //
        }

        buffer[bytes_read - 1] = '\0';  // Remove newline character

        if (strcmp(buffer, "/shutdown") == 0) {
            pthread_mutex_lock(threadStruct_PTR->mutex_queue_PTR);
            threadStruct_PTR->num_clients--;

            if(isAdmin){
                printf("Server is shutting down.\n");
                printf("Waiting for %d clients to disconnect.\n", threadStruct_PTR->num_clients);

                pthread_cancel(threadStruct_PTR->listener_tid);
            }

            close(client->sockfd);
            pthread_mutex_unlock(threadStruct_PTR->mutex_queue_PTR);
            pthread_exit(NULL);
        }
        printf("<%s>: %s\n", client->username, buffer);

    }
    return NULL;
}
