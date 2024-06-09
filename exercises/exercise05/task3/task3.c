#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE
// #define _BSD_SOURCE

/*
 * Linking
       Programs using the POSIX shared memory API must be compiled with
       cc -lrt to link against the real-time library, librt.
 */

//Future improvement: include signal handler for cleanup when terminating program
// -> requires partial re-write with inclusion of all parameters in struct for better handling.
// ?how to handle different "times" of crash, when not everything is set up?

//Question:  Why is the other push / pop functionality not working for large numbers?

//#define _GNU_SOURCE     //<<-essential for compiler...
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <string.h>
#include <stdatomic.h>
#include <semaphore.h>
#include <pthread.h>

#define DEBUG 0
#define MUTEX 0

// Global flag to indicate when the signal has been received
volatile sig_atomic_t signal_received = 0;

typedef struct RingBuffer {
    uint64_t head;
    uint64_t tail;
    sem_t free_space_available;
    sem_t data_available;
    pthread_mutex_t mutex_buffer;
    pthread_mutexattr_t mutex_buffer_attr;
    _Atomic uint64_t result;
    uint64_t buffer[];      //must be last in struct!
} RingBuffer;

void check_argc(int argc);

unsigned long long int cast_to_ulli_with_check(char *string);

void shm_clean_before_exit(const char *name, int fd);

bool fork_error_check(pid_t pid);

void validate_result(uint64_t result, const uint64_t K, const uint64_t N);

void ring_buffer_init(RingBuffer *ringBuffer);

bool ring_buffer_is_full(RingBuffer *buf, uint64_t buffersize);

bool ring_buffer_is_empty(RingBuffer *buf);

bool ring_buffer_push(RingBuffer *buf, uint64_t buffersize, uint64_t *data_transfer_in, uint64_t i, uint64_t L);

bool ring_buffer_pop(RingBuffer *buf, uint64_t buffersize, uint64_t *data_transfer, uint64_t i, uint64_t L);

void sem_clean_before_exit(RingBuffer *ring_buffer_ptr);


bool sem_init_error(int sem_ret);


int main(int argc, char *argv[]) {
    check_argc(argc);

    uint64_t N = cast_to_ulli_with_check(argv[1]);     //N, an arbitrary integer
    uint64_t K_debug = cast_to_ulli_with_check(argv[2]);     //K_debug, number of reads/writes to the buffer
    uint64_t L = cast_to_ulli_with_check(
            argv[3]);     //L, the length of the circular buffer (total size: L * sizeof(uint64_t))
    uint64_t buffersize = (L * sizeof(uint64_t));     //RingBuffer: WHY THIS STILL WORK WITH 0 ???


    //RingBuffer: for dev only:
    //fprintf(stderr, "N = %lu\n", N);
    //fprintf(stderr, "K_debug = %lu\n", K_debug);
    //fprintf(stderr, "L = %lu\n", L);
    //fprintf(stderr, "buffersize = %lu\n", buffersize);


    //set up shared memory for communication.
    // It contains a circular buffer and one element for the result.

    //shm_open()
    errno = 0;
    const char *name = "/shared_memory";
    const int oflag = O_RDWR | O_CREAT | O_EXCL;
    const mode_t permission = S_IRUSR | S_IWUSR;
    int fd = shm_open(name, oflag, permission);
    if (fd < 0) {
        perror("shm_open");
        shm_clean_before_exit(name, fd);
        exit(EXIT_FAILURE);
    }

    //ftruncate()
    //const size_t shared_mem_size = sizeof(RingBuffer) + buffersize;
    uint64_t shared_mem_size = sizeof(RingBuffer) + buffersize;
    errno = 0;
    int ftrunc_error = ftruncate(fd, shared_mem_size);  //Todo: maybe error here?
    if (ftrunc_error < 0) {
        perror("ftruncate");
        shm_clean_before_exit(name, fd);
        exit(EXIT_FAILURE);
    }

    //mmap()
    // why here on slides char* instead of void* ? -> because char is exactly size of 1 byte.
    //here more useful for me: RingBuffer
    /*
     * After the mmap() call has returned, the file descriptor, fd, can
       be closed immediately without invalidating the mapping. https://man7.org/linux/man-pages/man2/mmap.2.html
     */
    errno = 0;
    RingBuffer *ring_buffer_ptr = mmap(NULL, shared_mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ring_buffer_ptr == MAP_FAILED) {
        perror("mmap");
        shm_clean_before_exit(name, fd);
        exit(EXIT_FAILURE);
    }


    //initialize Ringbuffer and semaphores:
    ring_buffer_init(ring_buffer_ptr);  //initialize head and tail to 0

    //typedef struct RingBuffer {
    //    uint64_t head;
    //    uint64_t tail;
    //    sem_t free_space_available;
    //    sem_t data_available;
    //    pthread_mutex_t mutex_buffer;
    //    pthread_mutexattr_t mutex_buffer_attr;
    //    _Atomic uint64_t result;
    //    uint64_t buffer[];      //must be last in struct!
    //} RingBuffer;
    errno = 0;
    int sem_ret;
    sem_ret = sem_init(&(ring_buffer_ptr->free_space_available), true, L-1);
    ///starting value is set to L-1 -> because there is L-1 spaces free.
    if (sem_init_error(sem_ret)) {
        shm_clean_before_exit(name, fd);
        exit(EXIT_FAILURE);
    }
    errno = 0;
    sem_ret = sem_init(&(ring_buffer_ptr->data_available), true, 0);
    //starting value is set to 0 -> because no data has been written to the ringbuffer
    if (sem_init_error(sem_ret)) {
        sem_destroy(&(ring_buffer_ptr->free_space_available));//here the previous sem must be destroyed.
        shm_clean_before_exit(name, fd);
        exit(EXIT_FAILURE);
    }
    /*
     * If pshared is nonzero, then the semaphore is shared between
     *  processes, and should be located in a region of shared memory
     */

    //initialize mutex:
    //        fprintf(stderr, "pthread_mutexattr_setpshared failed: %s\n", strerror(ret));
    //needs special attention for usage with shared memory.

    //initialize Attributes of mutex:
    int pma_error = pthread_mutexattr_init(&(ring_buffer_ptr->mutex_buffer_attr));
    if (pma_error != 0) {
        fprintf(stderr, "pthread_mutexattr_init failed: %s\n", strerror(pma_error));
        sem_clean_before_exit(ring_buffer_ptr);
        shm_clean_before_exit(name, fd);
        exit(EXIT_FAILURE);
    }
    pma_error = pthread_mutexattr_setpshared(&(ring_buffer_ptr->mutex_buffer_attr), PTHREAD_PROCESS_SHARED);
    if (pma_error != 0) {
        fprintf(stderr, "pthread_mutexattr_setpshared failed: %s\n", strerror(pma_error));
        sem_clean_before_exit(ring_buffer_ptr);
        shm_clean_before_exit(name, fd);
        exit(EXIT_FAILURE);
    }

    //initialize mutex:
    pthread_mutex_init(&(ring_buffer_ptr->mutex_buffer), &(ring_buffer_ptr->mutex_buffer_attr));
    //always returns 0 -> no checking useful




    //Next, two child processes are created.
    //The processes run in parallel and perform calculations on the buffer:

    for (int i = 0; i < 2; ++i) {
        errno = 0;
        pid_t pid = fork();
        if (fork_error_check(pid)) {
            pthread_mutex_destroy(&(ring_buffer_ptr->mutex_buffer));
            sem_clean_before_exit(ring_buffer_ptr);
            shm_clean_before_exit(name, fd);
            exit(EXIT_FAILURE);

        }
        if (pid == 0) {
            switch (i) {
                case 0: {   //child A:  "Producer"
                    /*The process loops K_debug times, starting from 0.
                     * In each iteration (i), the number N * (i + 1) is written into position i % L of the circular buffer.
                     */
                    uint64_t *data_transfer_in = malloc(sizeof(*data_transfer_in));
                    for (uint64_t j = 0; j < K_debug; ++j) {

                        sem_wait(&(ring_buffer_ptr->free_space_available));
                        //decrements free_space_available by 1, ONLY if currently >0. otherwise waits

#if MUTEX
                        pthread_mutex_lock(&(ring_buffer_ptr->mutex_buffer));
#endif
                        *data_transfer_in = N * (j + 1);    //dont mix data types... (int was causing wrong number!)
                        ring_buffer_push(ring_buffer_ptr, buffersize, data_transfer_in, j, L);
#if MUTEX
                        pthread_mutex_unlock(&(ring_buffer_ptr->mutex_buffer));
#endif

                        sem_post(&(ring_buffer_ptr->data_available));   //increments data_available by 1

                    }
                    free(data_transfer_in);
                    exit(EXIT_SUCCESS);
                    break;
                }
                case 1: {    //child B.  "Consumer"
                    /*The process computes the sum of each element in the circular buffer.
                     * It prints the final result, and writes it into the result element in the shared memory.
                     */

                    uint64_t temp = 0;
                    uint64_t *data_transfer = malloc(sizeof(*data_transfer));
                    for (uint64_t j = 0; j < K_debug; ++j) {

                        sem_wait(&(ring_buffer_ptr->data_available));
                        //decrements data_available by 1, ONLY if currently >0. otherwise waits
#if MUTEX
                        pthread_mutex_lock(&(ring_buffer_ptr->mutex_buffer));
#endif
                        ring_buffer_pop(ring_buffer_ptr, buffersize, data_transfer, j, L);
#if MUTEX
                        pthread_mutex_unlock(&(ring_buffer_ptr->mutex_buffer));
#endif
                        temp += *data_transfer;

                        sem_post(&(ring_buffer_ptr->free_space_available));

                    }
                    free(data_transfer);

                    //after loop: write to struct once.
                    ring_buffer_ptr->result = temp;
                    exit(EXIT_SUCCESS);
                    break;
                }
            }
        }
    }

    //this is parent:
    /*The parent process waits for the termination of both child processes.
     * It reads the result of their computation from the result element in the shared memory. (And prints it.)
     * It then validates the result of the computation using the following function. -> validate_result(uint64_t result, const uint64_t K_debug, const uint64_t N)
     * It then finishes up, and returns success.
     */
    for (int i = 0; i < 2; ++i) {
        errno = 0;
        int wait_error = wait(NULL);
        if (wait_error < 0) {
            perror("Wait: ");
            pthread_mutex_destroy(&(ring_buffer_ptr->mutex_buffer));
            sem_clean_before_exit(ring_buffer_ptr);
            shm_clean_before_exit(name, fd);
            exit(EXIT_FAILURE);
        }
    }

    printf("Result: %lu\n", ring_buffer_ptr->result);
    validate_result(ring_buffer_ptr->result, K_debug, N);


    pthread_mutex_destroy(&(ring_buffer_ptr->mutex_buffer));
    sem_clean_before_exit(ring_buffer_ptr);
    shm_clean_before_exit(name, fd);
    exit(EXIT_SUCCESS);
}
///////////////////////////////////////////////////////////////////////////////////////////////////
//helper functions:


bool sem_init_error(int sem_ret) {
    if (sem_ret < 0) {
        perror("sem_init");
        //If sem_init() fails, the semaphore is not initialized and should not be destroyed.
        return true;
    } else {
        return false;
    }
}


void ring_buffer_init(RingBuffer *ringBuffer) {
    ringBuffer->head = 0;
    ringBuffer->tail = 0;
}

bool ring_buffer_is_full(RingBuffer *buf, uint64_t buffersize) {
    return ((buf->head + 1) % buffersize) ==
           buf->tail;  //+1 because for RingBuffer one variant is to leave one space empty to differentiate full from empty.
}

bool ring_buffer_is_empty(RingBuffer *buf) {
    return buf->head == buf->tail;
}

bool ring_buffer_push(RingBuffer *buf, uint64_t buffersize, uint64_t *data_transfer_in, uint64_t i, uint64_t L) {
    //if(ring_buffer_is_full(buf, buffersize)) {return false;}
    //else{
    //buf->buffer[buf->head] = *data_transfer_in;     //invalid write of size 8 (size 8 is size of uint64_t)
    buf->buffer[i % L] = *data_transfer_in;           //Problem of invalid write size fixed like this...???
    //memcpy(&(buf->buffer[buf->head]), data_transfer_in, sizeof(uint64_t));
#if DEBUG
    fprintf(stderr, "push: %lu\n", buf->buffer[buf->head]);
#endif
    buf->head = ((buf->head + 1) % buffersize); //move buffer head forward, because we have written to the buffer.
    // %buffersize because we have a "ring" buffer
    return true;
    //}
}


bool ring_buffer_pop(RingBuffer *buf, uint64_t buffersize, uint64_t *data_transfer, uint64_t i, uint64_t L) {
    //if(ring_buffer_is_empty(buf)) {return false;}
    //else{
    //*data_transfer = (buf->buffer[buf->tail]);  //invalid read of size 8
    *data_transfer = (buf->buffer[i % L]);          //Problem of invalid write size fixed like this...???
#if DEBUG
    fprintf(stderr, "pop: %lu\n", buf->buffer[buf->tail]);
#endif
    buf->tail = ((buf->tail + 1) %
                 buffersize); //move buffer tail forward, because we have taken one item from the buffer.
    // %buffersize because we have a "ring" buffer
    return true;
    //}
}

//supplied by PS_team
void validate_result(uint64_t result, const uint64_t K, const uint64_t N) {
    for (uint64_t i = 0; i < K; i++) {
        result -= N * (i + 1);
    }
    printf("Checksum: %lu \n", result);
}


bool fork_error_check(pid_t pid) {
    if (pid < 0) {
        //set errno = 0 before fork() call!
        perror("Fork failed");
        return true;
    } else {
        return false;
    }
}


void shm_clean_before_exit(const char *name, int fd) {
    close(fd);
    shm_unlink(name);
}

void sem_clean_before_exit(RingBuffer *ring_buffer_ptr) {
    sem_destroy(&(ring_buffer_ptr->free_space_available));
    sem_destroy(&(ring_buffer_ptr->data_available));
}

void check_argc(int argc) {
    if (argc < 4 || argc > 4) {
        printf("usage: ."__FILE__" < 3  ints>");
        exit(EXIT_FAILURE);
    }
}

unsigned long long int cast_to_ulli_with_check(char *string) {
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


