//Instructions were a bit unclear due to (for me) confusing order of those instructions.

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
#include "client_queue.h"

#define BUFFER_SIZE 1024


void check_argc(int argc);
unsigned long long int cast_to_ulli_with_check(char *string);
//double cast_to_double_with_check(char* buf, jmp_buf msg_rev_loop) ;
void pthread_error_funct(int pthread_returnValue);
void *listener_thread(void *arg);
void* request_handler(void* arg);
double cast_to_double_with_check(char* buf);


//Region signal handler
volatile sig_atomic_t interrupted = 0;      // ! no idea how to handle signals without a global flag.

void sigint_handler(int sig) {
    (void) sig;
    interrupted = 1;
}
//End

typedef struct thread_struct{
    myqueue queue;
    int sockfd;
    pthread_mutex_t* mutex_queue_PTR;
    double total_donations;
    pthread_t listener_tid;
    pthread_cond_t cond_data_pushed_to_queue;
    int num_of_request_handlers;
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

    //cond_data_pushed_to_queue init:
    pthread_cond_t cond_data_pushed_to_queue;
    pthread_error_funct(pthread_cond_init(&cond_data_pushed_to_queue, NULL));


    //End

    //Region check args
    int return_param = EXIT_SUCCESS;


    check_argc(argc);

    unsigned long long port = cast_to_ulli_with_check(argv[1]);
    if (port < 1024 || port > 65535) {
        printf("usage: ."__FILE__" < 1 port between 1024 - 65535 >\n");
        exit(EXIT_FAILURE);
    }
    unsigned long long num_of_request_handlers = cast_to_ulli_with_check(argv[2]);
    pthread_t request_handler_tid[num_of_request_handlers];

    //End

    //Region queue - socket init - listener thread
/**
 * The server creates a queue for client connections.
 * The server creates and binds the socket as in the previous task,
 * and then spawns a listener thread for accepting incoming connections:
 *  Each accepted connection is added to the client connection queue.
 */

    thread_struct_t threadStruct;
    //myqueue queue;
    //threadStruct.queue = queue;       // ! this was one problem! I init'd the wrong queue!
    myqueue_init(&threadStruct.queue);

    errno = 0;
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);   //protocol: 0 for default for this , 6 for tcp
    if (sockfd < 0) {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }
    int opt = 1; // * set SO_REUSEADDR to value of opt
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        goto cleanup1;
    }
    //posted struct did NOT work
    //const struct sockaddr_in addr = {
    //        .sin_family = AF_INET,
    //        .sin_addr = htonl(INADDR_ANY),
    //        //.sin_addr = htonl(INADDR_LOOPBACK),
    //        .sin_port = htons(port),
    //};
    /**
     * server.c:112:36: warning: missing braces around initializer []8;;https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html#index-Wmissing-braces-Wmissing-braces]8;;]
  112 |     const struct sockaddr_in addr ={
      |                                    ^
     */

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);


    if(bind(sockfd, (const struct sockaddr*) &addr, sizeof(addr) ) != 0 ){
        perror("Bind");
        return_param = EXIT_FAILURE;
        goto cleanup1;
    }
    listen(sockfd, num_of_request_handlers);

    //thread_struct_t threadStruct;
    //threadStruct.queue = queue;
    threadStruct.sockfd = sockfd;
    threadStruct.mutex_queue_PTR = &mutex_queue;
    threadStruct.total_donations = 0;
    threadStruct.cond_data_pushed_to_queue = cond_data_pushed_to_queue;
    threadStruct.num_of_request_handlers = num_of_request_handlers;

    if (pthread_create(&threadStruct.listener_tid, NULL, listener_thread, &threadStruct) != 0) {
        perror("Failed to create listener thread");
        goto cleanup1;
    }

    //End

    //Region request-handler spawn

    //pthread_t request_handler_tid[num_of_request_handlers]; // * handled further up due to goto-problems
    for(unsigned long long i = 0 ; i< num_of_request_handlers; i++){
        pthread_error_funct(pthread_create(&request_handler_tid[i], NULL, &request_handler, &threadStruct));
    }

    //End

    //Region wait for listener thread to finish
    pthread_join(threadStruct.listener_tid, NULL);
    //End

    //Region poison value push -> wait for request handlers to finish

    pthread_error_funct(pthread_mutex_lock(threadStruct.mutex_queue_PTR));
    for(unsigned long long i = 0; i<num_of_request_handlers; i++){
        myqueue_push(&threadStruct.queue, -1);
    }
    pthread_cond_broadcast(&threadStruct.cond_data_pushed_to_queue); // ! Check if works !
    pthread_mutex_unlock(threadStruct.mutex_queue_PTR);

    //End

    //Region cleanup

    //cleanup2:
    pthread_mutex_destroy(threadStruct.mutex_queue_PTR);
    pthread_cond_destroy(&threadStruct.cond_data_pushed_to_queue);

    cleanup1:
    printf("Shutting down.\n");
    close(sockfd);
    exit(return_param);

    //End




}


//helper functs
void check_argc(int argc) {
    if (argc != 3) {
        printf("usage: ."__FILE__" < 1 port >= 1024 and number of request handlers>");
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
    while (true) {
        errno = 0;
        if ((conn_sockfd = accept(threadStruct_PTR->sockfd, NULL, NULL)) == -1) {
            perror("Accept");
            // ? how to handle failure - cleanup
            //return_param = EXIT_FAILURE;
            //goto cleanup2;
        }

        //fprintf(stderr, "In Listener Tread: Accept: Established connection!\n");

        pthread_error_funct(pthread_mutex_lock(threadStruct_PTR->mutex_queue_PTR));
        myqueue_push(&threadStruct_PTR->queue, conn_sockfd);
        if(myqueue_is_empty(&threadStruct_PTR->queue)){
            fprintf(stderr, "QUEUE EMPTY EVEN AFTER PUSH\n");
        }
        pthread_cond_signal(&threadStruct_PTR->cond_data_pushed_to_queue);
        //fprintf(stderr, "In Listener Tread: Signaled the accept!\n");
        pthread_mutex_unlock(threadStruct_PTR->mutex_queue_PTR);

    }
}

void* request_handler(void* arg){
    /**
    Pop a client connection from the queue.
        If the popped connection is a poison value (e.g. -1), return.
        -> here poison value refers to a special value used to signal a specific condition.
        Otherwise, receive and “parse” the HTTP request. You only need the first line of the request for this (see below).
    To simulate a real workload, sleep for 100 milliseconds.
    Send a response to the client:
        When a GET / request is received, sends a response with a small HTML response body of your own choosing. Be sure to set the correct size in the Content-Length header field. (see below)
        When a POST /donate request is received, the server incements a double variable by the donation amount received in the POST body. Subsequently, it returns a message acknowledging the donation and stating the current balance (e.g. Thank you very much for donating $13.500000! The balance is now $49.500000.).
        When a POST /shutdown request is received, the listener thread is terminated. Since accept is blocking, one way of implementing this is to use the pthread_cancel(3) function.
        Otherwise, you can either send the same response as for GET / or a custom error response (e.g. with HTTP status code 501 Not Implemented).
    Close the connection.
     */
    thread_struct_t *threadStruct_PTR = (thread_struct_t *) arg;

    //fprintf(stderr, "in request handler\n");

    while(true) {
        int conn_sockfd;
        pthread_error_funct(pthread_mutex_lock(threadStruct_PTR->mutex_queue_PTR));

        while ((myqueue_is_empty(&threadStruct_PTR->queue)) == true) {             //catch for spurious wakeup!
            //fprintf(stderr, "before cond_wait\n");
            pthread_error_funct(pthread_cond_wait(&threadStruct_PTR->cond_data_pushed_to_queue, threadStruct_PTR->mutex_queue_PTR));
            //fprintf(stderr, "Wakes up from pthread_cond_wait\n");
        }

        conn_sockfd = myqueue_pop(&threadStruct_PTR->queue);
        //fprintf(stderr, "poped conn_sockfd from queue\n");
        pthread_mutex_unlock(threadStruct_PTR->mutex_queue_PTR);

        if (conn_sockfd == -1) // * this is poison value
        {
            pthread_exit(NULL);
        }

        char buffer[BUFFER_SIZE];
        recv(conn_sockfd, buffer, BUFFER_SIZE, 0);

        char method[10], url[100];
        sscanf(buffer, "%s %s", method, url);




        //Region GET
        if(strcmp(method, "GET") == 0 && strcmp(url, "/") == 0) {

            char get_response_body[200];
            int get_length = sprintf(get_response_body, "<html><body><h1>Welcome to my web server!</h1></body></html>");

            char response[BUFFER_SIZE];
            sprintf(response, "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/html\r\n"
                              //"Content-Type: text/plain\r\n"
                              "Content-Length: %d\r\n\r\n"
                              "<html><body><h1>Welcome to my web server!</h1></body></html>",
                    get_length);
            usleep(100000); // sleep for 100 milliseconds
            send(conn_sockfd, response, strlen(response), 0);
        }

        //Region POST /donate
        else if(strcmp(method, "POST") == 0 && strcmp(url, "/donate") == 0) {

            char* beforecontentLength = strstr(buffer, "Content-Length: ");
            char* afterContentLength = beforecontentLength + strlen("Content-Length: ");

            char contentLength[10];
            sscanf(afterContentLength, "%s", contentLength);
            //fprintf(stderr, "Content Length: %s\n", contentLength);
            unsigned long long contentLengthNumber = cast_to_ulli_with_check(contentLength);

            //fprintf(stderr, "%llu\n", contentLengthNumber);

            char content[contentLengthNumber +1 ];
            content[contentLengthNumber] = '\0';

            char* contentBody = strstr(buffer, "\r\n\r\n");
            sscanf(contentBody, "%s", content);

            //fprintf(stderr, "%s", content);


            double amount = cast_to_double_with_check(content);


            pthread_error_funct(pthread_mutex_lock(threadStruct_PTR->mutex_queue_PTR));
            threadStruct_PTR->total_donations += amount;
            double balance = threadStruct_PTR->total_donations;
            pthread_mutex_unlock(threadStruct_PTR->mutex_queue_PTR);

            //char response[100];
            char response_body[200];
            int length = sprintf(response_body, "Thank you very much for donating $%.6f The balance is now $%.6f.\n", amount, balance);

            char response[BUFFER_SIZE];
            sprintf(response, "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/plain\r\n"
                              "Content-Length: %d\r\n\r\n"
                              "Thank you very much for donating $%.6f The balance is now $%.6f.\n",
                    length, amount, balance);
            usleep(100000); // sleep for 100 milliseconds
            send(conn_sockfd, response, strlen(response), 0);
        }

        //Region POST /shutdown
        else if(strcmp(method, "POST") == 0 && strcmp(url, "/shutdown") == 0) {
            //pthread_exit(NULL);
            // kill the LISTENER thread with e.g. pthread_cancel()
            //this causes the server (that waits for the listener thread) -> to push num_request_handler poisen values into the queue
            char response_body[200];
            int length = sprintf(response_body, "Server shutting down...\n");

            char response[BUFFER_SIZE];
            sprintf(response, "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/plain\r\n"
                              "Content-Length: %d\r\n\r\n"
                              "Server shutting down...\n",
                    length);
            usleep(100000); // sleep for 100 milliseconds
            send(conn_sockfd, response, strlen(response), 0);





            pthread_cancel(threadStruct_PTR->listener_tid);
            //accept() is a cancellation point
        }

        //Region Else
        else {
            char response[] = "HTTP/1.1 501 Not Implemented\r\n"
                              "Content-Type: text/plain\r\n"
                              "Content-Length: 15\r\n\r\n"
                              "Not Implemented";

            usleep(100000); // sleep for 100 milliseconds
            send(conn_sockfd, response, strlen(response), 0);
        }

        close(conn_sockfd);
        usleep(100000); // sleep for 100 milliseconds
    }
        
        
}

double cast_to_double_with_check(char* buf) {

    errno = 0;
    char *end = NULL;
    double operand = strtod(buf, &end);

    //check conversion:
    if ((*end != '\0' && *end != '\n') || (buf == end)) {       //conversion interrupted || no conversion happened
        perror("Strtod");
        exit(EXIT_FAILURE);
    }

    if (errno != 0) {        //== ERANGE //as alternative to != 0
        //printf("Overflow or underflow occurred.");
        perror("Conversion of argument ended with error");
        fprintf(stderr, "usage: ."__FILE__" <number of files> \n");
        exit(EXIT_FAILURE);
    }
    return operand;
}
