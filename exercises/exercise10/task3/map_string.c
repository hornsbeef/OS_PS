//gcc -o map_string map_string.c -ldl
//gcc -shared -o ceasar_chipher.so -fvisibility=hidden ceasar_chipher.c
//./map_string "Hello World" ./ceasar_chipher.so


#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE
// #define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <dlfcn.h>

typedef char *(*plugin_func_t)(char *);


void check_argc(int argc) {
    if (argc <= 2) {
        printf("usage: ."__FILE__" <string> <list of plugins>");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    check_argc(argc);

    char *input_string = argv[1];
    char *result = strdup(input_string);

    for (int i = 2; i < argc; i++) {
        void *handle = dlopen(argv[i], RTLD_LAZY);
        if (!handle) //TODO: check if error handling is good
        {
            fprintf(stderr, "Error loading plugin %s: %s\n", argv[i], dlerror());
            continue;
        }

        plugin_func_t plugin_func = (plugin_func_t)dlsym(handle, "map_string");
        if (!plugin_func) {
            fprintf(stderr, "Error finding map_string function in %s: %s\n", argv[i], dlerror());
            dlclose(handle);
            continue;
        }

        char *new_result = plugin_func(result);
        free(result);
        result = new_result;

    }

    printf("Result: %s\n", result);
    free(result);
    return 0;


}