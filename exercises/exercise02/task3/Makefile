CFLAGS = -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=199309L
CC = cc
executable = task3
files_to_clean = $(executable) *.o


all: $(executable).o
	$(CC) $(CFLAGS) -o $(executable) $^

%.o: %.c
	$(info Compiling $(patsubst %.o,%,$@))
	$(CC) $(CFLAGS) -c -o $@ $^

.PHONY: clean
clean:
	-rm -f $(files_to_clean)




#https://www.gnu.org/software/make/manual/make.html#Make-Control-Functions
#https://www.gnu.org/software/make/manual/html_node/Multi_002dLine.html

#https://www.gnu.org/software/make/manual/html_node/File-Function.html
## better explaining the file function: https://stackoverflow.com/a/30793196/23090293

#https://www.gnu.org/software/make/manual/html_node/Text-Functions.html
#https://www.gnu.org/software/make/manual/html_node/Substitution-Refs.html

#Section for depreciated lines:
#	#$(info Compiling $(@:.o= ))
