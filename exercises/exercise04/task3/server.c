#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct client {
    char name[256];
    int counter;

} client_t;
void pipe_error_check(int pipe_error);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: ."__FILE__ " <client names>");
        exit(EXIT_FAILURE);
    }

    client_t clients[argc];
    mode_t client_server_fifo_perm = (S_IRUSR | S_IWUSR);


    for (int i = 1; i < argc; ++i) {
        strcpy(clients[i].name, argv[i]);
        clients[i].counter = 0;

        errno = 0;
        int fifo_error = mkfifo(clients[i].name, client_server_fifo_perm);
        pipe_error_check(fifo_error);
        
    }





}
void pipe_error_check(int pipe_error) {
    if(pipe_error < 0){
        //todo: set errno = 0 before pipe() call!
        perror("Pipe failed");
        exit(EXIT_FAILURE);
    }
}