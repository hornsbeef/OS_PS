CFLAGS = -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=199309L -lrt -D_BSD_SOURCE -g -$(optimisation) -ldl
CC = cc
optimisation = O3


files_to_clean = *.so $(files_to_compile)

files_to_compile = map_string

all: $(files_to_compile:%=%.c)
	$(info  $(files_to_compile:%=%.c))
	$(CC) $(CFLAGS) -o $(word 1, $(files_to_compile)) $(word 1, $(files_to_compile:%=%.c))
	gcc -shared -o ceasar_chipher.so -fvisibility=hidden ceasar_chipher.c
	gcc -shared -o reverse_string.so -fvisibility=hidden reverse_string.c
	gcc -shared -o xor_string.so -fvisibility=hidden xor_string.c
	gcc -shared -o library_one.so -fvisibility=hidden library_one.c
	gcc -shared -o library_two.so -fvisibility=hidden library_two.c


.PHONY: clean run
clean:
	-rm -f $(files_to_clean)

run:
	./map_string "Hello World" ./ceasar_chipher.so ./xor_string.so ./reverse_string.so ./library_one.so ./library_two.so ./library_two.so ./library_one.so ./reverse_string.so ./xor_string.so ./ceasar_chipher.so