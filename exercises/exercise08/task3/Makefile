# $RM instead of rm -> more independence form system.
CFLAGS = -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=199309L -lrt -D_BSD_SOURCE -g -$(optimisation)
#maybe needs these flags for sigaction! CFLAGS = -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=199309L -lrt -D_POSIX_C_SOURCE -D_BSD_SOURCE -g
CC = cc

optimisation = O3

files_to_clean = *.o $(files_to_compile) valgrind_*.txt helgrind_*.txt drd_*.txt run_*.txt
files_to_compile = server

#might be problematic with multi-file make
program_args = 4445 5

run_times = 10



all: $(files_to_compile:%=%.o)
	$(info  $(files_to_compile:%=%.o))
	$(CC) $(CFLAGS) -o $(word 1, $(files_to_compile)) $(word 1, $(files_to_compile:%=%.o))
	#$(CC) $(CFLAGS) -o $(word 2, $(files_to_compile)) $(word 2, $(files_to_compile:%=%.o))


%.o: %.c
	$(info Compiling $(patsubst %.o,%,$@):)
	$(CC) $(CFLAGS) -c -o $@ $^


%: %.c
	$(info Compiling $@)
	$(CC) $(CFLAGS) -c -o $@.o $^


.PHONY: clean main.c
clean:
	-rm -f $(files_to_clean)


#maybe not going to work for multi-file make!
.PHONY: valgrind
valgrind:
	valgrind --tool=memcheck --log-file="valgrind_"$(word 1, $(files_to_compile))".txt" --verbose --trace-children=yes ./$(word 1, $(files_to_compile)) $(program_args)
	#valgrind --tool=memcheck --log-file="valgrind_"$(word 2, $(files_to_compile))".txt" --verbose --trace-children=yes ./$(word 2, $(files_to_compile)) $(program_args)

.PHONY: time
time:
	/usr/bin/time -v ./$(word 1, $(files_to_compile)) $(program_args)
	#/usr/bin/time -p ./$(word 2, $(files_to_compile)) $(program_args)

.PHONY: helgrind
helgrind:
	valgrind --tool=helgrind --log-file="helgrind_"$(word 1, $(files_to_compile))".txt" ./$(word 1, $(files_to_compile)) $(program_args)
	#valgrind --tool=helgrind --log-file="helgrind_"$(word 1, $(files_to_compile))".txt" --verbose --trace-children=yes ./$(word 1, $(files_to_compile)) $(program_args)
	#valgrind --tool=helgrind --log-file="helgrind_"$(word 2, $(files_to_compile))".txt" --verbose --trace-children=yes ./$(word 2, $(files_to_compile)) $(program_args)

.PHONY: drd
drd:
	valgrind --tool=drd --log-file="drd_"$(word 1, $(files_to_compile))".txt" ./$(word 1, $(files_to_compile)) $(program_args)
	#valgrind --tool=drd --log-file="drd_"$(word 1, $(files_to_compile))".txt" --verbose --trace-children=yes ./$(word 1, $(files_to_compile)) $(program_args)
	#valgrind --tool=drd --log-file="drd_"$(word 2, $(files_to_compile))".txt" --verbose --trace-children=yes ./$(word 2, $(files_to_compile)) $(program_args)

.PHONY: run
run:
	@for i in $(shell seq 1 $(run_times)); do \
		echo $$i >>"run_"$(run_times)"_"$(word 1, $(files_to_compile))".txt"; \
		./$(word 1, $(files_to_compile)) $(program_args) >> "run_"$(run_times)"_"$(word 1, $(files_to_compile))".txt"; \
		echo "------------------------------------------" >>"run_"$(run_times)"_"$(word 1, $(files_to_compile))".txt"; \
  	done

#https://www.gnu.org/software/make/manual/make.html#Make-Control-Functions
#https://www.gnu.org/software/make/manual/html_node/Multi_002dLine.html

#https://www.gnu.org/software/make/manual/html_node/File-Function.html
## better explaining the file function: https://stackoverflow.com/a/30793196/23090293

#https://www.gnu.org/software/make/manual/html_node/Text-Functions.html
#https://www.gnu.org/software/make/manual/html_node/Substitution-Refs.html

