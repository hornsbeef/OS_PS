#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include <stdio.h>

void* string_shift_right(void* arg) {
	char* input = (char*)arg;   //todo: check if here strcpy would have been better.


	size_t input_length = strlen(input);
	char shifted_input[input_length];   //todo: maybe +1 for \0 ?


    //is this trying to do the shift operation?
    //maybe better like this: https://www.perplexity.ai/search/c-programming-rightshift-97L56ybmTdyonOiheOi8ng#1
	for (size_t char_index = 0; char_index < input_length; ++char_index) {
		size_t new_position = (char_index + 2) / input_length;
		shifted_input[new_position] = input[char_index];    //todo: needs some dereferencing here
	}

	for (size_t char_index = 0; char_index < input_length; ++char_index) {
		input[char_index] = toupper(input[char_index]);
	}

	return shifted_input;
}



int main(int argc, const char** argv) {
	char* shared_strings[argc];     //array of char* ; times number of argc


	for (int arg_index = 0; arg_index < argc; ++arg_index) {    //loop: for all argv arguments

		//size_t arg_length = strlen(argv[arg_index]);            //FOUND: size of the argv length must be +1 for \0 !
		size_t arg_length = strlen(argv[arg_index]) + 1;            //size of the argv length //todo: check if that is good

        shared_strings[arg_index] = malloc(arg_length * sizeof(char));

		if (shared_strings[arg_index] == NULL) {
			fprintf(stderr, "Could not allocate memory.\n");
			exit(EXIT_FAILURE);
		}
		strcpy(shared_strings[arg_index], argv[arg_index]);
	}


	pthread_t t_ids[argc];          //array for thread-IDs



	//for (int arg_index = 0; arg_index <= argc; arg_index++) { //FOUND: error in for-loop: used <= insted of <
	for (int arg_index = 0; arg_index < argc; arg_index++) {
		pthread_create(&t_ids[arg_index], NULL, string_shift_right, shared_strings[arg_index]); //todo: check maybe needs address / void_ptr here.
		//free(shared_strings[arg_index]);            //FOUND: check why free here already?? -> commented out

	}



	//for (int arg_index = 0; arg_index <= argc; arg_index++) { //FOUND: error in for-loop: used <= insted of <
	for (int arg_index = 0; arg_index < argc; arg_index++) {
		char* shifted_string;
		waitpid(*t_ids + arg_index, (void*)&shifted_string, 0);

		printf("original argv[%d]: %s\n"
               "uppercased: %s\nshifted: %s\n"
               , arg_index, argv[arg_index], shared_strings[arg_index], shifted_string);

    }


    //TODO: might need free of the shard_strings[arg_index] after completion



	return EXIT_SUCCESS;
}
