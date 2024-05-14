#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <stddef.h> // for size_t
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/queue.h> // see myQueue(7) & stailq(3)
#include <stdatomic.h>


typedef void* (*job_function)(void *);      //typedef void (*thread_func_t)(void *arg);
typedef void *job_arg;

typedef struct myqueue_head myqueue;


/***
 * This is the stub for the thread pool that uses the myQueue.
 * Implement at LEAST the Prototype functions below.
 */
typedef struct {
    myqueue* queue;
    size_t num_threads;
    pthread_mutex_t mutex_queue;
    pthread_cond_t cond_data_pushed_to_queue;
    atomic_bool stop;
    pthread_t* tid;          //check if works   //for saving the pthead_t of each thread. later for joining.
} thread_pool;


typedef struct {
    pthread_cond_t job_cond;
    bool completed;
    thread_pool* pool;
} job_status;
typedef job_status* job_id;    // https://www.perplexity.ai/search/ifndef-THREADPOOLH-define-zX4YeosJQUKWvMjOcMaSrg


/***
 * This is the stub of a simple job myQueue.
 * Design the contents(->fields) for a myQueue, which stores jobs.
 */
typedef struct job_queue_entry {
    job_function jobFunction;
    job_arg jobArg;
    job_id jobID;
    STAILQ_ENTRY(job_queue_entry) entries;  //STAILQ_ENTRY(myqueue_entry) entries;  //old
}job_queue_entry;



// Prototypes for REQUIRED functions
void pool_create(thread_pool *pool, size_t size);
job_id pool_submit(thread_pool *pool, job_function start_routine, job_arg arg);
void pool_await(job_id id);// You need to define a datatype for the job_id (chose wisely).
void pool_destroy(thread_pool *pool);

#endif
