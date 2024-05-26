#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define MAX_USERNAME_LEN 20
#define MAX_MESSAGE_LEN 256

typedef struct {
    int sockfd;
    char username[MAX_USERNAME_LEN];
    int is_admin;
} client_t;

client_t clients[MAX_CLIENTS];
int num_clients = 0;
char *admin_usernames[5];
int num_admins = 0;
int server_socket;
pthread_t listener_thread;

void *client_thread(void *arg) {
    client_t *client = (client_t *)arg;
    char buffer[MAX_MESSAGE_LEN];

    while (1) {
        ssize_t bytes_read = recv(client->sockfd, buffer, sizeof(buffer), 0);
        if (bytes_read <= 0) {
            break;
        }

        buffer[bytes_read - 1] = '\0';  // Remove newline character
        printf("<%s>: %s\n", client->username, buffer);
    }

    for (int i = 0; i < num_clients; i++) {
        if (clients[i].sockfd == client->sockfd) {
            num_clients--;
            memmove(&clients[i], &clients[i + 1], (num_clients - i) * sizeof(client_t));
            break;
        }
    }

    close(client->sockfd);
    printf("%s has disconnected.\n", client->username);
    return NULL;
}

void *listener_thread_func(void *arg) {
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);

        if (client_socket < 0) {
            perror("accept");
            continue;
        }

        char username[MAX_USERNAME_LEN];
        ssize_t bytes_read = recv(client_socket, username, sizeof(username), 0);
        if (bytes_read <= 0) {
            close(client_socket);
            continue;
        }

        username[bytes_read - 1] = '\0';  // Remove newline character

        int is_admin = 0;
        for (int i = 0; i < num_admins; i++) {
            if (strcmp(username, admin_usernames[i]) == 0) {
                is_admin = 1;
                break;
            }
        }

        clients[num_clients].sockfd = client_socket;
        strncpy(clients[num_clients].username, username, MAX_USERNAME_LEN - 1);
        clients[num_clients].is_admin = is_admin;
        num_clients++;

        pthread_t client_thread_id;
        pthread_create(&client_thread_id, NULL, client_thread, &clients[num_clients - 1]);
        pthread_detach(client_thread_id);

        printf("%s has connected. (Admin: %d)\n", username, is_admin);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <port> <admin_username1> [<admin_username2> ...]\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    for (int i = 2; i < argc; i++) {
        admin_usernames[num_admins++] = argv[i];
    }

    struct sockaddr_in server_addr;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_socket);
        return 1;
    }

    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("listen");
        close(server_socket);
        return 1;
    }

    pthread_create(&listener_thread, NULL, listener_thread_func, NULL);

    while (1) {
        char buffer[MAX_MESSAGE_LEN];
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = '\0';  // Remove newline character

        if (strcmp(buffer, "/shutdown") == 0) {
            printf("Server is shutting down.\n");
            pthread_cancel(listener_thread);
            break;
        }
    }

    pthread_join(listener_thread, NULL);

    for (int i = 0; i < num_clients; i++) {
        close(clients[i].sockfd);
    }

    close(server_socket);
    printf("Server has shut down.\n");
    return 0;
}
