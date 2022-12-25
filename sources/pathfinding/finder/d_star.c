/**
 * @file d_star.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/12/11
 *
 * @example
 *
 * @reference
 * 		https://zhuanlan.zhihu.com/p/366462473
 * 		https://blog.csdn.net/lqzdreamer/article/details/85055569
 *		https://github.com/mdeyo/d-star-lite
 *
 *		https://github.com/dittmar/d_star
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pathfinding/graph.h"
#include "pathfinding/pathfinder.h"
#include "utils/utils.h"
#include "data_structure/heap.h"

typedef struct context_d   context_t;
typedef struct d_star_node d_star_node_t;

enum {
	NODE_NEW,
	NODE_IS_OPEN,
	NODE_IS_CLOSE
};

struct d_star_node {
	int state_;

	graph_weight_t h, k;

	struct d_star_node *parent;
};

struct context_d {
	graph_t *graph;

	int map_changed_;

	graph_id_t from, to;

	graph_count_t  n_nodes_;
	d_star_node_t *nodes_;

	graph_id_t       *open_heap_data_;
	heap_t open_heap_;
};

static INLINE graph_id_t get_node_id(context_t *context, d_star_node_t *node)
{
	return node - context->nodes_;
}

static INLINE d_star_node_t *get_node(context_t *context, graph_id_t id)
{
	return &context->nodes_[id];
}

/* get the node with the minimum of K */
static graph_id_t get_min_state(context_t *context)
{
	if (context->open_heap_.n_used == 0) return GRAPH_MAX_ID;
	return context->open_heap_data_[0];
}

static graph_weight_t get_k_min(context_t *context)
{
	if (context->open_heap_.n_used == 0) return -1;
	return get_node(context, context->open_heap_data_[0])->k;
}

static void open_heap_insert(context_t *context, graph_id_t id, graph_weight_t h_new)
{
	d_star_node_t *node = get_node(context, id);

	if (node->state_ == NODE_NEW) {
		node->k = h_new;
	} else if (node->state_ == NODE_IS_OPEN) {
		node->k = node->k < h_new ? node->k : h_new;
	} else if (node->state_ == NODE_IS_CLOSE) {
		node->k = node->h < h_new ? node->h : h_new;
	}
	node->h      = h_new;
	node->state_ = NODE_IS_OPEN;
	heap_push(&context->open_heap_, &id);
}

static void open_heap_pop(context_t *context)
{
	d_star_node_t *node = get_node(context, 0);
	if (node->state_ == NODE_IS_OPEN) node->state_ = NODE_IS_CLOSE;
	heap_pop(&context->open_heap_);
}

static int compare_k(context_t *context, graph_id_t a, graph_id_t b)
{
	graph_weight_t o = get_node(context, a)->k - get_node(context, b)->k;
	return o > 0 ? 1 : (o < 0 ? -1 : 0);
}

static int compare(heap_t *heap, void *a, void *b)
{
	return compare_k(heap->context_, *(graph_id_t *) a, *(graph_id_t *) b);
}

static graph_weight_t get_cost(context_t *context, graph_id_t from, graph_id_t to)
{
	return context->graph->get_arc_weight_by_id(context->graph, from, to);
}

static void get_neighbors(context_t *context, graph_id_t id, graph_id_t **neighbors, graph_count_t *n)
{
	context->graph->get_neighbors_array_by_id(context->graph, id, neighbors, n);
}

static graph_weight_t process_state(context_t *context)
{
	graph_id_t     cur = get_min_state(context);
	graph_weight_t k_old;
	d_star_node_t *x;
	graph_id_t    *neighbors = NULL;
	graph_count_t  i, n_neighbors = 0;
	if (cur == GRAPH_MAX_ID) return -1;

	x     = get_node(context, cur);
	k_old = x->k;
	open_heap_pop(context);

	get_neighbors(context, cur, &neighbors, &n_neighbors);
	if (k_old < x->h) {
		for (i = 0; i < n_neighbors; i++) {
			d_star_node_t *y = get_node(context, neighbors[i]);
			graph_weight_t h = y->h + get_cost(context, cur, neighbors[i]);
			if (y->h <= k_old && x->h > h) {
				x->parent = y;
				x->h      = h;
			}
		}
	}
	if (k_old == x->h) {
		for (i = 0; i < n_neighbors; i++) {
			d_star_node_t *y = get_node(context, neighbors[i]);
			graph_weight_t h = x->h + get_cost(context, neighbors[i], cur);
			if (y->state_ == NODE_NEW ||
			    (y->parent == x && y->h != h) ||
			    (y->parent != x && y->h > h)) {
				y->parent = x;
				open_heap_insert(context, neighbors[i], h);
			}
		}
	} else {
		for (i = 0; i < n_neighbors; i++) {
			d_star_node_t *y = get_node(context, neighbors[i]);
			graph_weight_t h = x->h + get_cost(context, neighbors[i], cur);
			if (y->state_ == NODE_NEW || (y->parent == x && y->h != h)) {
				y->parent = x;
				open_heap_insert(context, neighbors[i], h);
			} else {
				if (y->parent != x && y->h > h) {
					open_heap_insert(context, cur, x->h);
				} else if (y->parent != x &&
				           x->h > y->h + get_cost(context, cur, neighbors[i]) &&
				           y->state_ == NODE_IS_CLOSE &&
				           y->h > k_old) {
					open_heap_insert(context, neighbors[i], y->h);
				}
			}
		}
	}
	if (neighbors) free(neighbors);

	cur = get_min_state(context);
	if (cur == GRAPH_MAX_ID) return -1;
	return get_node(context, cur)->k;
}

static graph_weight_t modify_cost(context_t *context, graph_id_t x, graph_id_t y)
{
	d_star_node_t *x_node = get_node(context, x);
	open_heap_insert(context, y, GRAPH_MAX_WEIGHT);

	if (x_node->state_ == NODE_IS_CLOSE) {
		/* X just got its path blocked, so put it on the OPEN heap */
		open_heap_insert(context, x, x_node->h);
	}
	return get_k_min(context);
}

static graph_status_t create_context(pathfinder_t *finder, void **args)
{
	context_t *context = finder->context_;

	/* we must have a graph to base our finder's context on */
	if (!finder->graph_) return GRAPH_FAIL;

	if (LIKELY(!context)) {
		context = (context_t *) malloc(sizeof(context_t));

		if (UNLIKELY(!context)) return GRAPH_NO_MEM;

		finder->context_      = context;
		context->graph        = finder->graph_;
		context->map_changed_ = 0;
		context->from         = GRAPH_MAX_ID;
		context->to           = GRAPH_MAX_ID;
		context->n_nodes_     = finder->graph_->get_vertex_count(finder->graph_);
		context->nodes_       = (d_star_node_t *) malloc(sizeof(d_star_node_t) * context->n_nodes_);

		if (!context->nodes_) {
			finder->context_ = NULL;
			free(context);
			return GRAPH_NO_MEM;
		}

		/* initialize nodes' state */
		memset(context->nodes_, 0, context->n_nodes_ * sizeof(d_star_node_t));

		context->open_heap_data_ = (graph_id_t *) malloc(sizeof(graph_id_t));

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

		free(context);
	}

	return GRAPH_OK;
}

static int search(pathfinder_t *finder, graph_id_t from, graph_id_t to)
{
	context_t *context = finder->context_;

	d_star_node_t *start_node = get_node(context, from);

	/* we need to re-plan the route */
	if (context->map_changed_) {
		if (from == context->from && to == context->to) {
			d_star_node_t *cur_node = start_node;
			d_star_node_t *end_node = get_node(context, to);

			graph_id_t cur_id = get_node_id(context, cur_node);

			while (cur_node != end_node) {
				graph_id_t p_id = get_node_id(context, cur_node->parent);
				if (get_cost(context, p_id, 0) == 1) {
					cur_node = cur_node->parent;
				} else {
					/* we find that our path is blocked */
					graph_weight_t k_min = modify_cost(context, cur_id, p_id);
					while (k_min != -1 && k_min < cur_node->parent->h && start_node->state_ != NODE_IS_CLOSE) {
						k_min = process_state(context);
					}
				}
			}

			context->map_changed_ = 0;

			return 1; /* re-planning completed */
		} else {
			/* invalid operation */
			return 0;
		}
	}

	context->from = from;
	context->to   = to;

	open_heap_insert(context, to, 0);
	for (;;) {
		if (process_state(context) == -1) break;        /* no possible path exists if we get -1 returned */
		if (start_node->state_ == NODE_IS_CLOSE) break; /* we have found a path */
	}

	return 1;
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

	cur  = context->from;
	p_id = GRAPH_MAX_ID;
	for (;;) {
		d_star_node_t *p = get_node(context, cur)->parent;
		p_id             = get_node_id(context, p);

		if (!p) break;
		if (p_id == context->to) break;

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

pathfinder_t *pathfinder_d_star_create(graph_t *graph)
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

/* let d star know something changed in the map to let it re-plan the route */
void pathfinder_d_star_mark_dirty(pathfinder_t *finder)
{
	context_t *context    = finder->context_;
	context->map_changed_ = 1;
}
