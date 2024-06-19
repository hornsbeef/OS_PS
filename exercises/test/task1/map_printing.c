#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "map_printing.h"

void print_surroundings(size_t size) {
	printf("+");
	for (size_t i = 0; i < size; i++) {
		printf("-");
	}
	printf("+\n");
}

void print_center(char* str, size_t size) {
	int len = strlen(str);
	printf("|%*s%s%*s|\n", (int)((size) / 2 - len / 2), " ", str,
	       (int)(size - ((size) / 2) - (len - len / 2)), " ");
}

void print_box(char* str, size_t size) {
	print_surroundings(size);
	print_center(str, size);
	print_surroundings(size);
}

void print_daily_map(size_t day, cell_type_t** map, size_t size) {
	size_t day_str_len = snprintf(NULL, 0, "Day %zu", day);
	day_str_len++;
	char* day_str = (char*)malloc(day_str_len);
	snprintf(day_str, day_str_len, "Day %zu", day);

	print_box(day_str, size);

	for (size_t i = 0; i < size; i++) {
		printf("|");
		for (size_t j = 0; j < size; j++) {
			switch (map[i][j]) {
				case NOCHECK: printf("#"); break;
				case EMPTY: printf("\033[0;34m.\033[0m"); break;
				case ITEM: printf("\033[0;32mI\033[0m"); break;
				case MONSTER: printf("\033[0;31mM\033[0m"); break;
				case MONEY: printf("\033[0;33m$\033[0m"); break;
			}
		}
		printf("|\n");
	}

	print_surroundings(size);
	free(day_str);
}

void print_title() {
	print_surroundings(101);
	print_center(",------.                            ,--.       ,--.         ", 101);
	print_center("|  .---',--.  ,--.,--,--.,--,--,  ,-|  |,--.--.`--' ,--,--. ", 101);
	print_center("|  `--,  \\  `'  /' ,-.  ||      \\' .-. ||  .--',--.' ,-.  | ", 101);
	print_center("|  `---. /  /.  \\\\ '-'  ||  ||  |\\ `-' ||  |   |  |\\ '-'  | ", 101);
	print_center("`------''--'  '--'`--`--'`--''--' `---' `--'   `--' `--`--' ", 101);
	print_surroundings(101);
}
