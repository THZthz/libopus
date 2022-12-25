/**
 * @file a_star_finder.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data_structure/heap.h"
#include "pathfinding/graph.h"
#include "pathfinding/pathfinder.h"
#include "utils/utils.h"

enum {
	NODE_NEW      = 0,
	NODE_IS_OPEN  = 1,
	NODE_IS_CLOSE = 2
};

typedef struct context_a_star context_t;
typedef struct a_star_node    a_star_node_t;

struct a_star_node {
	int            state_;
	graph_weight_t f, g;

	struct a_star_node *parent; /* used to backtrace path */
};

struct context_a_star {
	graph_id_t start, end; /* the target of current search */

	graph_id_t *open_heap_data_;
	heap_t      open_heap_;

	graph_count_t  n_nodes_;
	a_star_node_t *nodes_;
};

static graph_id_t get_node_id(context_t *context, a_star_node_t *node)
{
	return node - context->nodes_;
}

static a_star_node_t *get_node(context_t *context, graph_id_t id)
{
	return &context->nodes_[id];
}

static graph_id_t *open_heap_push(context_t *context, graph_id_t id)
{
	return heap_push(&context->open_heap_, &id);
}

static graph_id_t *open_heap_pop(context_t *context)
{
	return heap_pop(&context->open_heap_);
}

static graph_id_t *open_heap_remove(context_t *context, size_t id)
{
	return heap_remove(&context->open_heap_, id);
}

static int open_heap_is_empty(context_t *context)
{
	return context->open_heap_.n_used == 0;
}

static int compare_f(context_t *context, graph_id_t a, graph_id_t b)
{
	graph_weight_t o = get_node(context, a)->f - get_node(context, b)->f;
	return o > 0 ? 1 : (o < 0 ? -1 : 0);
}

static int compare(heap_t *heap, void *a, void *b)
{
	return compare_f(heap->context_, *(graph_id_t *) a, *(graph_id_t *) b);
}

static graph_status_t create_context(pathfinder_t *finder, void **args)
{
	context_t *context = finder->context_;

	/* we must have a graph to base our finder's context on */
	if (!finder->graph_) return GRAPH_FAIL;

	if (LIKELY(!context)) {
		context = (context_t *) malloc(sizeof(context_t));

		if (UNLIKELY(!context)) return GRAPH_NO_MEM;

		finder->context_  = context;
		context->start    = GRAPH_MAX_ID;
		context->end      = GRAPH_MAX_ID;
		context->n_nodes_ = finder->graph_->get_vertex_count(finder->graph_);
		context->nodes_   = (a_star_node_t *) malloc(context->n_nodes_ * sizeof(a_star_node_t));

		if (UNLIKELY(!context->nodes_)) {
			finder->context_ = NULL;
			free(context);
			return GRAPH_NO_MEM;
		}

		context->open_heap_data_ = (graph_id_t *) malloc(sizeof(graph_id_t) * context->n_nodes_);
		if (!context->open_heap_data_) {
			finder->context_ = NULL;
			free(context->nodes_);
			free(context);
			return GRAPH_NO_MEM;
		}

		context->open_heap_.data_    = context->open_heap_data_;
		context->open_heap_.context_ = context;
		context->open_heap_.capacity = context->n_nodes_;
		context->open_heap_.ele_size = sizeof(graph_id_t);
		context->open_heap_.n_used   = 0;
		context->open_heap_.compare_ = compare;
	}

	return GRAPH_OK;
}

static graph_status_t destroy_context(pathfinder_t *finder)
{
	if (finder && finder->context_) {
		context_t *context = finder->context_;
		free(context->nodes_);
		free(context);
	}

	return GRAPH_OK;
}

static int search(pathfinder_t *finder, graph_id_t from, graph_id_t to)
{
	graph_t   *graph   = finder->graph_;
	context_t *context = finder->context_;

	context->start = from;
	context->end   = to;

	/* clear nodes' state */
	memset(context->nodes_, 0, context->n_nodes_ * sizeof(a_star_node_t));

	/* clear open heap */
	context->open_heap_.n_used = 0;

	get_node(context, from)->state_ = NODE_IS_OPEN;
	open_heap_push(context, from);

	while (!open_heap_is_empty(context)) {
		graph_id_t     cur;
		a_star_node_t *cur_node;
		graph_id_t    *neighbors   = NULL;
		graph_count_t  n_neighbors = 0, i;

		/* get the node with minimum F */
		cur      = *open_heap_pop(context);
		cur_node = get_node(context, cur);

		cur_node->state_ = NODE_IS_CLOSE;

		if (cur == to) return 1;

		/* loop through all the neighbors of current node */
		graph->get_neighbors_array_by_id(graph, cur, &neighbors, &n_neighbors);
		for (i = 0; i < n_neighbors; i++) {
			graph_id_t     neighbor      = neighbors[i];
			a_star_node_t *neighbor_node = get_node(context, neighbor);
			graph_weight_t g_c2n; /* pass cost from cur_node to neighbor_node */

			/* ignore neighbor vertex which is closed (already visited by main "while" loop) */
			if (neighbor_node->state_ == NODE_IS_CLOSE) continue;

			g_c2n = graph->get_arc_weight_by_id(graph, cur, neighbor);

			if (neighbor_node->state_ == NODE_IS_OPEN) {
				/* if the neighbor is already in the open heap */
				/* check if there exists a better path to neighbor_node */
				if (g_c2n + cur_node->g < neighbor_node->g) {
					graph_id_t     c, *heap = context->open_heap_data_;
					graph_weight_t h = neighbor_node->f - neighbor_node->g;

					/* remove it from open heap */
					for (c = 0; c < context->open_heap_.n_used; c++) {
						if (heap[c] == neighbor) break;
					}
					open_heap_remove(context, c);

					/* update its properties */
					neighbor_node->parent = cur_node;
					neighbor_node->g      = g_c2n + cur_node->g;
					neighbor_node->f      = neighbor_node->g + h;

					/* re-insert it */
					open_heap_push(context, neighbor);
				}
			} else {
				graph_weight_t g_e = graph->get_estimated_cost_by_id(graph, neighbor, to);

				/* the neighbor is brand new! */
				neighbor_node->state_ = NODE_IS_OPEN;
				neighbor_node->parent = cur_node;
				neighbor_node->g      = g_c2n + cur_node->g;
				neighbor_node->f      = neighbor_node->g + g_e;

				/* insert to open heap */
				open_heap_push(context, neighbor);
			}
		}
		if (neighbors != NULL) free(neighbors);
	}

	return 0; /* we could not find the path */
}

static graph_status_t get_path(pathfinder_t *finder, graph_id_t **res_path, graph_count_t *res_count)
{
	graph_id_t    cur, p_id;
	graph_count_t capacity = 10;
	context_t    *context  = finder->context_;

	*res_path  = (graph_id_t *) malloc(capacity * sizeof(graph_id_t));
	*res_count = 0;

	if (UNLIKELY(!(*res_path))) {
		*res_path = NULL;
		return GRAPH_NO_MEM;
	}

	cur  = context->end;
	p_id = GRAPH_MAX_ID;
	for (;;) {
		a_star_node_t *p = get_node(context, cur)->parent;
		p_id             = get_node_id(context, p);

		if (!p) break;
		if (p_id == context->start) break;

		if (*res_count == capacity) {
			capacity *= 2;
			*res_path = (graph_id_t *) realloc(*res_path, capacity * sizeof(graph_id_t));
			if (!(*res_path)) return GRAPH_NO_MEM;
		}
		(*res_path)[(*res_count)++] = p_id;

		cur = p_id;
	}

	return GRAPH_OK;
}

pathfinder_t *pathfinder_a_star_create(graph_t *graph)
{
	pathfinder_t te = {
	        NULL,
	        NULL,
	        create_context,
	        destroy_context,
	        search,
	        get_path};
	pathfinder_t *finder = (pathfinder_t *) malloc(sizeof(pathfinder_t));

	if (!finder) return NULL;

	memcpy(finder, &te, sizeof(pathfinder_t));
	finder->context_ = NULL;
	finder->graph_   = graph;

	if (finder->create_context_(finder, NULL)) {
		free(finder);
		return NULL;
	}

	return finder;
}

void pathfinder_destroy(pathfinder_t *finder)
{
	if (finder && finder->context_) {
		finder->destroy_context_(finder);
		free(finder);
	}
}

int pathfinder_search(pathfinder_t *finder, graph_id_t from, graph_id_t to)
{
	return finder->search(finder, from, to);
}

graph_status_t pathfinder_get_path(pathfinder_t *finder, graph_id_t **res_path, graph_count_t *res_count)
{
	return finder->get_path(finder, res_path, res_count);
}
