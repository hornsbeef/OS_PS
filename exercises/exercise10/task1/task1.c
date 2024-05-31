
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
typedef struct{
    void* gameState_PTR;
    int player_number;      //number in the players-array
    pthread_t tid;
    atomic_bool isLowest; //
}player;


typedef struct {
    unsigned long long starting_number_of_players;
    unsigned long long current_number_of_players;
    pthread_barrier_t barrier;
    atomic_int roll_result[MAX_NUMBER_OF_PLAYERS];
    player players[MAX_NUMBER_OF_PLAYERS];
}game_state;


void check_argc(int argc);
unsigned long long int cast_to_ulli_with_check(char *string);
void pthread_error_funct(int pthread_returnValue);

void* player_funct(void* arg);


int main(int argc, char *argv[]) {

    check_argc(argc);

    game_state gameState;
    memset(gameState.roll_result, -1, MAX_NUMBER_OF_PLAYERS);
    gameState.starting_number_of_players = cast_to_ulli_with_check(argv[1]);
    gameState.current_number_of_players = gameState.starting_number_of_players;

    // * INIT BARRIER
    pthread_error_funct(pthread_barrier_init(&gameState.barrier, NULL, gameState.current_number_of_players));

    for (int i = 0; i < gameState.starting_number_of_players; ++i) {
        gameState.players[i].gameState_PTR = &gameState;
        gameState.players[i].player_number = i;         //TODO: check if needed
        pthread_error_funct(pthread_create(&gameState.players[i].tid, NULL, player_funct, &gameState.players[i]));
    }


}

//helper functs
void check_argc(int argc) {
    if (argc !=2) {
        printf("usage: ."__FILE__" <number of players>");
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