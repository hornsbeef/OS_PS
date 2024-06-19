#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE
// #define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <setjmp.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdatomic.h>
#include "map_printing.h"
#include "myqueue.h"


#define GRID_SIZE 10
#define MAX_ADVENTURERS 100

//#define EMPTY 81
//#define MONEY 17
//#define MONSTER 1
//#define ITEM 1

//copied from previous homework (e9 or e10 or e8) -> often reused.
#define MIN_ARGS 1
#define MAX_ARGS 1
void check_argc(int argc) {
    if (argc < MIN_ARGS || argc > MAX_ARGS) {
        printf("usage: ."__FILE__" <number of adventurers>\n");
        exit(EXIT_FAILURE);
    }
}
#undef MIN_ARGS
#undef MAX_ARGS

//copied from previous homework (e9 or e10 or e8) -> often reused.
unsigned long long int cast_to_ulli_with_check(char *string) {
    errno = 0;
    char *end = NULL;
    unsigned long long operand = strtoull(string, &end, 10);
    //check conversion:
    if ((*end != '\0') || (string == end)) {       //conversion interrupted || no conversion happened
        fprintf(stderr, "Conversion of argument ended with error.\n");
        if (errno != 0) {
            perror("StrToULL");
        }
        exit(EXIT_FAILURE);
    }
    if (errno != 0) {        //== ERANGE //as alternative to != 0
        perror("Conversion of argument ended with error");
        fprintf(stderr, "Please recheck usage!\n");
        exit(EXIT_FAILURE);
    }
    return operand;
}

//copied form prvious homework (e9 or e10 or e8) -> often reused.
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

void *pthreadStartRoutine(void *arg) {
    //pthread_args* my_pthread_args_ptr = (pthread_args *) arg;   //casting the void* arg to pthread_args*
    adventurer_t *adventurer_PTR = (adventurer_t *) arg;
    unsigned int seed = time(NULL) ^ pthread_self(); //MUST BE DONE BEFORE LOOP! HERE rand_r safes state!


    //Day_night cycle:
    int max_moves = GRID_SIZE*GRID_SIZE;
    int moves = 0;




    loop:
    while(moves < max_moves && adventurer_PTR->monsters_encountered < 2) {

        //first check, does it every cell, but no matter.
        pthread_error_funct(pthread_mutex_lock(adventurer_PTR->gamemaster_PTR->mutex_PTR));
        //only first adventurer gets points.
        if((adventurer_PTR->map_PTR + adventurer_PTR->height * GRID_SIZE + adventurer_PTR->width)->has_been_harvested == false) //TODO: CHECK IF CORRECT
        {
            (adventurer_PTR->map_PTR + adventurer_PTR->height * GRID_SIZE + adventurer_PTR->width)->has_been_harvested = true;
            adventurer_PTR->gamemaster_PTR->number_of_fields_discovered++;

            int random_chance = rand_r(&seed) % (100 - 0 + 1) + 1;
            if (random_chance <= 1) {
                (adventurer_PTR->map_PTR + adventurer_PTR->height * GRID_SIZE + adventurer_PTR->width)->type = MONSTER;
                adventurer_PTR->monsters_encountered++;
                adventurer_PTR->points += 1; //increment points
            } else if (random_chance <= 1+1 && random_chance > 1) {
                (adventurer_PTR->map_PTR + adventurer_PTR->height * GRID_SIZE + adventurer_PTR->width)->type = ITEM;
                adventurer_PTR->points += 26; //increment points

            } else if (random_chance > 2 && random_chance <= 17 + 2) {
                (adventurer_PTR->map_PTR + adventurer_PTR->height * GRID_SIZE + adventurer_PTR->width)->type = MONEY;
                adventurer_PTR->points += 17; //increment points

            } else {
                (adventurer_PTR->map_PTR + adventurer_PTR->height * GRID_SIZE + adventurer_PTR->width)->type =  EMPTY;
                //no point increase
            }
        }
        pthread_error_funct(pthread_mutex_unlock(adventurer_PTR->gamemaster_PTR->mutex_PTR));


        int random_chance_move = rand_r(&seed) % (100 - 0 + 1) + 1;
        if (random_chance_move <= 25) {
            //move up
            if (adventurer_PTR->height > 0) {
                adventurer_PTR->height--;
            }else if(adventurer_PTR->height == 0){
                adventurer_PTR->height = GRID_SIZE-1;
            }

        } else if (random_chance_move <= 50 && random_chance_move > 25) {
            //move down
            if (adventurer_PTR->height < GRID_SIZE-1) {
                adventurer_PTR->height++;
            } else if(adventurer_PTR->height == GRID_SIZE-1){
                adventurer_PTR->height = 0;
            }

        } else if (random_chance_move <= 75 && random_chance_move > 50) {
            //move left
            if (adventurer_PTR->width > 0) {
                adventurer_PTR->width--;
            }else if(adventurer_PTR->width == 0){
                adventurer_PTR->width = GRID_SIZE-1;
            }
        } else {
            //move right
            if (adventurer_PTR->width < GRID_SIZE-1) {
                adventurer_PTR->width++;
            }else if(adventurer_PTR->width == GRID_SIZE-1){
                adventurer_PTR->width = 0;
            }
        }
        moves++; //increment moves

        //// Access elements using *(map_PTR + i * GRID_SIZE + j)
        //check if cell has been harvested && get points:
        pthread_error_funct(pthread_mutex_lock(adventurer_PTR->gamemaster_PTR->mutex_PTR));
        //only first adventurer gets points.
        if((adventurer_PTR->map_PTR + adventurer_PTR->height * GRID_SIZE + adventurer_PTR->width)->has_been_harvested == false) //TODO: CHECK IF CORRECT
        {
            (adventurer_PTR->map_PTR + adventurer_PTR->height * GRID_SIZE + adventurer_PTR->width)->has_been_harvested = true;
            adventurer_PTR->gamemaster_PTR->number_of_fields_discovered++;
            int random_chance = rand_r(&seed) % (100 - 0 + 1) + 1;
            if (random_chance <= 1) {
                (adventurer_PTR->map_PTR + adventurer_PTR->height * GRID_SIZE + adventurer_PTR->width)->type = MONSTER;
                adventurer_PTR->monsters_encountered++;
                adventurer_PTR->points += 1; //increment points
            } else if (random_chance <= 1+1 && random_chance > 1) {
                (adventurer_PTR->map_PTR + adventurer_PTR->height * GRID_SIZE + adventurer_PTR->width)->type = ITEM;
                adventurer_PTR->points += 26; //increment points

            } else if (random_chance > 2 && random_chance <= 17 + 2) {
                (adventurer_PTR->map_PTR + adventurer_PTR->height * GRID_SIZE + adventurer_PTR->width)->type = MONEY;
                adventurer_PTR->points += 17; //increment points

            } else {
                (adventurer_PTR->map_PTR + adventurer_PTR->height * GRID_SIZE + adventurer_PTR->width)->type =  EMPTY;
                //no point increase
            }
        }
        pthread_error_funct(pthread_mutex_unlock(adventurer_PTR->gamemaster_PTR->mutex_PTR));


    }
    //has reachd max moves or has encountered 2 monsters
    //simmilar to homework(e10) about barriers
    int ret = pthread_barrier_wait(adventurer_PTR->gamemaster_PTR->barrier_PTR);
    if (ret == 0) { goto nextwait; }    //All but 1 thread: go to next pthread_barrier_wait so all threads are at the same location again.
    else if (ret == PTHREAD_BARRIER_SERIAL_THREAD) // the ONE thread:
    {
        adventurer_PTR->gamemaster_PTR->days++;
        pthread_error_funct(pthread_mutex_lock(adventurer_PTR->gamemaster_PTR->mutex_PTR));
        adventurer_PTR->gamemaster_PTR->ready_to_print = true;
        pthread_error_funct(pthread_cond_signal(adventurer_PTR->gamemaster_PTR->ready_to_print_PTR));

        //just waiting for the master to print
        while(adventurer_PTR->gamemaster_PTR->has_printed == false){
            pthread_error_funct(pthread_cond_wait(adventurer_PTR->gamemaster_PTR->master_has_printed_PTR, adventurer_PTR->gamemaster_PTR->mutex_PTR));
        }
        //must have printed:
        adventurer_PTR->gamemaster_PTR->ready_to_print = false; //reset
        adventurer_PTR->gamemaster_PTR->has_printed = false; //reset
        pthread_error_funct(pthread_mutex_unlock(adventurer_PTR->gamemaster_PTR->mutex_PTR));


    }else /*this means error in pthread_barrier_wait() */{
        pthread_error_funct(ret);
    }


    nextwait:
    int ret1 = pthread_barrier_wait(adventurer_PTR->gamemaster_PTR->barrier_PTR);
    if (ret1 != 0 && ret1 != PTHREAD_BARRIER_SERIAL_THREAD) { pthread_error_funct(ret1); }
    if(adventurer_PTR->gamemaster_PTR->game_over == true){
        pthread_exit(NULL);
    }
    goto loop;


}


int main(int argc, char *argv[]) {
    check_argc(argc);

    unsigned long long int num_of_adventurers = cast_to_ulli_with_check(argv[1]);
    if (num_of_adventurers == 0) {
        fprintf(stderr, "Number of adventurers must be greater than 0.\n");
        exit(EXIT_FAILURE);
    }

    gamemaster_t gamemaster;
    gamemaster.number_of_fields_discovered = 0;

    pthread_barrier_t barrier;
    gamemaster.barrier_PTR = &barrier;

    //copied from homework(e10)
    // * INIT BARRIER
    pthread_error_funct(pthread_barrier_init(gamemaster.barrier_PTR, NULL, num_of_adventurers));


    //simmilar to homework(06 or 08 or 09)
    pthread_mutex_t mutex;
    gamemaster.mutex_PTR = &mutex;
    //mutex and mutex_attr init:
    pthread_mutexattr_t mutex_attr;
    pthread_error_funct(pthread_mutexattr_init(&mutex_attr));
    pthread_error_funct(pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED));
    //Synchronization variables that are initialized with the PTHREAD_PROCESS_SHARED process-shared attribute may be operated on by any thread in any process that has access to it.
    pthread_error_funct(pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK));
    //PTHREAD_MUTEX_ERRORCHECK: This type of mutex provides error checking.
    // A thread attempting to relock this mutex without first unlocking it shall return with an error.
    // A thread attempting to unlock a mutex which another thread has locked shall return with an error.
    // A thread attempting to unlock an unlocked mutex shall return with an error.
    pthread_error_funct(pthread_mutex_init(gamemaster.mutex_PTR, &mutex_attr));

    ////////////////CONDITION VARIABLE INIT////////////////////////
    //master_has_printed init:
    pthread_cond_t ready_to_print;
    pthread_cond_t master_has_printed;
    pthread_error_funct(pthread_cond_init(&master_has_printed, NULL));
    pthread_error_funct(pthread_cond_init(&ready_to_print, NULL));
    gamemaster.master_has_printed_PTR = &master_has_printed;
    gamemaster.ready_to_print_PTR = &ready_to_print;

    gamemaster.has_printed = false; //init to false

    gamemaster.days = 0; //init to 0
    gamemaster.game_over = false;




    //unsigned int seed = time(NULL) ^ pthread_self();

    cell_t map[GRID_SIZE][GRID_SIZE];
    for (unsigned long long i = 0; i < GRID_SIZE; i++) {
        for (unsigned long long j = 0; j < GRID_SIZE; j++) {
            map[i][j].has_been_harvested = false;
            map[i][j].height = i;
            map[i][j].width = j;

        }
    }

    //Region start threads:

    adventurer_t all_adventurers_array[num_of_adventurers];

    //create ptheads = bees
    //simmilar to all previous homeworks containing pthreads.
    pthread_t pid_array[num_of_adventurers];
    for (unsigned long long i = 0; i < num_of_adventurers; i++) {
        //set adventurer things:
        all_adventurers_array[i].adventurer_name = i;
        all_adventurers_array[i].points = 0;
        all_adventurers_array[i].gamemaster_PTR = &gamemaster;
        all_adventurers_array[i].map_PTR = &map[0][0];
        all_adventurers_array[i].height = GRID_SIZE/2;
        all_adventurers_array[i].width = 0;
        all_adventurers_array[i].monsters_encountered = 0;

        //// Access elements using *(map_PTR + i * GRID_SIZE + j)
        pthread_error_funct(pthread_create(&pid_array[i], NULL, &pthreadStartRoutine, &all_adventurers_array[i]));

    }


    int max_cells = GRID_SIZE*GRID_SIZE;
    bool continue_game = true;

    while(continue_game) //get break checking
    {

        pthread_error_funct(pthread_mutex_lock(gamemaster.mutex_PTR));
        while(gamemaster.ready_to_print == false){
            pthread_error_funct(pthread_cond_wait(gamemaster.ready_to_print_PTR, gamemaster.mutex_PTR));
        }
        //do the printing
        print_daily_map(gamemaster.days, (cell_type_t**)map, GRID_SIZE);

        //check if more than 90% of map is discovered
        if(gamemaster.number_of_fields_discovered >= max_cells*0.9){
            continue_game = false;
            gamemaster.game_over = true;
        }

        gamemaster.has_printed = true;
        pthread_error_funct(pthread_cond_signal(gamemaster.master_has_printed_PTR));

        pthread_error_funct(pthread_mutex_unlock(gamemaster.mutex_PTR));

    }

    //simmilar to all previous homeworks containing pthreads.
    for (unsigned long long i = 0; i < num_of_adventurers; i++) {
        pthread_error_funct(pthread_join(pid_array[i], NULL));
    }

    for (unsigned long long i = 0; i < num_of_adventurers; i++) {
        //get points
        //TODO: print points
    }



    pthread_mutex_destroy(gamemaster.mutex_PTR);
    pthread_cond_destroy(gamemaster.ready_to_print_PTR);
    pthread_cond_destroy(gamemaster.master_has_printed_PTR);


    return(EXIT_SUCCESS);







}