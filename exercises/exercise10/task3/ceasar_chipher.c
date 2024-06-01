#include <ctype.h>
#include <stdlib.h>
#include <string.h>

__attribute__((visibility("default")))
char *map_string(char *str) {
    int len = strlen(str);
    char *result = (char *)malloc((len + 1) * sizeof(char));

    for (int i = 0; i < len; i++) {
        char c = str[i];
        if (isalpha(c)) {
            if (islower(c)) {
                c = 'a' + (c - 'a' + 13) % 26;
            } else {
                c = 'A' + (c - 'A' + 13) % 26;
            }
        }
        result[i] = c;
    }
    result[len] = '\0';
    return result;
}