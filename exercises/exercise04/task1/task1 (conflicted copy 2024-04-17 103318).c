.,#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include <stdio.h>
#include <errno.h>

void* string_shift_right(void* arg) {
    //INTENDED: The job of the thread is shifting this argument to the right by two (2) positions (into a new array),
    //          and converting the input string to upper case (in-place).
    //          In order to make the shifted string accessible to the main thread,
    //          a pointer to it should be returned by the thread.


	char* input = (char*)arg;

	size_t input_length = strlen(input);

    //INTENDED: new Array for shifting:
	//char shifted_input[input_length];   //FOUND: needs malloc otherwise will disappear when function returns!
    errno = 0;
	char* shifted_input = malloc(input_length * sizeof(*shifted_input) +1); //+1 is for the \0 ending
    if(shifted_input == NULL){
        perror( "Could not allocate memory");
        exit(EXIT_FAILURE);
    }


    /* FOUND shift operation logic seems flawed -> only writes to index 0 of array.
	for (size_t char_index = 0; char_index < input_length; ++char_index) {
        size_t new_position = (char_index + 2) / input_length;
		shifted_input[new_position] = input[char_index];
	}
    */
    //better like this:
    memmove(&shifted_input[0], &input[(input_length) -2], (2* sizeof(char)));   //last two chars into first pos of shifted array
    memmove(&shifted_input[2], &input[0], (input_length * sizeof(char)) - 2);
    shifted_input[input_length] = '\0';


    //INTENDED: convert to uppper case in place:
	for (size_t char_index = 0; char_index < input_length; ++char_index) {
		input[char_index] = toupper(input[char_index]);
	}

    //INTENDED: return pointer to shifted array to main thread:
	//return shifted_input; FOUND: needs pthread_exit for returning the pointer!
    pthread_exit(shifted_input);
}



int main(int argc, const char** argv) {
	char* shared_strings[argc];     //array of char* ; times number of argc


    //INTENDED: First, all command line arguments should be copied into heap memory.
	for (int arg_index = 0; arg_index < argc; ++arg_index) {    //loop: for all argv arguments

		//size_t arg_length = strlen(argv[arg_index]);            //FOUND: size of the argv length must be +1 for \0 !
		size_t arg_length = strlen(argv[arg_index]) + 1;            //size of the argv length //todo: check if that is good


        errno = 0;
        shared_strings[arg_index] = malloc(arg_length * sizeof(char));
		if (shared_strings[arg_index] == NULL) {
			//fprintf(stderr, "Could not allocate memory.\n"); //FOUND use perror to display errno <- is set by malloc
			perror("Could not allocate memory");        //should set errno to 0 before
			exit(EXIT_FAILURE);
		}


		strcpy(shared_strings[arg_index], argv[arg_index]);
	}



    //INTENDED: Then, one thread per command line argument is spawned (so in total argc threads), and each thread works on a single one of these arguments.
    //INTENDED: Each thread receives a pointer to the copy of the argument it should work on.
	pthread_t t_ids[argc];  //array for thread-IDs
    // FOUND: error in for-loop: used <= insted of <
	//for (int arg_index = 0; arg_index <= argc; arg_index++) {
	for (int arg_index = 0; arg_index < argc; arg_index++) {
		pthread_create(&t_ids[arg_index], NULL, string_shift_right, shared_strings[arg_index]);
        //FOUND: Free here to early - shared_strings[] are still in use!
        //free(shared_strings[arg_index]);

	}


    //INTENDED: Once the main thread is done spawning the threads,
    //          it waits for their completion and for each completed thread,
    //          the original argument it received, the upper-cased version, and the shifted argument are printed.
    //FOUND: error in for-loop: used <= insted of <
	//for (int arg_index = 0; arg_index <= argc; arg_index++) {
	for (int arg_index = 0; arg_index < argc; arg_index++) {
		char* shifted_string;
		//waitpid(*t_ids + arg_index, (void*)&shifted_string, 0);   //FOUND: waitpid is used for PROCESSES not Threads!
		pthread_join(t_ids[arg_index], (void **) &shifted_string); //use pthread_join instead!

		printf("original argv[%d]: %s\n"
               "uppercased: %s\n"
               "shifted: %s\n",
               arg_index,
               argv[arg_index],
               shared_strings[arg_index],
               shifted_string
               );
        //FOUND: need to free malloc'd shifted_input from thread_function -> here shifted_string:
        free(shifted_string);

        //FOUND: need to free malloc'd  shared_strings[] from main function
        free(shared_strings[arg_index]);
    }


	return EXIT_SUCCESS;
}
