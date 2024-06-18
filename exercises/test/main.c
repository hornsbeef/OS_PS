#include <stdio.h>
#include <string.h>

void reverse_string(char* str) {
    int len = strlen(str);
    for(int i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = temp;
    }
}

int main(void) {
    char test[] = "Hello, World";
reverse_string(test);
printf("%s!\n", test);



    //write me a function that takes in a string and returns the string in reverse

}

