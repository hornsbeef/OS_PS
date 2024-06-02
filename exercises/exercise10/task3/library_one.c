#include <stdio.h>
#include <stdlib.h>
#include <string.h>

__attribute__((visibility("default")))
char *map_string(char *str) {
    int len = strlen(str);
    char *result = (char *) malloc((len + 1) * sizeof(char));

    for (int i = 0; i < len; i++) {
        char c = str[i];
        if (c == 'e') {
            result[i] = 't';
        } else if (c == 't') {
            result[i] = 'e';
        } else if (c == 'E') {
            result[i] = 'T';
        } else if (c == 'T') {
            result[i] = 'E';
        } else {
            result[i] = c;
        }
    }
    result[len] = '\0';
    return result;
}