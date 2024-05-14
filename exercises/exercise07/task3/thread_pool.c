//https://www.perplexity.ai/search/typedef-void-jobfunctionvoid-j9g0hq9XS1KLJ9f4WVHvXw
//https://nachtimwald.com/2019/04/12/thread-pool-in-c/#waiting-for-processing-to-complete
#define DEBUG 1


#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE
// #define _BSD_SOURCE

#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include "thread_pool.h"
#include <stddef.h>

//typedef struct pthread_args {
//    myqueue *myQueue;
//    pthread_t tid;
//} pthread_args;

STAILQ_HEAD(myqueue_head, job_queue_entry);




pthread_mutex_t mutex_queue;
pthread_cond_t cond_data_pushed_to_queue;
myqueue myQueue;


void pthread_error_funct(int pthread_returnValue);
void* pthread_worker_funct(void* arg);
///////////////////////////////////////////////////////////////////////////////////////////////////
static void myqueue_init(myqueue* q) {
    STAILQ_INIT(q);
}

static bool myqueue_is_empty(myqueue* q) {
    return STAILQ_EMPTY(q);
}

static void myqueue_push(myqueue* q, job_function jobFunction, job_arg jobArg, job_id jobID) {  //error here maybe?
    //mutex already locked at calling function!
    struct job_queue_entry* entry = malloc(sizeof(struct job_queue_entry));
    if(entry == NULL){
        fprintf(stderr, "Malloc at myqueue_push failed -> (un)gracefully crashing program");
        exit(EXIT_FAILURE);
    }

    entry->jobFunction = jobFunction;
    entry->jobArg = jobArg;
    entry->jobID = jobID;
    STAILQ_INSERT_TAIL(q, entry, entries);
}



static job_queue_entry* myqueue_pop(myqueue* q) {
    //mutex is still locked from function calling myqueue_pop

    assert(!myqueue_is_empty(q));
    struct job_queue_entry* entry = STAILQ_FIRST(q);


    STAILQ_REMOVE_HEAD(q, entries);

    return entry;

    //free(entry);  //old   //-> is freed in function calling myqueue_pop()
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/***
 * The void pool_create(thread_pool* pool, size_t size) function
 * initializes a thread_pool by starting size worker threads
 * and initializing a myQueue for jobs.
 * Each worker thread continuously checks the myQueue for submitted jobs.
 * Whenever a job is available, (exactly) one worker thread removes it from the myQueue and runs it.
 */
void pool_create(thread_pool* pool, size_t size){

    if(size == 0){
        size = 2; //set size to 2 as a default
    }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//mutex and mutex_attr init: -> try if works with this too
//    pthread_mutexattr_t mutex_queue_attr;
//    pthread_error_funct(pthread_mutexattr_init(&mutex_queue_attr));
//    pthread_error_funct(pthread_mutexattr_setpshared(&mutex_queue_attr, PTHREAD_PROCESS_SHARED));
//    pthread_error_funct(pthread_mutexattr_settype(&mutex_queue_attr, PTHREAD_MUTEX_ERRORCHECK));
//
//    pthread_error_funct(pthread_mutex_init(&mutex_queue, &mutex_queue_attr));
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    pthread_error_funct(pthread_mutex_init(&(pool->mutex_queue), NULL));
    pthread_error_funct(pthread_cond_init(&(pool->cond_data_pushed_to_queue), NULL));


    pthread_error_funct(pthread_mutex_lock(&(pool->mutex_queue)));
    //I am confused: I apparently should put the mutex in the pool_struct, but whenever I read/modify the pool_struct I
    //should only do so when the pool is locked...


    myqueue_init(&myQueue); //initializing a myQueue for jobs.
    pool->queue = &myQueue;
    pool->num_threads = size;
    pool->stop = false;     //different to example


//different from example:
    //pool->tid = malloc(pool->num_threads * sizeof(*(pool->tid)));

    pthread_t* tID_arr = malloc(pool->num_threads * sizeof(pthread_t));
    if(tID_arr == NULL){
        fprintf(stderr, "malloc for tid-array failed");
        exit(EXIT_FAILURE);
    }
//enddif




    for(size_t i = 0; i<size; i++){
        pthread_error_funct(pthread_create(&(tID_arr[i]), NULL, pthread_worker_funct, pool));

        //pthread_error_funct(pthread_detach(tID_arr[i])); //resources automagically release when thread terminates //TODO: maybe this causes problem with joining?
    }
    pool->tid = tID_arr;

    pthread_mutex_unlock(&(pool->mutex_queue));

}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *The job_id pool_submit(thread_pool* pool, job_function start_routine, job_arg arg)
 * submits a job to the thread pool (-> to the queue in the thread_pool)
 * and returns a job_id.
 */
job_id pool_submit(thread_pool* pool, job_function start_routine, job_arg arg) {

    if(pool == NULL){
        return NULL;    //different from example
    }

//different from example: job_stat struct
    job_status* job_stat = malloc(sizeof(*job_stat));  //creating "unique-id" aka a pointer to a struct
    pthread_cond_init(&job_stat->job_cond, NULL);
    job_stat->completed = false;        //setting to false, because job has not been started.
    job_stat->pool = pool;
//enddiff


    pthread_error_funct(pthread_mutex_lock(&(pool->mutex_queue)));

    //uint64_t id = generate_unique_id();
    myqueue_push(pool->queue, start_routine, arg, job_stat);

    pthread_cond_signal(&(pool->cond_data_pushed_to_queue));    //why is example using broadcast here?
    pthread_mutex_unlock(&(pool->mutex_queue));


    return job_stat;    //different from example

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * The void pool_await(job_id id) function waits for the job with the given job_id to finish.
 */
 //to finish what? executing its current job? or finish executing after it executed all jobs in queue?
void pool_await(job_id id) {    //TODO: this seems to be the culprit!

    if(id == NULL){
        return;
    }

    //fprintf(stderr, "awaiting job with id: %p\n", id);

    pthread_mutex_lock(&(id->pool->mutex_queue));   //TODO: might this be a problem, because it is "different" pointer to mutex?


    job_status* job_stat = id;

    while (!job_stat->completed) {
        //pthread_cond_wait(&job_stat->job_cond, &(id->pool->mutex_queue));   //TODO: here LOCKING MAYBE?
        pthread_cond_wait(&job_stat->job_cond, &(id->pool->mutex_queue));   //TODO: here LOCKING MAYBE?
    }   //stops here, because a job has not finished.



    pthread_cond_destroy(&job_stat->job_cond);
    pthread_mutex_unlock(&(id->pool->mutex_queue));

    //do anything else? -> maybe stop them from taking new jobs?


     free(id);   //freeing the job_struct, that was alloc'd in pool_submit


     return;     //not needed, but for my sanity.

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
 * The void pool_destroy(thread_pool* pool)
 * shuts down the thread pool and frees all associated resources.
 * Worker threads finish the currently running job (if any) and then stop gracefully.
 */
void pool_destroy(thread_pool* pool) {

    if(pool == NULL){
        return;
    }

    pthread_error_funct(pthread_mutex_lock(&(pool->mutex_queue)));

    while(!myqueue_is_empty((pool->queue))){    //throwing away not started jobs!
        job_queue_entry* work =myqueue_pop(pool->queue);
        work->jobID->completed = true;
    }

    assert(myqueue_is_empty(pool->queue));

    pool->stop = true; //-> telling workers to quit
    pthread_cond_broadcast(&(pool->cond_data_pushed_to_queue)); //making sure all workers get the memo.

    pthread_mutex_unlock(&(pool->mutex_queue));


    //different form example:
    //waiting for threads that are currently running to finish and join.
    for (size_t i = 0; i < pool->num_threads; i++) {
        //fprintf(stderr, "waiting to join thread : i = %zu\n", i);
        pthread_join((pool->tid[i]), NULL);
    }

    free(pool->tid);
    pthread_mutex_destroy(&pool->mutex_queue);
    pthread_cond_destroy(&pool->cond_data_pushed_to_queue);   //TODO: Thread #1: pthread_cond_destroy: destruction of condition variable being waited upon

    //pool is NOT malloc'd, it was provided by main!
    //-> DO NOT FREE!

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


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


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
 * Each worker thread continuously checks the myQueue for submitted jobs.
 * Whenever a job is available, (exactly) one worker thread removes it from the myQueue and runs it.
 */
void* pthread_worker_funct(void* arg){

    pthread_error_funct(pthread_mutex_lock(&mutex_queue));
    thread_pool* pool = (thread_pool*) arg;
    myqueue* queue = pool->queue;
    pthread_mutex_unlock(&mutex_queue);

    //infinity_loop:
    while (true) {
        pthread_error_funct(pthread_mutex_lock(&(pool->mutex_queue)));  //TODO: THIS SEEMS TO CAUSE PROBLEM
        //pthread_error_funct(pthread_mutex_lock(&(mutex_queue)));  //seemed to work

        //cond_data_pushed_to_queue wait: && checking if pool is shutting down
        while (myqueue_is_empty(pool->queue) && (pool->stop == false)) {             //catch for spurious wakeup!
            //pthread_error_funct(pthread_cond_wait(&cond_data_pushed_to_queue, &mutex_queue));
            pthread_error_funct(pthread_cond_wait(&(pool->cond_data_pushed_to_queue), &(pool->mutex_queue)));
        }

        if(pool->stop == true){
            break;
        }

        job_queue_entry* work = myqueue_pop(queue);
        //maybe add: working_cnt to pool_struct and ++ here;
        pthread_mutex_unlock(&(pool->mutex_queue));
        //pthread_mutex_unlock(&mutex_queue);

        if(work != NULL){
            work->jobFunction(work->jobArg);

            //signal that job is finished.  //different from example!
            //pthread_mutex_lock(&(pool->mutex_queue));   //this gets stuck at i=0 when waiting
            pthread_error_funct(pthread_mutex_lock(&(mutex_queue)));   //this at i = ~15 todo: seems like a mutex-Problem here!

            work->jobID->completed = true;              //sets jobID to completed, after running job.
            pthread_cond_signal(&(work->jobID->job_cond));
            /*TODO:
             * Thread #2: pthread_cond_{signal,broadcast}: dubious: associated lock is not held by any thread
             * Thread #3: pthread_cond_{signal,broadcast}: dubious: associated lock is not held by any thread
             */

            //pthread_mutex_unlock(&(pool->mutex_queue));
            pthread_mutex_unlock(&(mutex_queue));

            free(work); //freeing temp that points to the job_queue_entry from myqueue_pop
        }

    }

    /* from example:
    tm->thread_cnt--;
    pthread_cond_signal(&(tm->working_cond));
    pthread_mutex_unlock(&(tm->work_mutex));
    return NULL;
     */

    pthread_mutex_unlock(&(pool->mutex_queue));
    return NULL;

}

