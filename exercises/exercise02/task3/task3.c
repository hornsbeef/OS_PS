#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define BILLION 1E9

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

int main(int argc, char* argv[]) {

	if (argc < 2 || argc > 3) { // still testing right coditions...
	USAGE:
		fprintf(stderr, "usage: ./"__FILE__
		                " <N child processes> <S steps> \n");
		return EXIT_FAILURE;
	}
	char* end = NULL;
	long N = strtol(argv[1], &end, 10);
	if (*(char*)end != '\0') {
		goto USAGE;
	}
	end = NULL;
	int64_t S = strtol(argv[2], &end, 10);
	if (*(char*)end != '\0') {
		goto USAGE;
	}
	////////////////////////////////////////////////////////////
	// actual fork implementation:

	// timer start:
	struct timespec start_time;
	clock_gettime(CLOCK_REALTIME, &start_time);

	int PID = -1;
	int i;
	// N = 3
	for (i = 0; i < N; i++) {
		// while(i<N){
		if (PID == 0) {
			// this is child:
			break;
		} else {
			// this is parent:
			PID = fork();
			// printf("this is parent number: %d! -> creating FORK: %d\n", PID, i+1);
		}
	}

	if (PID == 0) {
		srand(getpid()); // seeding rand(); -> in child-process, not parent! because parent-pid stays
		                 // the same!
		int T = rand() % 6 + 1; // create random T for child
		int child_number = i;
		pid_t child_PID = getpid();

		double probability = DR_p(T, S);

		struct timespec end_time;
		clock_gettime(CLOCK_REALTIME, &end_time);

		double time_elapsed = (end_time.tv_sec - start_time.tv_sec) +
		                      ((end_time.tv_nsec - start_time.tv_nsec) / (double)BILLION);

		printf("Child %d PID = %d. DR_p(%d,%ld) = %f. Elapsed time = %f (s).\n", child_number,
		       child_PID, T, S, probability, time_elapsed);
	}

	if (PID != 0) {
		while (N >= 0) { // waiting for all child processes; important: "If a child has already changed
			               // state, then these calls return immediately."
			wait(NULL); // because I'm not interested in the status
			N--;
		}

		printf("Done.");
	}

	return EXIT_SUCCESS;
}