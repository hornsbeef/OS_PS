#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>

#define READ_END 0
#define WRITE_END 1

void pipe_error_check(int pipe_error);

void fork_error_check(pid_t pid);

void dup_error_checker(int dup2_error);

void child_1(char *data, const int *pipe_fd_1);

int main() {
    //In the dup2(one, two) function,
    // the file descriptor one is duplicated onto the file descriptor two.
    // This means that the file descriptor one is copied to the file descriptor two,
    // and the file descriptor two now refers to the same file as one.
    // The original file descriptor two is replaced by the new file descriptor
    // that points to the same file as one.
    //char* data = "./task2/file.txt";    //todo: change to other
    char* data = "./file.txt";

    int dup2_error;


    int pipe_fd_1[2];
    errno = 0;
    int pipe_error = pipe(pipe_fd_1);  //creates pipe
    pipe_error_check(pipe_error);

    errno = 0;
    pid_t pid = fork();
    fork_error_check(pid);

    switch(pid){
        case 0:{
            child_1(data, pipe_fd_1);
            break;
        }

        default:{
            //this is parent:

            //waitpid(pid, NULL, 0);

            close(pipe_fd_1[WRITE_END]);  //close write end of pipe

            errno = 0;
            dup2_error = dup2(pipe_fd_1[READ_END], STDIN_FILENO);  //for redirecting input!
            dup_error_checker(dup2_error);

            char *args[] = {"cut", "-c","22-", NULL};
            errno = 0;
            execvp(args[0], args);

            //if execvp failed this code will be used:
            //The exec() functions return only if an error has occurred.
            // The return value is -1, and errno is set to indicate the error.
            //set errno = 0; before execvp() call!
            perror("Exec failed");
            exit(EXIT_FAILURE);

        }
    }
}

void child_1( char *data, const int *pipe_fd_1) {
    //this is child_1


    int pipe_fd_2[2];
    errno = 0;
    int pipe_error = pipe(pipe_fd_2);  //creates pipe
    pipe_error_check(pipe_error);

    errno = 0;
    pid_t pid_2 = fork();
    fork_error_check(pid_2);

    switch (pid_2) {
        case 0: {
            //this is child_2:
            close(pipe_fd_2[READ_END]);   //close read end of pipe_2

            {//close all pipe_1 connections
                close(pipe_fd_1[READ_END]);
                close(pipe_fd_1[WRITE_END]);
            }


            {//for redirecting STDOUT to pipe_2 Write_End
            errno = 0;
            int dup2_error = dup2(pipe_fd_2[WRITE_END], STDOUT_FILENO);
            dup_error_checker(dup2_error);
            }

            {
            char *args[] = {"cat", data, NULL};
            errno = 0;
            execvp(args[0], args);
            //if execvp failed this code will be used:
            //The exec() functions return only if an error has occurred.
            // The return value is -1, and errno is set to indicate the error.
            //set errno = 0; before execvp() call!
            perror("Exec failed");
            exit(EXIT_FAILURE);
            }

        }
        default: {
            //this is child_1:

            {
            close(pipe_fd_1[READ_END]); //close read end of pipe_1
            close(pipe_fd_2[WRITE_END]); //close write_end of pipe_2
            }

            {
            errno = 0;
            int dup2_error = dup2(pipe_fd_1[WRITE_END], STDOUT_FILENO);  //for redirecting output!
            dup_error_checker(dup2_error);
            }

            {
            errno = 0;
            int dup2_error = dup2(pipe_fd_2[READ_END], STDIN_FILENO);  //for redirecting input!
            dup_error_checker(dup2_error);
            }


            {
            char *args[] = {"grep", "^;", NULL};    //CAVE: '^;' is "^;" here!!!
            errno = 0;
            execvp(args[0], args);

            //if execvp failed this code will be used:
            //The exec() functions return only if an error has occurred.
            // The return value is -1, and errno is set to indicate the error.
            //set errno = 0; before execvp() call!
            perror("Exec failed");
            exit(EXIT_FAILURE);
            }


        }

    }




}

void dup_error_checker(int dup2_error) {
    if(dup2_error < 0){
        //todo: set errno = 0 before dup2() call!
        perror("dup failed");
        exit(EXIT_FAILURE);
    }
}

void fork_error_check(pid_t pid) {
    if(pid < 0){
        //todo: set errno = 0 before fork() call!
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }
}

void pipe_error_check(int pipe_error) {
    if(pipe_error < 0){
        //set errno = 0 before pipe() call!
        perror("Pipe failed");
        exit(EXIT_FAILURE);
    }
}
