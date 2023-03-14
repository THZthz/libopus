/**
 * @file heap_demo.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2023/3/13
 *
 * @example
 *
 * @development_log
 *
 */

#include <stdio.h>
#include "data_structure/heap.h"

void p(opus_heap *heap)
{
	int *arr = heap->data_;
	int  i;
	for (i = 1; i <= heap->last_index; i++)
		printf("%d ", arr[i]);
}

int cmp(opus_heap *heap, const void *a, const void *b)
{
	return *(int *) a - *(int *) b;
}

int main()
{
	opus_heap *h     = opus_heap_create(sizeof(int), cmp);
	int        arr[9] = {1, 3, 2, 5, 3, 6, 7, 9, 8};
	opus_arr_reserve(h->data_, 10);
	memcpy(h->data_, arr, sizeof(arr));
	opus_heap_update(h, 9);
	p(h);
	opus_heap_destroy(h);
	return 0;
}
