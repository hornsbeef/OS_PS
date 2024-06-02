//gcc -shared -o xor_string.so -fvisibility=hidden xor_string.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

__attribute__((visibility("default")))
char *map_string(char *str) {
    int len = strlen(str);
    char *result = (char *) malloc((len + 1) * sizeof(char));

    for (int i = 0; i < len; i++) {
        char c = str[i];
        if (isalpha(c)) {
            //if (islower(c)) {
            //    c = 'a' + (c - 'a' ^ 0x20) % 26;
            //} else {
            //    c = 'A' + (c - 'A' ^ 0x20) % 26;
            //}
            c = c ^ 0x20;
        }
        result[i] = c;
    }
    result[len] = '\0';
    return result;
}