/**
 * @file graph.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/12/9
 *
 * @example
 *
 * @development_log
 *
 */

#include <stddef.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "pathfinding/graph.h"

void graph_xy2id(graph_id_t x, graph_id_t y, graph_count_t width, graph_id_t *id)
{
	*id = y * width + x;
}

void graph_id2xy(graph_id_t id, graph_count_t width, graph_id_t *x, graph_id_t *y)
{
	*y = id / width;
	*x = id - width * (*y);
}

void graph_destroy(graph_t *graph)
{
	if (graph) {
		graph->destroy_context_(graph);
	}
}

/* how many elements is lesser than the element given */
graph_id_t graph_bisect_asc_left(void *arr, size_t ele_size, void *ele, graph_id_t low, graph_id_t high, int (*cmp)(void *a, void *b))
{
	high++; /* we use [low, high] as the searching scope */
	while (low < high) {
		graph_id_t mid = low + (high - low) / 2;
		if (cmp((char *) arr + mid * ele_size, ele) < 0) low = mid + 1;
		else high = mid;
	}
	return low;
}

/* how many elements is not bigger than the element given */
graph_id_t graph_bisect_asc_right(void *arr, size_t ele_size, void *ele, graph_id_t low, graph_id_t high, int (*cmp)(void *a, void *b))
{
	high++; /* we use [low, high] as the searching scope */
	while (low < high) {
		graph_id_t mid = low + (high - low) / 2;
		if (cmp((char *) arr + mid * ele_size, ele) > 0)high = mid;
		else  low = mid + 1;
	}
	return low;
}

