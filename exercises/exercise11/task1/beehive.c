//copied many bits and pieces form previous homeworks, but usually < 5 lines.

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
#include "myqueue.h"

#define MINTIME 100
#define MAXTIME 500


//copied from previous homework (e10)
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

//copied from previous homework (e10) but modified
#define MIN_ARGS 4
#define MAX_ARGS 4
void check_argc(int argc) {
    if (argc < MIN_ARGS || argc > MAX_ARGS) {
        printf("usage: ."__FILE__" <flower field width> <flower field height> <number of bees>\n");
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

void *pthreadStartRoutine(void *arg) {
    //pthread_args* my_pthread_args_ptr = (pthread_args *) arg;   //casting the void* arg to pthread_args*
    bee_t* bee_PTR = (bee_t*) arg;
    unsigned int seed = time(NULL) ^ pthread_self(); //MUST BE DONE BEFORE LOOP! HERE rand_r safes state!

    while(bee_PTR->beehive_PTR->num_nectar < bee_PTR->beehive_PTR->max_number_nectar){
        //terminate when all flowers have been collected.

        bool work_flower = false;

        flower_t* food_source = NULL;
        pthread_error_funct(pthread_mutex_lock(bee_PTR->beehive_PTR->mutex_PTR));
        if(myqueue_is_empty(bee_PTR->beehive_PTR->queue) == false){
            food_source = myqueue_pop(bee_PTR->beehive_PTR->queue);
            work_flower = true;
        }
        pthread_error_funct(pthread_mutex_unlock(bee_PTR->beehive_PTR->mutex_PTR));

        //waiting - on the way /or working at home
        //fprintf(stderr, "seed: %d\n", seed);
        int random_number = rand_r(&seed) % (MAXTIME - MINTIME + 1) + MINTIME;
        //fprintf(stderr, "random num: %d\n", random_number);
        //relocating the acutal sleep into the different work szenarios

        if(work_flower){
            if(food_source != NULL) //if food_source is NULL -> something went wrong -> just ignore this cycle (hopefully works)
            {
                printf("Bee %llu is flying to food source at position (%llu,%llu).\n", bee_PTR->bee_name, food_source->heigt, food_source->width);
                errno = 0;
                if(usleep(random_number * 1000) < 0){
                    perror("Usleep");
                    exit(EXIT_FAILURE);
                };

            }
            //check if flowr is harvestable:
            if(food_source != NULL && food_source->has_been_harvested == false){
                //harvest from flower
                food_source->has_been_harvested = true;
                //increase total nectar count
                bee_PTR->beehive_PTR->num_nectar++;

            //check surroundings -> add to queue;
                //create strings for new food source:
                char left[256] = {"\0"};
                char right[256] = {"\0"};
                char up[256] = {"\0"};
                char down[256] = {"\0"};
                //left  -> height same
                if(food_source->width >0 ){
                    flower_t* nextfl = (bee_PTR->flowerfield_PTR + food_source->heigt * bee_PTR->flowerfield_PTR->ff_width + (food_source->width - 1));
                    //if((bee_PTR->flowerfield_PTR + food_source->heigt * bee_PTR->flowerfield_PTR->ff_width + (food_source->width - 1))->has_been_harvested == false){
                    if(nextfl->has_been_harvested == false){

                        //simmilar to previous homeworks that used queues -> as are all other myqueue_pushes /pops
                        pthread_error_funct(pthread_mutex_lock(bee_PTR->beehive_PTR->mutex_PTR));
                        myqueue_push(bee_PTR->beehive_PTR->queue, nextfl);
                       // fprintf(stderr, "queue empty: %s\n", myqueue_is_empty(bee_PTR->beehive_PTR->queue) ? "true" : "false");
                        pthread_error_funct(pthread_mutex_unlock(bee_PTR->beehive_PTR->mutex_PTR));
                        errno = 0;  //https://pubs.opengroup.org/onlinepubs/9699919799/functions/fprintf.html
                        if(sprintf(left, "(%llu,%llu)", nextfl->heigt, nextfl->width) <0){
                            perror("Sprintf");
                            exit(EXIT_FAILURE);
                        }
                    }
                    else{
                        memset(left, '\0', sizeof(left));
                        //The memset() function shall return s; no return value is reserved
                        //       to indicate an error.
                    }
                }
                //right-> height same
                if(food_source->width < food_source->ff_width -1 ){
                    flower_t* nextfl = (bee_PTR->flowerfield_PTR + food_source->heigt * bee_PTR->flowerfield_PTR->ff_width + (food_source->width + 1));
                    //if((bee_PTR->flowerfield_PTR + food_source->heigt * bee_PTR->flowerfield_PTR->ff_width + (food_source->width - 1))->has_been_harvested == false){
                    if(nextfl->has_been_harvested == false){
                        pthread_error_funct(pthread_mutex_lock(bee_PTR->beehive_PTR->mutex_PTR));
                        myqueue_push(bee_PTR->beehive_PTR->queue, nextfl);
                        //fprintf(stderr, "queue empty: %s\n", myqueue_is_empty(bee_PTR->beehive_PTR->queue) ? "true" : "false");
                        pthread_error_funct(pthread_mutex_unlock(bee_PTR->beehive_PTR->mutex_PTR));
                        errno = 0;  //https://pubs.opengroup.org/onlinepubs/9699919799/functions/fprintf.html
                        if(sprintf(right, "(%llu,%llu)", nextfl->heigt, nextfl->width) <0){
                            perror("Sprintf");
                            exit(EXIT_FAILURE);
                        }
                    }
                    else{
                        memset(right, '\0', sizeof(right));
                    }
                }
                //up -> width same
                if(food_source->heigt >0 ){
                    flower_t* nextfl = (bee_PTR->flowerfield_PTR + (food_source->heigt -1)* bee_PTR->flowerfield_PTR->ff_width + (food_source->width));
                    //if((bee_PTR->flowerfield_PTR + food_source->heigt * bee_PTR->flowerfield_PTR->ff_width + (food_source->width - 1))->has_been_harvested == false){
                    if(nextfl->has_been_harvested == false){
                        pthread_error_funct(pthread_mutex_lock(bee_PTR->beehive_PTR->mutex_PTR));
                        myqueue_push(bee_PTR->beehive_PTR->queue, nextfl);
                        //fprintf(stderr, "queue empty: %s\n", myqueue_is_empty(bee_PTR->beehive_PTR->queue) ? "true" : "false");
                        pthread_error_funct(pthread_mutex_unlock(bee_PTR->beehive_PTR->mutex_PTR));

                        errno = 0;  //https://pubs.opengroup.org/onlinepubs/9699919799/functions/fprintf.html
                        if(sprintf(up, "(%llu,%llu)", nextfl->heigt, nextfl->width) <0){
                            perror("Sprintf");
                            exit(EXIT_FAILURE);
                        }
                    }
                    else{
                        memset(up, '\0', sizeof(up));
                    }
                }
                //down -> width same
                if(food_source->heigt < food_source->ff_height -1 ){
                    flower_t* nextfl = (bee_PTR->flowerfield_PTR + (food_source->heigt + 1)* bee_PTR->flowerfield_PTR->ff_width + (food_source->width));
                    //if((bee_PTR->flowerfield_PTR + food_source->heigt * bee_PTR->flowerfield_PTR->ff_width + (food_source->width - 1))->has_been_harvested == false){
                    if(nextfl->has_been_harvested == false){
                        pthread_error_funct(pthread_mutex_lock(bee_PTR->beehive_PTR->mutex_PTR));
                        myqueue_push(bee_PTR->beehive_PTR->queue, nextfl);
                       // fprintf(stderr, "queue empty: %s\n", myqueue_is_empty(bee_PTR->beehive_PTR->queue) ? "true" : "false");
                        pthread_error_funct(pthread_mutex_unlock(bee_PTR->beehive_PTR->mutex_PTR));
                        errno = 0;  //https://pubs.opengroup.org/onlinepubs/9699919799/functions/fprintf.html
                        if(sprintf(down, "(%llu,%llu)", nextfl->heigt, nextfl->width) <0){
                            perror("Sprintf");
                            exit(EXIT_FAILURE);
                        }
                    }
                    else{
                        memset(down, '\0', sizeof(down));
                    }
                }

                //fprintf(stderr, "DONE CHECKING SURROUNDINGS \n\n\n");
                char comp[256] = {"\0"};
                if(strcmp(comp, left) ==0 && strcmp(comp, right) ==0 && strcmp(comp, up) ==0 && strcmp(comp, down)== 0){
                    printf("Bee %llu collected nectar at position (%llu,%llu) and reports no other food sources.\n",
                           bee_PTR->bee_name, food_source->heigt, food_source->width);
                }
                else{
                    printf("Bee %llu collected nectar at position (%llu,%llu) and reports potential food sources: %s%s%s%s.\n",
                           bee_PTR->bee_name, food_source->heigt, food_source->width,
                           left, right, up, down);
                }


            }
            work_flower = false;
            //return home;
        }
        //else working at home
        else{
            printf("Bee %llu is working in beehive.\n", bee_PTR->bee_name);
            if(usleep(random_number * 1000) < 0){
                perror("Usleep");
                exit(EXIT_FAILURE);
            };
        }



        //possible bear attack:
        //simmilar to homework(e10) about barriers
        int ret = pthread_barrier_wait(bee_PTR->beehive_PTR->barrier_PTR);
        if (ret == 0) { goto nextwait; }    //All but 1 thread: go to next pthread_barrier_wait so all threads are at the same location again.
        else if (ret == PTHREAD_BARRIER_SERIAL_THREAD) // the ONE thread:
        {
            int random_chance = rand_r(&seed) % (100 - 0 + 1) + 1;
            //fprintf(stderr, "random chance: %d\n"
            //                "calculated by thread: %lu\n\n", random_chance, pthread_self());
            if(random_chance <= 10){
                //bear attack occurs:
                printf("Bees encounter a bear and engage in a fight.\n");
                int random_chance2 = rand_r(&seed) % (100 - 0 + 1) + 1;
                if(random_chance2<=50){
                    //bear destorys nest -> end of story
                    bee_PTR->beehive_PTR->actual_number_of_nectar_collected = bee_PTR->beehive_PTR->num_nectar;
                    bee_PTR->beehive_PTR->destroyed_by_bear = true;
                    bee_PTR->beehive_PTR->num_nectar = bee_PTR->beehive_PTR->max_number_nectar;
                    printf("Bear destroys the beehive.\n");
                }
                else{
                    printf("The bees successfully repel the bear and resume their work.\n");
                }
            }
        }
        else /*this means error in pthread_barrier_wait() */{
            pthread_error_funct(ret);
        }

        nextwait:
        int ret1 = pthread_barrier_wait(bee_PTR->beehive_PTR->barrier_PTR);
        if (ret1 != 0 && ret1 != PTHREAD_BARRIER_SERIAL_THREAD) { pthread_error_funct(ret1); }
    }

    //fprintf(stderr, "ALL FLOWERS HAVE BEEN HARVESTED\n");

    pthread_exit(NULL); //This function always succeeds.
}


//starting of main is simmilar in all recent homeworks
int main(int argc, char *argv[]) {
    check_argc(argc);

    unsigned long long ff_width = cast_to_ulli_with_check(argv[1]);
    unsigned long long ff_height = cast_to_ulli_with_check(argv[2]);
    unsigned long long  num_bees = cast_to_ulli_with_check(argv[3]);

    flower_t flowerfield[ff_height][ff_width];
    for (unsigned long long i = 0; i < ff_height; i++){
        for (unsigned long long j = 0; j < ff_width; j++){
            flowerfield[i][j].has_been_harvested = false;
            flowerfield[i][j].ff_height = ff_height;
            flowerfield[i][j].ff_width = ff_width;
            flowerfield[i][j].heigt = i;
            flowerfield[i][j].width = j;
        }
    }


    //Region bees:
    //needs queue:
    beehive_t beehive;
    myqueue queue;
    beehive.queue = &queue;
    myqueue_init(beehive.queue);
    beehive.max_number_nectar = ff_width * ff_height;
    beehive.num_nectar = 0;
    beehive.destroyed_by_bear = false;
    beehive.actual_number_of_nectar_collected = 0;

    //for bear attack : pthread_barrier:
    pthread_barrier_t barrier;
    beehive.barrier_PTR = &barrier;

    //copied from homework(e10)
    // * INIT BARRIER
    pthread_error_funct(pthread_barrier_init(beehive.barrier_PTR, NULL, num_bees));

    //simmilar to homework(06 or 08 or 09)
    pthread_mutex_t mutex;
    beehive.mutex_PTR = &mutex;
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
    pthread_error_funct(pthread_mutex_init(beehive.mutex_PTR, &mutex_attr));



    //push 1 food source in queue for first bee:

    i_dont_trust_myself_to_get_the_random_number_between_the_array_bounds:
    unsigned int seed = time(NULL) ^ pthread_self();
    unsigned long long random_height = rand_r(&seed) % ((ff_height-1) - 0 + 1) + (0);
    unsigned long long random_width = rand_r(&seed) % ((ff_width-1) - 0 + 1) + (0);
    //fprintf(stderr, "TESTING: %d", random_height);
    if(random_height > ff_height || random_width > ff_width){
        //this could create a very long loop time, but it should at least prevent an array_out_of_bounds problem.
        goto i_dont_trust_myself_to_get_the_random_number_between_the_array_bounds;
    }

    myqueue_push(beehive.queue, &flowerfield[random_height][random_width]);
    //fprintf(stderr, "after push \n");


    bee_t all_bees_array[num_bees];

    //create ptheads = bees
    //simmilar to all previous homeworks containing pthreads.
    pthread_t pid_array[num_bees];
    for(unsigned long long i = 0; i < num_bees; i++){
        //set beehive & flowerfield PTR
        all_bees_array[i].beehive_PTR = &beehive;
        all_bees_array[i].flowerfield_PTR = &flowerfield[0][0];
        all_bees_array[i].bee_name = i;
        //// Access elements using *(flowrefield_PTR + i * ff_width + j)
        pthread_error_funct(pthread_create(&pid_array[i],NULL, &pthreadStartRoutine, &all_bees_array[i] ));

    }

    //simmilar to all previous homeworks containing pthreads.
    for(unsigned long long i = 0; i< num_bees; i++){
        pthread_error_funct(pthread_join(pid_array[i], NULL));
    }

    //sanity checking: should not print anything when there is no bear!
    //for (unsigned long long i = 0; i < ff_height; i++){
    //    for (unsigned long long j = 0; j < ff_width; j++){
    //        if(flowerfield[i][j].has_been_harvested == false){
    //            fprintf(stderr, "A FLOWER HAS BEEN MISSED !!!!!!");
    //        }
    //    }
    //}

    if(beehive.destroyed_by_bear == true){
        printf("%llu bees collected nectar from %d/%llu flowers.\n", num_bees, beehive.actual_number_of_nectar_collected, (ff_height*ff_width));
        printf("Beehive was destroyed.\n");
    }
    else{
        printf("The bees have collected all the nectar form the flowerfield.\n");
        printf("The beehive was NOT destroyed.\n   ");
    }

    while(myqueue_is_empty(beehive.queue) == false){
        myqueue_pop(beehive.queue); //getting rid of all still stored jobs
    }


    return(EXIT_SUCCESS);
}