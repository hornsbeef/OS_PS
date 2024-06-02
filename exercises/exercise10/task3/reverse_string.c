//gcc -shared -o reverse_string.so -fvisibility=hidden reverse_string.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

__attribute__((visibility("default")))
char *map_string(char *str) {
    int len = strlen(str);
    char *result = (char *) malloc((len + 1) * sizeof(char));

    for (int i = 0; i < len; i++) {
        result[i] = str[len - 1 - i];
    }
    result[len] = '\0';
    return result;
}