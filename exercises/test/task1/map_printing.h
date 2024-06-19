#ifndef MAP_PRINTING_H
#define MAP_PRINTING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
	EMPTY = 0,
	MONEY = 1,
	MONSTER = 2,
	ITEM = 3,
	NOCHECK = 99
} cell_type_t;

void print_surroundings(size_t size);
void print_center(char* str, size_t size);
void print_box(char* str, size_t size);
void print_daily_map(size_t day, cell_type_t** map, size_t size);
void print_title();

#endif