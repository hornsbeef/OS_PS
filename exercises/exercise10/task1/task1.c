
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

#define MAX_NUMBER_OF_PLAYERS 1024
//typedef struct{
//    void* gameState_PTR;
//    int player_number;      //number in the players-array
//    pthread_t tid;
//    atomic_bool isLowest; //
//}player;
//
//typedef struct {
//    unsigned long long starting_number_of_players;
//    unsigned long long current_number_of_players;
//    pthread_barrier_t barrier;
//    //pthread_mutex_t* mutex_PTR;
//    atomic_int roll_result[MAX_NUMBER_OF_PLAYERS];
//    player players[MAX_NUMBER_OF_PLAYERS];
//}game_state;

typedef struct {
    atomic_int number_of_players;
    atomic_int number_of_players_still_playing; // * to check when only one player is left and has won
    pthread_barrier_t barrier;
    atomic_int roll_result[MAX_NUMBER_OF_PLAYERS];
    atomic_bool player_plays[MAX_NUMBER_OF_PLAYERS];
    pthread_t player_tids[MAX_NUMBER_OF_PLAYERS];

}game_state;

void check_argc(int argc);
unsigned long long int cast_to_ulli_with_check(char *string);
void pthread_error_funct(int pthread_returnValue);

void* player_funct(void* arg);


int main(int argc, char *argv[]) {
    //time_t t;
    //srand((unsigned) time(&t));
    check_argc(argc);

    game_state gameState;
    memset(gameState.roll_result, -1, MAX_NUMBER_OF_PLAYERS);
    memset(gameState.player_tids, -1, MAX_NUMBER_OF_PLAYERS);
    memset(gameState.player_plays, false, MAX_NUMBER_OF_PLAYERS);
    gameState.number_of_players = cast_to_ulli_with_check(argv[1]);
    gameState.number_of_players_still_playing = gameState.number_of_players;


    // * INIT BARRIER
    pthread_error_funct(pthread_barrier_init(&gameState.barrier, NULL, gameState.number_of_players));


//Region spawn player_threads
    for (int i = 0; i < gameState.number_of_players; ++i) {
        gameState.player_plays[i] = true;
        pthread_error_funct(pthread_create(&gameState.player_tids[i], NULL, player_funct, &gameState));
    }

//Region join player_threads
    for (int i = 0; i < gameState.number_of_players; ++i) {
        pthread_error_funct(pthread_join(gameState.player_tids[i], NULL));
    }

//Region cleanup:

    pthread_error_funct(pthread_barrier_destroy(&gameState.barrier));
    return(EXIT_SUCCESS);

}

//helper functs
void check_argc(int argc) {
    if (argc !=2) {
        printf("usage: ."__FILE__" <number of players>\n");
        exit(EXIT_FAILURE);
    }
}

unsigned long long int cast_to_ulli_with_check(char *string) {
    errno = 0;
    char *end = NULL;
    unsigned long long operand = strtoull(string, &end, 10);
    //check conversion:
    if ((*end != '\0') || (string == end)) {       //conversion interrupted || no conversion happened
        fprintf(stderr, "Conversion of argument ended with error.\n");
        if(errno != 0){
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


void* player_funct(void* arg){
    game_state* gameState_PTR = (game_state*) arg;
    pthread_t mytid = pthread_self();     //always succeeds
    int playerNumber = 0;

    //finding out THIS players number in the arrays
    for (; playerNumber < gameState_PTR->number_of_players; ++playerNumber) {
        if(mytid == gameState_PTR->player_tids[playerNumber]){break;}
    }

    while(gameState_PTR->number_of_players_still_playing>1){

        //do the random rolling
        if(gameState_PTR->player_plays[playerNumber] == true){
            gameState_PTR->roll_result[playerNumber] = (rand_r((unsigned int *) &mytid) % 6) + 1;
        }

//Region Barrier
        int ret = pthread_barrier_wait(&gameState_PTR->barrier);
        if(ret == 0){goto nextwait;}
        else if(ret == PTHREAD_BARRIER_SERIAL_THREAD){

            //Find the highest numbers in array. && print results
            int max = 0;
            for (int i = 0; i < gameState_PTR->number_of_players; ++i) {
                if(gameState_PTR->roll_result[i] > max){
                    max = gameState_PTR->roll_result[i];
                }
                if(gameState_PTR->roll_result[i] != 0){
                    printf("Player %d rolled a %d\n", i, gameState_PTR->roll_result[i]);
                }

            }
//printf( "The Max is %d\n", max);

            //Eliminate all players with "max"
            for (int i = 0; i < gameState_PTR->number_of_players; ++i) {
                if(gameState_PTR->roll_result[i] == max){
                    printf("Eliminating player %d\n",i);
                    gameState_PTR->player_plays[i] = false;
                    gameState_PTR->number_of_players_still_playing--;
                }
            }
            printf("---------------------\n");
            memset(gameState_PTR->roll_result, 0, MAX_NUMBER_OF_PLAYERS);

        }
        else /*this means error in pthread_barrier_wait() */{
            pthread_error_funct(ret); //TODO: see if this works
        }

        nextwait:
        int ret1 = pthread_barrier_wait(&gameState_PTR->barrier);
        if(ret1 != 0 && ret1 != PTHREAD_BARRIER_SERIAL_THREAD){ pthread_error_funct(ret1);}
        //repeat
    }

//Region Player X won

    int ret2 = pthread_barrier_wait(&gameState_PTR->barrier);
    if(ret2 == 0){goto return_label;}
    else if(ret2 == PTHREAD_BARRIER_SERIAL_THREAD){
        int winningPlayer = 0;
        for (; winningPlayer < gameState_PTR->number_of_players; ++winningPlayer) {
            if(gameState_PTR->player_plays[winningPlayer]){break;}
        }
        printf("Player %d has won the game.", winningPlayer);
    }
    else /*this means error */{
        pthread_error_funct(ret2); //TODO: see if this works
    }

    return_label:
    pthread_exit(NULL);

}
