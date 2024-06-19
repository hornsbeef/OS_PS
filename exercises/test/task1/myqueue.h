#ifndef MYQUEUE_H_
#define MYQUEUE_H_

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/queue.h> // see queue(7) & stailq(3)
//copied whole file form previous homework exercise06 but modified it a bit

typedef struct myqueue_head myqueue;

typedef struct cell {
    atomic_bool has_been_harvested;
    unsigned long long height;
    unsigned long long width;
    cell_type_t type;
} cell_t;

typedef struct gamemaster {
    atomic_int number_of_fields_discovered;
    pthread_mutex_t *mutex_PTR;
    pthread_barrier_t *barrier_PTR;
    pthread_cond_t *master_has_printed_PTR;
    pthread_cond_t *ready_to_print_PTR;
    atomic_bool has_printed;
    atomic_bool ready_to_print;
    atomic_int days;
    atomic_bool game_over;
} gamemaster_t;

typedef struct adventurer{
    unsigned long long adventurer_name;
    atomic_int points; //points collected by adventurer
    gamemaster_t *gamemaster_PTR;
    cell_t *map_PTR; //map of the game
    int height;
    int width;
    int monsters_encountered;
}adventurer_t;









/*
struct myqueue_entry {
    //int value;  //depends on this, what can be pushed into the queue
    flower_t *flower_PTR;
    STAILQ_ENTRY(myqueue_entry) entries;
};

STAILQ_HEAD(myqueue_head, myqueue_entry);

static void myqueue_init(myqueue *q) {
    STAILQ_INIT(q);
}

static bool myqueue_is_empty(myqueue *q) {
    return STAILQ_EMPTY(q);
}

static void myqueue_push(myqueue *q, flower_t *flower_PTR) {
    struct myqueue_entry *entry = malloc(sizeof(struct myqueue_entry));
    //entry->value = value;
    entry->flower_PTR = flower_PTR;
    STAILQ_INSERT_TAIL(q, entry, entries);
}

static flower_t *myqueue_pop(myqueue *q) {
    assert(!myqueue_is_empty(q));
    struct myqueue_entry *entry = STAILQ_FIRST(q);
    //const int value = entry->value;
    flower_t *flower_PTR = entry->flower_PTR;
    STAILQ_REMOVE_HEAD(q, entries);
    free(entry);
    return flower_PTR;
}
*/
#endif
