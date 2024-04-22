#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE
// #define _BSD_SOURCE

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdatomic.h>
#include <getopt.h>
#include <limits.h>
#include <inttypes.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>

// settings
static const char shm_name[] = "/csbb5051_e05_t2";

#define FORK_AND_CALL(FUNC, PID_PTR, ...)           \
{                                                   \
    *(PID_PTR) = fork();                            \
    switch (*(PID_PTR))                             \
    {                                               \
        /*error*/                                   \
        case -1: break;                             \
        /*child*/                                   \
        case 0:                                     \
        {                                           \
            (FUNC)(__VA_ARGS__);                    \
            printf("exit");_exit(EXIT_SUCCESS);                    \
        }                                           \
        /*parent*/                                  \
        default: break;                             \
    }                                               \
}

typedef void (*process_func)(void*);

typedef struct shared_struct
{
    uint64_t result;
    uint64_t buffer[];
} shared_struct;

typedef struct shared_memory
{
    int fd;
    shared_struct* ptr;
} shared_memory;

// returns EXIT_SUCCESS if successful
int strtoint64_t(const char* str, int64_t* out)
{
    char* endptr;
    long temp = strtol(str, &endptr, 10);
    if (*endptr != '\0' && *endptr != EOF)      return EXIT_FAILURE;
    if (temp < INT64_MIN || temp > INT64_MAX)   return EXIT_FAILURE;
    // success
    *out = (int64_t)temp;
    return EXIT_SUCCESS;
}

// EXIT_SUCCESS on success
// no allocated resources on failure
int init_shared_struct(shared_memory* shared, bool create, int L)
{
    const int open_oflag = O_RDWR;
    const int create_oflag = O_CREAT | O_EXCL | O_RDWR;
    const int oflag = create ? create_oflag : open_oflag;
    const mode_t permissions = S_IRUSR | S_IWUSR; // 600
    shared->fd = shm_open(shm_name, oflag, permissions);
    if (shared->fd < 0)
    {
        perror("opening shared memory failed");
        return EXIT_FAILURE;
    }

    const size_t shm_size = sizeof(shared_struct) + sizeof(int64_t) * L;

    if (create && ftruncate(shared->fd, shm_size) != 0)
    {
        close(shared->fd);
        perror("failed resizing shared memory");
        return EXIT_FAILURE;
    }

    shared->ptr = mmap(NULL, shm_size,
        PROT_READ | PROT_WRITE, MAP_SHARED, shared->fd, 0);
    if (shared->ptr == MAP_FAILED)
    {
        close(shared->fd);
        perror("shared memory mapping failed");
        return EXIT_FAILURE;
    }

    //RingBuffer: JUST TESTING
    shared->ptr->buffer[shm_size+1] = 33;
    fprintf(stderr, "shm_size =  %lu\n", shm_size);
    fprintf(stderr, "TESTING: Buffer[shm_size] = %lu\n", shared->ptr->buffer[shm_size+1]);




    return EXIT_SUCCESS;
}

int child_A(int64_t N, int64_t K, int64_t L)
{
    shared_memory shared;
    if(init_shared_struct(&shared, false, L) != EXIT_SUCCESS)
    {
        _exit(EXIT_FAILURE);
    }

    for (int64_t i = 0; i < K; ++i)
    {
        shared.ptr->buffer[i % L] = N * (i + 1);
    }

    // cleanup
    close(shared.fd);

    return EXIT_SUCCESS;
}

int child_B(int64_t L)
{
    shared_memory shared;
    if(init_shared_struct(&shared, false, L) != EXIT_SUCCESS)
    {
        _exit(EXIT_FAILURE);
    }

    for (int64_t i = 0; i < L; ++i)
    {
        shared.ptr->result += shared.ptr->buffer[i];
    }
    printf("Result: %"PRId64"\n", shared.ptr->result);

    //sleep(10);

    return EXIT_SUCCESS;
}

void validate_result(uint64_t result, const uint64_t K, const uint64_t N) {
    for (uint64_t i = 0; i < K; i++) {
        result -= N * (i + 1);
    }
    printf("Checksum: %lu \n", result);
}

int main(int argc, char* argv[])
{
    int return_val = EXIT_SUCCESS;

    if (argc != 4)
    {
        fprintf(stderr, "usage: %s <N> <K> <L>.\n", argv[0]);
        return EXIT_FAILURE;
    }

    // parse args
    int64_t N, K, L;

    if( strtoint64_t(argv[1], &N) == EXIT_FAILURE ||
        strtoint64_t(argv[2], &K) == EXIT_FAILURE ||
        strtoint64_t(argv[3], &L) == EXIT_FAILURE)
    {
        fprintf(stderr, "invalid arg.\n");
    }

    // create shared mem
    shared_memory shared;
    if(init_shared_struct(&shared, true, L) != EXIT_SUCCESS)
    {
        // will close shared memory with specified name, even if created by other process
        // but else will only be released on OS restart -> program won't work if failed
        // to reach cleanup ONCE
        shm_unlink(shm_name);
        return EXIT_FAILURE;
    }

    int child_A_pid, child_B_pid;
    FORK_AND_CALL(child_A, &child_A_pid, N, K, L);
    FORK_AND_CALL(child_B, &child_B_pid, L);
    if (child_A_pid == -1 || child_B_pid == -1)
    {
        perror("fork failed");
        return_val = EXIT_FAILURE;
        goto cleanup;
    }

    while(wait(NULL) > 0);

    validate_result(shared.ptr->result, K, N);

cleanup:
    close(shared.fd);
    shm_unlink(shm_name);
    return return_val;
}
