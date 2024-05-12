#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include "thread_pool.h"
#include <stddef.h>

//typedef struct pthread_args {
//    myqueue *myQueue;
//    pthread_t tid;
//} pthread_args;

pthread_mutex_t mutex_queue;
pthread_cond_t cond_data_pushed_to_queue;
myqueue myQueue;


void pthread_error_funct(int pthread_returnValue);
uint64_t generate_unique_id();

_Noreturn void* pthread_worker_funct(void* arg);
///////////////////////////////////////////////////////////////////////////////////////////////////


/***
 * The void pool_create(thread_pool* pool, size_t size) function
 * initializes a thread_pool by starting size worker threads
 * and initializing a myQueue for jobs.
 * Each worker thread continuously checks the myQueue for submitted jobs.
 * Whenever a job is available, (exactly) one worker thread removes it from the myQueue and runs it.
 */
void pool_create(thread_pool* pool, size_t size){
//srand init
    srand(time(NULL));

//initializing a myQueue for jobs.
    myqueue_init(&myQueue);


//mutex and mutex_attr init:
    pthread_mutexattr_t mutex_queue_attr;
    pthread_error_funct(pthread_mutexattr_init(&mutex_queue_attr));
    pthread_error_funct(pthread_mutexattr_setpshared(&mutex_queue_attr, PTHREAD_PROCESS_SHARED));
    pthread_error_funct(pthread_mutexattr_settype(&mutex_queue_attr, PTHREAD_MUTEX_ERRORCHECK));

    pthread_error_funct(pthread_mutex_init(&mutex_queue, &mutex_queue_attr));

//cond_data_pushed_to_queue init:
    pthread_error_funct(pthread_cond_init(&cond_data_pushed_to_queue, NULL));

//init thread_pool:
    pool->queue = &myQueue;
    pool->num_threads = size;
    pool->mutex_queue = mutex_queue;
    pool->cond_data_pushed_to_queue = cond_data_pushed_to_queue;
    pool->stop = false;

    pool->tid = malloc(pool->num_threads * sizeof(*(pool->tid)));
    if(pool->tid == NULL){
        fprintf(stderr, "malloc for tid-array failed");
        exit(EXIT_FAILURE);
    }

//starting size amount of worker threads:
    for(int i = 0; i<size; i++){
        pool->id_tid = i;

        pthread_error_funct(
                pthread_create(&(pool->tid[i]), NULL, &pthread_worker_funct, &pool));
        //TODO: check pool->tid[i] if works!
    }

    //todo: check if everything done to create pool

}

/**
 *The job_id pool_submit(thread_pool* pool, job_function start_routine, job_arg arg)
 * submits a job to the thread pool (-> to the queue in the thread_pool)
 * and returns a job_id.
 */
job_id pool_submit(thread_pool* pool, job_function start_routine, job_arg arg) {

    //creating "unique-id" aka a pointer to a struct
    job_status* job_stat = malloc(sizeof(job_status));
    pthread_cond_init(&job_stat->job_cond, NULL);
    job_stat->completed = false;

    pthread_error_funct(pthread_mutex_lock(&mutex_queue));

    //uint64_t id = generate_unique_id();
    myqueue_push(pool->queue, start_routine, arg, job_stat);

    pthread_cond_signal(&cond_data_pushed_to_queue);
    pthread_mutex_unlock(&mutex_queue);


    return job_stat;

}

/**
 * The void pool_await(job_id id) function waits for the job with the given job_id to finish.
 */
void pool_await(job_id id) {
    //https://www.perplexity.ai/search/typedef-void-jobfunctionvoid-j9g0hq9XS1KLJ9f4WVHvXw
    pthread_mutex_lock(&mutex_queue);

    // Find the job_status entry for the given job_id
    // This can be done using a hash table or a linear search
    job_status* job_stat = id;

    while (!job_stat->completed) {
        pthread_cond_wait(&job_stat->job_cond, &mutex_queue);
    }
    //todo: free the job_status memory!!!
    free(id);

    pthread_mutex_unlock(&mutex_queue);




}

/**
 * The void pool_destroy(thread_pool* pool) shuts down the thread pool and frees all associated resources.
 * Worker threads finish the currently running job (if any) and then stop gracefully.
 */
void pool_destroy(thread_pool* pool) {
#error "TODO"
}


///////////////////////////////////////////////////////////////////////////////////////////////////

void pthread_error_funct(int pthread_returnValue) {
    if (pthread_returnValue != 0) {
        char *error_msg = strerror(pthread_returnValue);
        fprintf(stderr, "Error code: %d\n"
                        "Error message: %s\n"
                        "Note that the pthreads functions do not set errno.\n",
                pthread_returnValue, error_msg);
        exit(EXIT_FAILURE);
    }
}

/**
 * Each worker thread continuously checks the myQueue for submitted jobs.
 * Whenever a job is available, (exactly) one worker thread removes it from the myQueue and runs it.
 */
_Noreturn void* pthread_worker_funct(void* arg){

    pthread_error_funct(pthread_mutex_lock(&mutex_queue));
    thread_pool* pool = (thread_pool*) arg;
    myqueue* queue = pool->queue;
    pthread_mutex_unlock(&mutex_queue);

    infinity_loop:
    while (true) {

        pthread_error_funct(pthread_mutex_lock(&mutex_queue));

        //cond_data_pushed_to_queue wait:
        while (myqueue_is_empty(pool->queue)) {             //catch for spurious wakeup!
            pthread_error_funct(pthread_cond_wait(&cond_data_pushed_to_queue, &mutex_queue));
        }

        job_queue_entry temp;
        myqueue_pop(queue, &temp);
        pthread_mutex_unlock(&mutex_queue);

        temp.jobFunction(temp.jobArg);

        //signal that job is finished.
        pthread_mutex_lock(&mutex_queue);
        temp.jobID->completed = true;
        pthread_cond_signal(&temp.jobID->job_cond);
        pthread_mutex_unlock(&mutex_queue);


    }
}

uint64_t generate_unique_id() {
    uint64_t timestamp = (uint64_t)time(NULL); // Get current timestamp
    uint64_t factor = 1000000; // Adjust this factor as needed
    int random_num = rand(); // Generate a random number

    uint64_t unique_id = timestamp * factor + random_num +1;

    return unique_id;
}