#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <stddef.h> // for size_t
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/queue.h> // see queue(7) & stailq(3)

typedef void* (*job_function)(void *);
typedef void *job_arg;

//typedef /* TODO */ job_id;
typedef uint64_t job_id;

///////////////////////////////////////////////////////////////////////////////////////////////////

/***
 * This is the stub of a simple job queue.
 */
struct job_queue_entry {
    // TODO: Design the contents for a queue, which stores jobs.
    int value;
    //todo: check if additional parameter for job_id is necessary??
    STAILQ_ENTRY(job_queue_entry) entries;
};

STAILQ_HEAD(myqueue_head, job_queue_entry);

typedef struct myqueue_head myqueue;

static void myqueue_init(myqueue* q) {
    STAILQ_INIT(q);
}

static bool myqueue_is_empty(myqueue* q) {
    return STAILQ_EMPTY(q);
}

static void myqueue_push(myqueue* q, int value) {
    struct job_queue_entry* entry = malloc(sizeof(struct job_queue_entry));
    entry->value = value;
    STAILQ_INSERT_TAIL(q, entry, entries);
}

static int myqueue_pop(myqueue* q) {
    assert(!myqueue_is_empty(q));
    struct job_queue_entry* entry = STAILQ_FIRST(q);
    const int value = entry->value;
    STAILQ_REMOVE_HEAD(q, entries);
    free(entry);
    return value;
}


///////////////////////////////////////////////////////////////////////////////////////////////////


/***
 * This is the stub for the thread pool that uses the queue.
 * Implement at LEAST the Prototype functions below.
 */
typedef struct {
    // TODO: Design the contents of a thread pool
} thread_pool;


// Prototypes for REQUIRED functions
void pool_create(thread_pool *pool, size_t size);

job_id pool_submit(thread_pool *pool, job_function start_routine, job_arg arg);

// You need to define a datatype for the job_id (chose wisely).
void pool_await(job_id id);

void pool_destroy(thread_pool *pool);

#endif
