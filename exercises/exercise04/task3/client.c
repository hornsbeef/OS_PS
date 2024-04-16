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
    char input[PIPE_BUF];
    //You can assume an expression to be at most PIPE_BUF long. Why is this important?
    //PIPE_BUF : from https://man7.org/linux/man-pages/man7/pipe.7.html
    //POSIX.1 says that writes of less than PIPE_BUF bytes must be
    //atomic: the output data is written to the pipe as a contiguous
    //sequence.  Writes of more than PIPE_BUF bytes may be nonatomic:
    //the kernel may interleave the data with data written by other
    //processes.

    while(cycle){
        printf("Expression: ");
        //int n_scanned = scanf("%s\n", input );
        fgets(input, PIPE_BUF, stdin);

        if (strcmp(input, "\n")==0){
                //nothing was entered
                //close the connection & break the loop
                close(pipe_fd);
                cycle = false;
                break;
        }else{
            //something was entered -> send to pipe, content must be handled by server.
            write(pipe_fd, input, strlen(input));
        }

    }


    exit(EXIT_SUCCESS);


}


