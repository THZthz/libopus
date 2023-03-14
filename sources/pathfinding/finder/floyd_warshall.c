/**
 * @file floyd_warshall.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/12/10
 *
 * @example
 *
 * @development_log
 *
 */

#include <stdlib.h>
#include <string.h>

#include "pathfinding/graph.h"
#include "pathfinding/pathfinder.h"
#include "utils/utils.h"


typedef struct context_floyd context_t;

struct context_floyd {
	graph_count_t   size; /* vertex count */
	graph_id_t     *path; /* max_id_ + 1 */
	graph_weight_t *cost;

	graph_id_t from, to;
};

static void init_matrix(context_t *context, graph_t *graph)
{
	uint32_t i, j;
	uint32_t size = context->size;

	/*
	 * initialize cost matrix
	 * the meaning of cost[i][j]:
	 *      weight from i to j (have direction)
	 */
	for (i = 0; i < size; i++) {
		for (j = 0; j < size; j++) {
			context->cost[i * size + j] = graph->get_arc_weight_by_id(graph, i, j);
		}
	}

	/* initialize path */
	for (i = 0; i < size; i++)
		for (j = 0; j < size; j++)
			context->path[i * size + j] = j;
}

static graph_status_t create_context(pathfinder_t *finder, void **args)
{
	context_t *context = finder->context_;

	if (!finder->graph_) return GRAPH_FAIL;

	if (!context) {
		context = (context_t *) OPUS_MALLOC(sizeof(context_t));

		if (!context) return GRAPH_NO_MEM;

		context->size = finder->graph_->get_vertex_count(finder->graph_);
		context->cost = (graph_weight_t *) OPUS_MALLOC(sizeof(graph_weight_t) * context->size * context->size);
		context->path = (uint32_t *) OPUS_MALLOC(sizeof(uint32_t) * context->size * context->size);

		if (!context->cost || !context->path) {
			finder->context_ = NULL;
			if (context->cost) OPUS_FREE(context->cost);
			if (context->path) OPUS_FREE(context->path);
			OPUS_FREE(context);

			return GRAPH_NO_MEM;
		}

		context->from = GRAPH_MAX_ID;
		context->to   = GRAPH_MAX_ID;

		init_matrix(context, finder->graph_);
	}

	return GRAPH_OK;
}

static graph_status_t destroy_context(pathfinder_t *finder)
{
	if (finder && finder->context_) {
		context_t *context = finder->context_;
		finder->context_   = NULL;
		OPUS_FREE(context->path);
		OPUS_FREE(context->cost);
		OPUS_FREE(context);
	}

	return GRAPH_OK;
}

static int search(pathfinder_t *finder, graph_id_t from, graph_id_t to)
{
	uint32_t        i, j, k;
	graph_count_t   size;
	graph_id_t     *path;
	graph_weight_t *cost;

	context_t *context = finder->context_;

	context->from = from;
	context->to   = to;

	/* perform floyd algorithm */
	cost = context->cost;
	path = context->path;
	size = context->size;
	for (k = 0; k < size; k++)
		for (i = 0; i < size; i++)
			for (j = 0; j < size; j++) {
				if (cost[i * size + k] != GRAPH_MAX_WEIGHT && cost[k * size + j] != GRAPH_MAX_WEIGHT) {
					if (cost[i * size + j] > cost[i * size + k] + cost[k * size + j]) {
						cost[i * size + j] = cost[i * size + k] + cost[k * size + j];
						path[i * size + j] = k;
					}
				}
			}

	return 1;
}

static graph_status_t get_path(pathfinder_t *finder, graph_id_t **res_path, graph_count_t *res_count)
{
#define CHECK_SPACE()                                                                 \
	if (*res_count == capacity) {                                                     \
		capacity *= 2;                                                                \
		*res_path = (graph_id_t *) OPUS_REALLOC(*res_path, capacity * sizeof(graph_id_t)); \
                                                                                      \
		if (!(*res_path)) return GRAPH_NO_MEM;                                        \
	}

	context_t *context = finder->context_;

	graph_id_t   i = context->from, j = context->to, k;
	graph_count_t size = context->size, capacity = 10;

	*res_count = 0;
	*res_path  = (graph_id_t *) OPUS_MALLOC(capacity * sizeof(graph_id_t));
	if (!(*res_path)) return GRAPH_NO_MEM;
	while (context->path[i * size + j] != j) {
		/* allocate enough space for path */
		CHECK_SPACE();

		(*res_path)[(*res_count)++] = j;

		k = context->path[i * size + j];
		j = k;
	}
	CHECK_SPACE();
	(*res_path)[(*res_count)++] = k;

	/* reverse the array */
	for (i = 0, j = *res_count - 1; i < j; i++, j--) {
		graph_id_t temp;

		temp           = (*res_path)[i];
		(*res_path)[i] = (*res_path)[j];
		(*res_path)[j] = temp;
	}

	return GRAPH_OK;
}

pathfinder_t *pathfinder_floyd_create(graph_t *graph)
{
	pathfinder_t te = {
	        NULL,
	        NULL,
	        create_context,
	        destroy_context,
	        search,
	        get_path};
	pathfinder_t *finder = (pathfinder_t *) OPUS_MALLOC(sizeof(pathfinder_t));

	if (!finder) return NULL;

	memcpy(finder, &te, sizeof(pathfinder_t));
	finder->context_ = NULL;
	finder->graph_   = graph;

	if (finder->create_context_(finder, NULL)) {
		OPUS_FREE(finder);
		return NULL;
	}

	return finder;
}

