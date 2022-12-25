/**
 * @file oopc_example.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/10/20
 *
 * @example
 *
 * @development_log
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

ptrdiff_t data_offset[] = {0};
uint8_t type_index[] = {0};

enum {
	TYPE_UNKNOWN = 0,
	TYPE_ANIMAL,
	TYPE_CAT,
	TYPE_DOTTED_CAT
};

typedef struct animal {
	uint8_t type_;

	int legs;
} animal_t;

typedef struct cat {
	uint8_t type_;

	int gender;

	animal_t inherit_;
} cat_t;

typedef struct dotted_cat {
	uint8_t type_;

	float color;

	cat_t inherit_;
} dotted_cat_t;




int main()
{

	return 0;
}



