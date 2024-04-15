#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <linux/limits.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char* argv[]){

    if (argc != 2) {
        fprintf(stderr, "usage: ."__FILE__ " <client name provided by server (in order!)>");
        exit(EXIT_FAILURE);
    }

    printf("Now opening file - Please wait.\n");
    fflush(stdout);
    errno = 0;
    int pipe_fd= open(argv[1], O_WRONLY);
    //because no O_NONBLOCK can be used the program waits here until it can open the file without informing the user.
    if(pipe_fd < 0){
        perror("Open failed");
        exit(EXIT_FAILURE);
    }else{
        printf("Successfully opened file.\n");
        fflush(stdout);
    }


    bool cycle = true;
    char input[PIPE_BUF];   //this is why it is important to know how long the input is going to be.

    while(cycle){
        printf("Expression: ");
        //int n_scanned = scanf("%s\n", input );
        fgets(input, PIPE_BUF, stdin);

        //todo: debugging only:
        fprintf(stdout, "this was entered:%s\n", input);
        fflush(stdout);




        if (strcmp(input, "\n")==0){
                //nothing was entered
                //todo: close the connection & break the loop
                close(pipe_fd);
                cycle = false;
                break;
        }else{
            //something was entered -> send to pipe, content must be handled by server.
            write(pipe_fd, input, strlen(input));
        }

    }


    fprintf(stderr, "EXITING...");
    fflush(stderr);
    exit(EXIT_SUCCESS);


}


