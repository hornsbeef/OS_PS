CFLAGS = -std=c11 -Wall -Wextra
CC = cc
files_to_clean = *.o main.c main
#files_to_clean = hello.o

all: main.o hello.o goodbye.o
	$(CC) $(CFLAGS) -o main $^


hello.o: hello.c
goodbye.o: goodbye.c
main.o: main.c

%.o: %.c
	$(info Compiling $(patsubst %.o,%,$@))
	$(CC) $(CFLAGS) -c -o $@ $^

hello: hello.c
goodbye: goodbye.c

%: %.c
	$(info Compiling $@)
	$(CC) $(CFLAGS) -c -o $@.o $^



.PHONY: clean main.c
clean:
	-rm -f $(files_to_clean)

main.c: create_main

create_main:
	$(file > main.c,$(make_code))

define make_code
#include "hello.h"
#include "goodbye.h"
#include <stdlib.h>

int main(void) {
	hello();
	goodbye();
	return EXIT_SUCCESS;
}
endef

#https://www.gnu.org/software/make/manual/make.html#Make-Control-Functions
#https://www.gnu.org/software/make/manual/html_node/Multi_002dLine.html

#https://www.gnu.org/software/make/manual/html_node/File-Function.html
## better explaining the file function: https://stackoverflow.com/a/30793196/23090293

#https://www.gnu.org/software/make/manual/html_node/Text-Functions.html
#https://www.gnu.org/software/make/manual/html_node/Substitution-Refs.html

#Section for depreciated lines:
#	#$(info Compiling $(@:.o= ))
