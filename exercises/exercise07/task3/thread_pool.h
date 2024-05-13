#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <stddef.h> // for size_t
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/queue.h> // see myQueue(7) & stailq(3)


typedef void* (*job_function)(void *);
typedef void *job_arg;

typedef struct {
    pthread_cond_t job_cond;
    bool completed;
} job_status;

typedef job_status* job_id;    //?good choice?
// https://www.perplexity.ai/search/ifndef-THREADPOOLH-define-zX4YeosJQUKWvMjOcMaSrg


///////////////////////////////////////////////////////////////////////////////////////////////////

/***
 * This is the stub of a simple job myQueue.
 */
typedef struct job_queue_entry {
    // TODO: Design the contents(->fields) for a myQueue, which stores jobs.:
    //todo: check if additional parameter (for job_id, etc.) is necessary??
    //how to include job_struct ?? or is even necessary?

    job_function jobFunction;
    job_arg jobArg;
    job_id jobID;

    STAILQ_ENTRY(job_queue_entry) entries;
}job_queue_entry;

STAILQ_HEAD(myqueue_head, job_queue_entry);

typedef struct myqueue_head myqueue;

static void myqueue_init(myqueue* q) {
    STAILQ_INIT(q);
}

static bool myqueue_is_empty(myqueue* q) {
    return STAILQ_EMPTY(q);
}

//todo: why compiler error that unused?? is used in thread_pool.c
static void myqueue_push(myqueue* q, job_function jobFunction, job_arg jobArg, job_id jobID) {
    struct job_queue_entry* entry = malloc(sizeof(struct job_queue_entry));
    entry->jobFunction = jobFunction;
    entry->jobArg = jobArg;
    entry->jobID = jobID;
    STAILQ_INSERT_TAIL(q, entry, entries);
}

//todo: why compiler error that unused?? is used in thread_pool.c
static void myqueue_pop(myqueue* q, job_queue_entry* temp) {
    assert(!myqueue_is_empty(q));
    struct job_queue_entry* entry = STAILQ_FIRST(q);

    temp->jobFunction = entry->jobFunction;     //todo: check if works
    temp->jobArg = entry->jobArg;                //todo: check if works

    STAILQ_REMOVE_HEAD(q, entries);
    free(entry);
}


///////////////////////////////////////////////////////////////////////////////////////////////////


/***
 * This is the stub for the thread pool that uses the myQueue.
 * Implement at LEAST the Prototype functions below.
 */
typedef struct {
    myqueue* queue;
    size_t num_threads;
    pthread_mutex_t mutex_queue;
    pthread_cond_t cond_data_pushed_to_queue;
    bool stop;
    size_t id_tid;
    pthread_t* tid;          //check if works
} thread_pool;


// Prototypes for REQUIRED functions
void pool_create(thread_pool *pool, size_t size);

job_id pool_submit(thread_pool *pool, job_function start_routine, job_arg arg);

// You need to define a datatype for the job_id (chose wisely).
void pool_await(job_id id);

void pool_destroy(thread_pool *pool);

#endif
