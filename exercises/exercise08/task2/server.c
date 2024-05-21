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
#include <math.h>
#include <ctype.h>


void check_argc(int argc);
unsigned long long int cast_to_ulli_with_check(char *string);
double cast_to_double_with_check(char* buf, jmp_buf msg_rev_loop) ;


// ! no global variables must be used -> no idea how to handle signals without a global flag.

int main(int argc, char *argv[]) {
    //Region check args
    volatile int return_param = EXIT_SUCCESS;
    volatile double total_donations = 0;

    check_argc(argc);

    unsigned long long port = cast_to_ulli_with_check(argv[1]);
    if(port < 1024 || port >65535){
        printf("usage: ."__FILE__" < 1 port between 1024 - 65535 >\n");
        exit(EXIT_FAILURE);
    }
    //End

    //Region socket init - listen

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


    //const struct sockaddr_in addr = {       //posted struct did NOT work
    //        .sin_family = AF_INET,
    //        .sin_addr = htonl(INADDR_ANY),
    //        //.sin_addr = htonl(INADDR_LOOPBACK),
    //        .sin_port = htons(port),
    //};

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);


    //bind is for server; //connect is for client
    if(bind(sockfd, (const struct sockaddr*) &addr, sizeof(addr) ) != 0 ){
        perror("Bind");
        return_param = EXIT_FAILURE;
        goto cleanup1;
    }

    // Listen for incoming connections
    if (listen(sockfd, 1) < 0) {
        perror("Error listening on socket");
        return_param = EXIT_FAILURE;
        goto cleanup1;
    }

    printf("Listening on port %llu.\n", port);

    //End

    //Region client conn
/**
 * In the accept() function in C, the addr and addrlen arguments can be set to NULL when you are not interested
 * in capturing the address information of the connecting client.
 * This is commonly done when you only need the new socket descriptor
 * for communication and do not require details about the client's address.
 */
    int conn_sockfd;
    while (true) {
        errno = 0;
        if ((conn_sockfd = accept(sockfd, NULL, NULL)) == -1) {
            perror("Accept");
            return_param = EXIT_FAILURE;
            goto cleanup2;
        }

        printf("Established connection!\n");


        char buffer[1024];
        jmp_buf msg_rec_loop;

        if(setjmp(msg_rec_loop) == 1){
            dprintf(conn_sockfd, "%s is not a valid amount.\n", buffer);
        }

        while(true){        // * continuously receiving msg
            memset(buffer, '\0', sizeof(*buffer));
            errno = 0;
            ssize_t bytes_received = recv(conn_sockfd, buffer, sizeof(buffer), 0);
            if(bytes_received == -1){
                perror("Recv");
                return_param = EXIT_FAILURE;
                goto cleanup2;
            }
            if(bytes_received == 0){    // * When a stream socket peer has performed an orderly shutdown, the return value will be 0 (the traditional "end-of-file" return).
                printf("Connection closed by client.\n");
                printf("Waiting for new connection...\n");
                // * could reset total_donations to 0 here, no idea if I should.
                break;
            }

            // * replace \n with \0 in char buffer[]
            buffer[strcspn(buffer, "\n")] = '\0';
            if(strcmp(buffer, "/shutdown") == 0){
                printf("Shutting down.\n"
                       "Closing connection ...\n");
                goto cleanup2;
            }
            double donation = cast_to_double_with_check(buffer, msg_rec_loop);


            total_donations += donation;
            dprintf(conn_sockfd, "Donations: %.2f\n", total_donations);

        }
    }



    //End

    //Region cleanup
cleanup2:
    close(conn_sockfd);
    printf("Connection closed.\n");

cleanup1:
    close(sockfd);
    exit(return_param);

    //End


}


//helper functs
void check_argc(int argc) {
    if (argc != 2) {
        printf("usage: ."__FILE__" < 1 port >= 1024 >");
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
double cast_to_double_with_check(char* buf, jmp_buf msg_rec_loop) {

    //Region another validity check
    char input_checker[1024];
    //unsigned long dot_value = strcspn(buf, ".");
    //fprintf(stderr, "%s\n",buf+dot_value);
    //if(!(buf[dot_value + 3] == '\0' || buf[dot_value + 3] == '\n')){
    //    fprintf(stderr, "Server: has more than 2 decimal points: %s\n", buf);
    //    longjmp(msg_rec_loop, 1);
    //}
    memcpy(input_checker, buf, sizeof(*buf));
    int i = 0;
    while(buf[i] != '.'){
        i++;
        // * check that it does not run out of bounds
        if(i>= 1021){
            goto nodotinmsg;
        }
    }
    i += 3;
    //testing
    //fprintf(stderr, "buf[i] = %c", buf[i]);

    if(isdigit(buf[i])){
        //fprintf(stderr, "Has more than 2 Decimal places !!! ");
        //fprintf(stderr, "3rd Decimal place is %c\n", buf[i]);
        longjmp(msg_rec_loop, 1);
    }

    nodotinmsg:
    //End

    errno = 0;
    char *end = NULL;
    double operand = strtod(buf, &end);

    //check conversion:
    if ((*end != '\0') && *end != EOF) {       //conversion interrupted || no conversion happened
        //printf("%s is not a valid amount.", buf);
        longjmp(msg_rec_loop, 1);
    }

    //Region new validity check:
    if (operand <= 0) {       // * All donations must be non-negative floating point numbers
        //fprintf(stderr, "%s is counted as smaller than 0.", buf);
        longjmp(msg_rec_loop, 1);
    }

//    double fractionalPart, integralPart;
//    fractionalPart = modf(operand, &integralPart);  // Get the fractional part
//
//    fractionalPart *= 100;
//    double fractionalFractionalPart, fractionalIntegralPart;
//    fractionalFractionalPart = modf(fractionalPart, &fractionalIntegralPart);
//    // Check if the fractional part has no more than 2 decimal places
//    if (fractionalFractionalPart > 0)
//    {
//        //fprintf(stderr, "Fractional part contains more than 2 decimal places\n");
//        longjmp(msg_rec_loop, 1);
//    }
    //End

    //fprintf(stderr, "CLIENT: conversion successful -> return to main\n");
    return operand;


}