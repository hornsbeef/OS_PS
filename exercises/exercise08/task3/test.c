#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <queue>
#include <iostream>

#define PORT 8080
#define N 5
#define BUFFER_SIZE 1024

std::queue<int> clientConnections;
double balance = 0.0;

void* requestHandler(void* arg) {
    while(true) {
        int clientSocket = clientConnections.front();
        clientConnections.pop();

        if(clientSocket == -1) {
            break;
        }

        char buffer[BUFFER_SIZE];
        recv(clientSocket, buffer, BUFFER_SIZE, 0);

        char method[10], url[100];
        sscanf(buffer, "%s %s", method, url);

        if(strcmp(method, "GET") == 0 && strcmp(url, "/") == 0) {
            char response[] = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/html\r\n"
                              "Content-Length: 46\r\n\r\n"
                              "<html><body><h1>Welcome to my web server!</h1></body></html>";
            send(clientSocket, response, strlen(response), 0);
        } else if(strcmp(method, "POST") == 0 && strcmp(url, "/donate") == 0) {
            char amountBuffer[100];
            recv(clientSocket, amountBuffer, 100, 0);
            double amount = atof(amountBuffer);
            balance += amount;
            char response[100];
            sprintf(response, "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/plain\r\n"
                              "Content-Length: %d\r\n\r\n"
                              "Thank you very much for donating $%.6f The balance is now $%.6f.",
                    strlen(response) - 35, amount, balance);
            send(clientSocket, response, strlen(response), 0);
        } else if(strcmp(method, "POST") == 0 && strcmp(url, "/shutdown") == 0) {
            pthread_exit(NULL);
        } else {
            char response[] = "HTTP/1.1 501 Not Implemented\r\n"
                              "Content-Type: text/plain\r\n"
                              "Content-Length: 23\r\n\r\n"
                              "Not Implemented";
            send(clientSocket, response, strlen(response), 0);
        }

        close(clientSocket);
        usleep(100000); // sleep for 100 milliseconds
    }

    return NULL;
}

void* listener(void* arg) {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);

    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    listen(serverSocket, 3);

    while(true) {
        struct sockaddr_in clientAddress;
        socklen_t clientLength = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientLength);
        clientConnections.push(clientSocket);
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    if(argc != 3) {
        printf("Usage: %s <port> <N>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int n = atoi(argv[2]);

    pthread_t listenerThread;
    pthread_create(&listenerThread, NULL, listener, NULL);

    pthread_t requestHandlerThreads[n];
    for(int i = 0; i < n; i++) {
        pthread_create(&requestHandlerThreads[i], NULL, requestHandler, NULL);
    }

    pthread_join(listenerThread, NULL);

    for(int i = 0; i < n; i++) {
        clientConnections.push(-1);
    }

    for(int i = 0; i < n; i++) {
        pthread_join(requestHandlerThreads[i], NULL);
    }

    return 0;
}
