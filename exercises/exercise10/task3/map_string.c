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
        if (handle == NULL) {
            fprintf(stderr, "Error loading plugin %s: %s\n", argv[i], dlerror());
            continue;   //try to keep working without the missing/bad plugin
        }

        /**
         *The correct way to distinguish an error from
            a symbol whose value is NULL is to call dlerror(3) to clear any
            old error conditions, then call dlsym(), and then call dlerror(3)
            again, saving its return value into a variable, and check whether
            this saved value is not NULL.
         *
         * dlerror() returns NULL if no errors have occurred since
            initialization or since it was last called.
         */

        dlerror();
        plugin_func_t plugin_func = (plugin_func_t) dlsym(handle, "map_string");
        char *dlerror_return = dlerror();
        if (dlerror_return != NULL) {
            fprintf(stderr, "Error finding map_string function in %s: %s\n", argv[i], dlerror_return);
            dlclose(handle);
            continue;
        }

        char *new_result = plugin_func(result);

        dlerror();
        if (dlclose(handle) != 0)
        {
            fprintf(stderr, "Error loading plugin %s: %s\n", argv[i], dlerror());
            //try to keep the program running, the frees will be done by the OS.
        }

        free(result);
        result = new_result;
        printf("%s: %s\n", argv[i], new_result);

    }

    //printf("Result: %s\n", result);
    free(result);
    return 0;


}