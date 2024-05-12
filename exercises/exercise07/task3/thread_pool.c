#include "thread_pool.h"

/***
 * The void pool_create(thread_pool* pool, size_t size) function
 * initializes a thread_pool by starting size worker threads
 * and initializing a queue for jobs.
 * Each worker thread continuously checks the queue for submitted jobs.
 * Whenever a job is available, (exactly) one worker thread removes it from the queue and runs it.
 */
void pool_create(thread_pool* pool, size_t size){
#error "TODO"
}

/**
 *The job_id pool_submit(thread_pool* pool, job_function start_routine, job_arg arg)
 * submits a job to the thread pool
 * and returns a job_id.
 */
job_id pool_submit(thread_pool* pool, job_function start_routine, job_arg arg) {
#error "TODO"
}

/**
 * The void pool_await(job_id id) function waits for the job with the given job_id to finish.
 */
void pool_await(job_id id) {
#error "TODO"
    //https://www.perplexity.ai/search/typedef-void-jobfunctionvoid-j9g0hq9XS1KLJ9f4WVHvXw
}

/**
 * The void pool_destroy(thread_pool* pool) shuts down the thread pool and frees all associated resources.
 * Worker threads finish the currently running job (if any) and then stop gracefully.
 */
void pool_destroy(thread_pool* pool) {
#error "TODO"
}
