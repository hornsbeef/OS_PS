
files_to_clean = some_library.o libsome_library.so my_program my_program.o

lib: some_library.o
	gcc -shared -fPIC -o libsome_library.so some_library.o

my_program:
	gcc -static -o my_program my_program.c

my_program_zid:
	$(info linked dynamically because zid does not support static linking)
	gcc -o my_program my_program.c

	#gcc -c some_library.c -o some_library.o
	#gcc -c my_program.c -o my_program.o
	#gcc -o my_program my_program.o some_library.o -ldl -static



some_library.o: some_library.c
	gcc -c some_library.c -o some_library.o



.PHONY: clean
clean:
	-rm -f $(files_to_clean)