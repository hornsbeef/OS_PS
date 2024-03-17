// include necessary header files

// define signal handler

#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <signal.h>

void sighandler_funct(int signum){
    char* msg = "The following signal was recived: ";
    write(STDOUT_FILENO, msg, strlen(msg));

    char buffer[20];
    switch (signum){
        case 2: strcpy(buffer, "2 = SIGINT");break;
        case 9: strcpy(buffer, "9 = SIGKILL");break;    //cannot be handled by sigaction ?
        case 10: strcpy(buffer, "10 = SIGUSR1");break;
        case 12: strcpy(buffer, "12 = SIGUSR2");break;
        case 18: strcpy(buffer, "18 = SIGCONT");break;
        case 19: strcpy(buffer, "19 = SIGSTOP");break;  //cannot be handled by sigaction ?
        default: strcpy(buffer, "SIGNAL NOT ON LIST"); break; //if no other case sticks
    }
    write(STDOUT_FILENO, buffer, strlen(buffer));


}



int main(void) {
    //posting pid for ease of operation:
    write(STDOUT_FILENO, "PID: ", strlen("PID: "));
    char pid[10];
    sprintf(pid,"%d", getpid());
    write(STDOUT_FILENO,pid, strlen(pid));
    write(STDOUT_FILENO,"\n", strlen("\n") );

    // use sigaction to register signal handler
    //https://github.com/skuhl/sys-prog-examples/blob/master/simple-examples/sigaction.c
    //http://www.mathe2.uni-bayreuth.de/axel/unix_system_kap3.pdf -> signalmenge
    //https://www4.cs.fau.de/Lehre/SS13/V_SPIC/Uebung/Folien/u8-a6.pdf

    struct sigaction act;
    act.sa_handler = &sighandler_funct;     //function that handles what happens when the signal is received.

    sigfillset(&act.sa_mask);           //Signal mask to be used while handling the signal with sigaction

    act.sa_flags = SA_RESTART;              //if systemcall is interrupted by a signal -> systemcall is restarted.

    sigaction(SIGINT, &act, NULL);
    sigaction(SIGSTOP, &act, NULL);
    sigaction(SIGCONT, &act, NULL);
    sigaction(SIGKILL, &act, NULL);
    sigaction(SIGUSR1, &act, NULL);
    sigaction(SIGUSR2, &act, NULL);


    // allow required signals

    while (true) {
        usleep(100);
        // more code (if needed)
    }

    return EXIT_SUCCESS;
}