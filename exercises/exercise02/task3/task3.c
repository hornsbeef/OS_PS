
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

double DR_p(int T, int64_t S) {
    int64_t hit_count = 0;
    for (int64_t i = 0; i < S; ++i) {
        const int roll = rand() % 6 + 1;
        if (roll == T) {
            hit_count++;
        }
    }
    return (double)hit_count / S;
}

void child_funct(int T, int64_t S, struct timespec start_time){

}


int main (int argc, char* argv[]){

    if (argc <2 || argc > 3){   //still testing right coditions...
        USAGE:
            fprintf(stderr, "usage: ./"__FILE__ " <N child processes> <S steps> \n");
            return EXIT_FAILURE;
    }


    char* end = NULL;
    long N = strtol(argv[1], &end, 10);
    if(*(char*)end != '\0'){
        goto USAGE;
    }
    end = NULL;
    int64_t S = strtol(argv[2], &end, 10);
    if(*(char*)end != '\0'){
        goto USAGE;
    }

    //TODO: DEBUB:
    //printf("%d", N);
    //printf("%d", S);

    struct timespec start_time;
    clock_gettime(CLOCK_REALTIME, &start_time);

    //TODO:DEBUG:
    //printf("sec: %ld", start_time.tv_sec);


    int PID = -1;


    for(int i = 0; i <= N; i++){
        if(PID == 0 ){
            //this is child:


            int T = rand() % 6 +1;      //create random T for child

            int child_number = i;

            int child_PID = getpid();

            printf("Child %d PID = %d. DR_p(%d,%ld) = XXX. Elapsed time = <t> (s).\n", child_number, child_PID, T, S );

            //todo: must exit? and not continue in for loop!
            //temporary:
            break;
        }else{
            //this is parent:
            PID = fork();



        }
    }
        //todo: for parent: wait() or waitpid();




}