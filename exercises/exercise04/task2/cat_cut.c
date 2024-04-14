#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>  //for pid_t needed

#define READ_END 0
#define WRITE_END 1

void pipe_error_check(int pipe_error);

void fork_error_check(pid_t pid);

void dup_error_checker(int dup2_error);

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




    int pipe_fd[2];
    errno = 0;
    int pipe_error = pipe(pipe_fd);  //creates pipe
    pipe_error_check(pipe_error);

    errno = 0;
    pid_t pid = fork();
    fork_error_check(pid);

    switch(pid){
        case 0:{
            //this is child:
            close(pipe_fd[READ_END]);   //close read end of pipe
            char *args[] = {"cat", data, NULL};

            errno = 0;
            dup2_error = dup2(pipe_fd[WRITE_END], STDOUT_FILENO);  //for redirecting output!
            dup_error_checker(dup2_error);
            //In the dup2(one, two) function,
            // the file descriptor one is duplicated onto the file descriptor two.
            // This means that the file descriptor one is copied to the file descriptor two,
            // and the file descriptor two now refers to the same file as one.
            // The original file descriptor two is replaced by the new file descriptor
            // that points to the same file as one.


            errno = 0;
            execvp(args[0], args);

            //if execvp failed this code will be used:
            //The exec() functions return only if an error has occurred.
            // The return value is -1, and errno is set to indicate the error.
            //set errno = 0; before execvp() call!
            perror("Exec failed");
            exit(EXIT_FAILURE);

        }
        default:{
            //this is parent:
            close(pipe_fd[WRITE_END]);  //close write end of pipe
            //todo: use input from pipe for cut -c 22-

            errno = 0;
            dup2_error = dup2(pipe_fd[READ_END], STDIN_FILENO);  //for redirecting input!
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








    //char *cut_args[] = {"cut", "-c", "22-",NULL};
    //execvp(cut_args[0], cut_args);

    return 0;
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
