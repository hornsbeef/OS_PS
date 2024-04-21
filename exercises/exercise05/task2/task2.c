/*
 * Linking
       Programs using the POSIX shared memory API must be compiled with
       cc -lrt to link against the real-time library, librt.
 */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

void check_argc(int argc);
unsigned long long int cast_to_ulli_with_check(char* string);

void clean_before_exit(const char *name, int fd);
void fork_error_check(pid_t pid);
void validate_result(uint64_t result, const uint64_t K, const uint64_t N);

typedef struct TODO {
    uint64_t result;
    uint64_t buffer[];
}TODO_t;


int main(int argc, char* argv[]) {
    check_argc(argc);

    uint64_t N = cast_to_ulli_with_check(argv[1]);     //N, an arbitrary integer
    uint64_t K = cast_to_ulli_with_check(argv[2]);     //K, number of reads/writes to the buffer
    uint64_t L = cast_to_ulli_with_check(argv[3]);     //L, the length of the circular buffer (total size: L * sizeof(uint64_t))
    uint64_t buffersize = L * sizeof(uint64_t);     //TODO: WHY THIS STILL WORK WITH 0 ???


    //TODO: for dev only:
    //fprintf(stderr, "N = %lu\n", N);
    //fprintf(stderr, "K = %lu\n", K);
    //fprintf(stderr, "L = %lu\n", L);
    //fprintf(stderr, "buffersize = %lu\n", buffersize);


    //TODO:
    //set up shared memory for communication.
    // It contains a circular buffer and one element for the result.

    //shm_open()
    errno = 0;
    const char* name = "/shared_memory";
    const int oflag = O_RDWR | O_CREAT | O_EXCL;
    const mode_t permission = S_IRUSR | S_IWUSR;
    int fd = shm_open(name, oflag, permission);
    if(fd<0){
        perror("shm_open");
        clean_before_exit(name, fd);
        exit(EXIT_FAILURE);
    }

    //ftruncate()
    const size_t shared_mem_size = sizeof(TODO_t) + buffersize;      //TODO: chekc if this is correct -> something strange going on here!!
    errno = 0;
    int ftrunc_error = ftruncate(fd, shared_mem_size);
    if(ftrunc_error < 0){
        perror("ftruncate");
        clean_before_exit(name, fd);
        exit(EXIT_FAILURE);
    }

    //mmap()    //todo: why here on slides char* instead of void* ?
    /*
     * After the mmap() call has returned, the file descriptor, fd, can
       be closed immediately without invalidating the mapping. https://man7.org/linux/man-pages/man2/mmap.2.html
     */
    errno=0;
    TODO_t* todo_ptr = mmap(NULL, shared_mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(todo_ptr == MAP_FAILED){
        perror("mmap");
        clean_before_exit(name, fd);
        exit(EXIT_FAILURE);
    }

/*
    //TODO:
    todo_ptr->buffer[buffersize+1] = (uint64_t) 33;                     //TODO: WHY IS THIS WORKING???
    fprintf(stderr,"WHY THIS WORKING? %lu\n", todo_ptr->buffer[buffersize+1]);//TODO: WHY IS THIS WORKING???
    todo_ptr->result = 123;
    fprintf(stderr,"WHY THIS WORKING? %lu\n", todo_ptr->result);//TODO: WHY IS THIS WORKING???
    //TODO:END

    for (int i = 0; i < 99999; ++i) {       //gets to 510
        todo_ptr->buffer[i] =  i;                     //TODO: WHY IS THIS WORKING???
        fprintf(stderr,"WHY THIS WORKING? %lu\n", todo_ptr->buffer[i]);//TODO: WHY IS THIS WORKING???

    }
*/

    //Next, two child processes are created.
    //The processes run in parallel and perform calculations on the buffer:

    for (int i = 0; i < 2; ++i) {
        errno = 0;
        pid_t pid = fork();
        fork_error_check(pid);
        if(pid == 0){
            switch (i) {
                case 0: {
                    for (uint64_t j = 0; j < K; ++j) {
                        int number = N * (j+1);
                        todo_ptr->buffer[j%L] = number;
                    }
                    exit(EXIT_SUCCESS);
                    break;
                }
                case 1:{
                    /*The process computes the sum of each element in the circular buffer.
                     * It prints the final result, and writes it into the result element in the shared memory.
                     */

                    uint64_t temp =0;
                    for (uint64_t j = 0; j < K; ++j) {
                       temp +=  todo_ptr->buffer[j%L];
                    }
                    todo_ptr->result = temp;
                    exit(EXIT_SUCCESS);
                    break;
                }
            }
        }
    }

    //this is parent:
    for (int i = 0; i < 2; ++i) {
        errno = 0;
        int wait_error = wait(NULL);
        if(wait_error<0){
            perror("Wait: ");
            clean_before_exit(name, fd);
            exit(EXIT_FAILURE);
        }
    }

    printf("Result: %lu\n", todo_ptr->result);
    validate_result(todo_ptr->result, K, N);


    clean_before_exit(name, fd);
    exit(EXIT_SUCCESS);


}

void validate_result(uint64_t result, const uint64_t K, const uint64_t N) {
    for (uint64_t i = 0; i < K; i++) {
        result -= N * (i + 1);
    }
    printf("Checksum: %lu \n", result);
}


void fork_error_check(pid_t pid) {
    if(pid < 0){
        //todo: set errno = 0 before fork() call!
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }
}



void clean_before_exit(const char *name, int fd) {
    close(fd);
    shm_unlink(name);
}

void check_argc(int argc) {
    if (argc < 4 || argc > 4) {
        printf("usage: ."__FILE__" < 3  ints>");
        exit(EXIT_FAILURE);
    }
}

unsigned long long int cast_to_ulli_with_check(char* string) {
    errno = 0;
    char *end = NULL;
    unsigned long long operand = strtoull(string, &end, 10);
    //check conversion:
    if ((*end != '\0') || (string == end)) {       //conversion interrupted || no conversion happened
        perror("StrToULL");
        exit(EXIT_FAILURE);
    }
    if (errno != 0) {        //== ERANGE //as alternative to != 0
        perror("Conversion of argument ended with error");
        fprintf(stderr, "usage: ."__FILE__" <number of files> \n");
        exit(EXIT_FAILURE);
    }
    return operand;
}


