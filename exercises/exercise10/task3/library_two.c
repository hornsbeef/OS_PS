#include <stdio.h>
#include <stdlib.h>
#include <string.h>

__attribute__((visibility("default")))
char *map_string(char *str) {
    int len = strlen(str);
    char *result = (char *) malloc((len * 2 + 1) * sizeof(char));
    memset(result, '\0', len * 2 + 1);

    int j = 0;

    for (int i = 0; i < len; ++i) {
        char c = str[i];
        if (i == len - 1) {
            result[j] = c;
            continue;
        }
        char d = str[i + 1];

        //remove double:
        if (c == d) {
            result[j] = c;
            j++;
            i++; //skipping the second (double) char
        } else {
            result[j] = c;
            result[j + 1] = c;
            j += 2;
        }
    }

    return result;
}