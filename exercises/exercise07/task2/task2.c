#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

///////////////////////////////////////////////////////////////////////////////

#define NUM_PHILOSOPHER 10

#define EAT_ITERATIONS 5000 //number of times each thread tries to "eat"
#define EAT_DURATION 100    //usleep duration

#define RIGHT_CHOPSTICK(n) (n)  //simply returns n -> code-bloat
#define LEFT_CHOPSTICK(n) (((n) + 1) % NUM_PHILOSOPHER) //takes n and returns the next higher number as long as it is not higher than number of mutexes(=pthread = philosophers)

//////////////////////////////////////////////////////////////////////////////////

#define WITHDEADLOCK 0
#define WITHOUTDEADLOCK 1


//create same number of threads and mutexes
pthread_t philosopher[NUM_PHILOSOPHER];     //threads
pthread_mutex_t chopstick[NUM_PHILOSOPHER]; //mutexes

void* dine(void* id);

void swap(int *pInt, int *pInt1);

int main(void) {

	for (int i = 0; i < NUM_PHILOSOPHER; ++i) {
		pthread_mutex_init(&chopstick[i], NULL);    //same number of mutexes and threads are created.
	}

	for (int i = 0; i < NUM_PHILOSOPHER; ++i) {
		pthread_create(&philosopher[i], NULL, dine, (void*)(intptr_t)i);    //philosophers == threads are created.
	}
    //The variable i is a local loop counter within the for loop.
    // It's declared inside the loop's scope, so it has a separate instance for each iteration.
    // This means each thread gets its own copy of i with a unique value


    //thread joining and mutex cleanup
	for (int i = 0; i < NUM_PHILOSOPHER; ++i) {
		pthread_join(philosopher[i], NULL);
	}
	for (int i = 0; i < NUM_PHILOSOPHER; ++i) {
		pthread_mutex_destroy(&chopstick[i]);
	}

	return EXIT_SUCCESS;
}

#if WITHDEADLOCK
//pthread worker function.
void* dine(void* id) {
    int n = (int)(intptr_t)id;  //"slot" each thread got assigned to in the philosopher-array

    for (int i = 0; i < EAT_ITERATIONS; ++i) {
        pthread_mutex_lock(&chopstick[RIGHT_CHOPSTICK(n)]); //n
        //here is where a deadlock might occur.
        //Say we have threads A-E and therefore mutex 1-5:
        //each thread tries to first lock the mutex with a number corresponding to their "position"/"name"
        //then each thread tries to lock the mutex with the next higher number.
        // e.g: A tries to lock mutex 1, and if successful tries to lock 2.
        //      B tries to lock mutex 2, and if successful tries to lock 3.
        //      ...
        //here the potential for a deadlock becomes visible:
        //if B managed to lock mutex 2 before A could do so -> then A waits until it can lock mutex 2, but keeps on holding mutex 1.
        //now when we go "through the whole circle" (A->B->C->D->E->A...)
        //and assume that:
        // C got mutex 3, (needs 4)
        // D got mutex 4, (needs 5)
        // E got mutex 5. (needs 1 - held by A)
        //-> no thread can finish its execution and release its lock, therefore creating a deadlock.
        pthread_mutex_lock(&chopstick[LEFT_CHOPSTICK(n)]);  //n+1 (is "circle" -> next in line)
        usleep(EAT_DURATION);
        pthread_mutex_unlock(&chopstick[LEFT_CHOPSTICK(n)]);    //n+1 (is "circle" -> next in line)
        pthread_mutex_unlock(&chopstick[RIGHT_CHOPSTICK(n)]);   //n
    }
    printf("Philosopher %d is done eating!\n", n);

    return NULL;
}
#endif

#if WITHOUTDEADLOCK
//pthread worker function.
void* dine(void* id) {
    int n = (int)(intptr_t)id;  //"slot" each thread got assigned to in the philosopher-array

    int lower = RIGHT_CHOPSTICK(n);
    int higher = LEFT_CHOPSTICK(n);
    if(lower > higher){
        swap(&higher, &lower);
    }
    //solution by introducing
    //the above 5 lines make sure, that each thread first locks the "lower numbered mutex"
    //This means only for the last thread the oreder changes from the original approach:
    //the last thread will try to lock 0 before locking "its own" mutex.
    // therefore the circular waiting component of deadlocks is removed -> no deadlock can occur.

    for (int i = 0; i < EAT_ITERATIONS; ++i) {
        pthread_mutex_lock(&chopstick[lower]);
        pthread_mutex_lock(&chopstick[higher]);
        usleep(EAT_DURATION);
        pthread_mutex_unlock(&chopstick[higher]);
        pthread_mutex_unlock(&chopstick[lower]);
    }
    printf("Philosopher %d is done eating!\n", n);

    return NULL;
}

void swap(int *var1, int *var2) {
    int num1 = *var1;
    int num2 = *var2;
    num1 = num1 + num2;
    num2 = num1 - num2; //num2 becomes num1
    num1 = num1 - num2; //sum - old num1 = num2 -> num1 becomes num2
    *var1 = num1;
    *var2 = num2;
}

#endif