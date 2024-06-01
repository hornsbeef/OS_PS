
files_to_clean = some_library.o libsome_library.so my_program

lib: some_library.o
	gcc -shared -fPIC -o libsome_library.so some_library.o

my_program:
	gcc -static -o my_program my_program.c

some_library.o: some_library.c
	gcc -c some_library.c -o some_library.o



.PHONY: clean
clean:
	-rm -f $(files_to_clean)