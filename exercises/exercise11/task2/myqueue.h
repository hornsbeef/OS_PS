#ifndef MYQUEUE_H_
#define MYQUEUE_H_

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/queue.h> // see queue(7) & stailq(3)
//copied whole file form previous homework exercise06 but modified it a bit

typedef struct myqueue_head myqueue;

typedef struct flower {
    atomic_bool has_been_harvested;
    unsigned long long ff_height;
    unsigned long long ff_width;
    unsigned long long  heigt;
    unsigned long long  width;
}flower_t;

typedef struct beehive {
    myqueue* queue;
    atomic_int max_number_nectar;
    atomic_int num_nectar;
    atomic_int actual_number_of_nectar_collected; //used in case of bear attack
    atomic_bool destroyed_by_bear;
    pthread_mutex_t* mutex_PTR;
    pthread_barrier_t* barrier_PTR;

}beehive_t;

typedef struct bee{
    flower_t* flowerfield_PTR;  // Access elements using *(flowrefield_PTR + i * ff_width + j)
    beehive_t* beehive_PTR;
    unsigned long long bee_name;
}bee_t;


struct myqueue_entry {
    //int value;  //depends on this, what can be pushed into the queue
    flower_t* flower_PTR;
    STAILQ_ENTRY(myqueue_entry) entries;
};

STAILQ_HEAD(myqueue_head, myqueue_entry);

static void myqueue_init(myqueue* q) {
    STAILQ_INIT(q);
}

static bool myqueue_is_empty(myqueue* q) {
    return STAILQ_EMPTY(q);
}

static void myqueue_push(myqueue* q, flower_t* flower_PTR) {
    struct myqueue_entry* entry = malloc(sizeof(struct myqueue_entry));
    //entry->value = value;
    entry->flower_PTR = flower_PTR;
    STAILQ_INSERT_TAIL(q, entry, entries);
}

static flower_t* myqueue_pop(myqueue* q) {
    assert(!myqueue_is_empty(q));
    struct myqueue_entry* entry = STAILQ_FIRST(q);
    //const int value = entry->value;
    flower_t* flower_PTR = entry->flower_PTR;
    STAILQ_REMOVE_HEAD(q, entries);
    free(entry);
    return flower_PTR;
}

#endif
