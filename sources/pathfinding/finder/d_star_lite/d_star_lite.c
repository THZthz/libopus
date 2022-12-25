/**
 * @file d_star_lite.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/12/13
 *
 * @brief d* lite algorithm variant for path planning on partially-known terrain
 * @reference https://github.com/Sollimann/Dstar-lite-pathplanner
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data_structure/array.h"
#include "data_structure/hashmap.h"
#include "data_structure/heap.h"
#include "pathfinding/finder/d_star_lite/map.h"
#include "pathfinding/graph.h"
#include "pathfinding/pathfinder.h"
#include "pathfinding/utils/utils.h"

typedef struct context context_t;

struct context {
	graph_t *graph;

	int is_inited_; /* if d_star_lite has initialized */

	map_t     map;
	cell_t  **path; /* array */
	heap_t    open_heap;
	unsigned *open_heap_data_;
	cell_t   *start;
	cell_t   *goal;
	cell_t   *last;
	unsigned  start_id, goal_id;
	double    k_m; /* accumulated heuristic value */
};

static void cell_(cell_t *cell)
{
	if (cell->is_open) return;

	cell->is_open     = 1;
	cell->priority.k1 = CELL_PRIORITY_MAX;
	cell->priority.k2 = CELL_PRIORITY_MAX;
}

static unsigned get_cell_id(context_t *context, cell_t *cell)
{
	return context->map.cols * cell->y + cell->x;
}

static cell_t *get_cell_by_id(context_t *context, unsigned id)
{
	graph_id_t x, y;
	graph_id2xy(id, (graph_count_t) context->map.cols, &x, &y);
	return &(context->map.cells[y][x]);
}

/*  set priority of a cell and insert it to open heap */
static void heap_insert_(context_t *context, cell_t *cell, double k1, double k2)
{
	unsigned id;
	id = get_cell_id(context, cell);

	cell->priority.k1 = k1;
	cell->priority.k2 = k2;
	cell->is_open     = 1;

	heap_push2(&context->open_heap, &id);
}

/* remove cell from open heap */
static void heap_remove_(context_t *context, cell_t *cell)
{
	unsigned i;
	for (i = 0; i < context->open_heap.n_used; i++) {
		unsigned id = context->open_heap_data_[i];
		if (get_cell_by_id(context, id) == cell) {
			heap_remove2(&context->open_heap, i);
			break;
		}
	}

	cell->is_open = 0;
}

/* update priority of a cell, remove it and re-insert it to open heap */
static void heap_update_(context_t *context, cell_t *cell, double k1, double k2)
{
	heap_remove_(context, cell);
	heap_insert_(context, cell, k1, k2);
}

static double rhs_(context_t *context, cell_t *cell, double new_rhs)
{
	if (cell == context->goal) return 0.0;

	cell_(cell);

	if (new_rhs != CELL_PRIORITY_MIN) {
		cell->priority.k2 = new_rhs;
	}

	return cell->priority.k2;
}

static double g_(context_t *context, cell_t *cell, double new_g)
{
	cell_(cell);

	if (new_g != CELL_PRIORITY_MIN) {
		cell->priority.k1 = new_g;
	}

	return cell->priority.k1;
}

static double h_(cell_t *a, cell_t *b)
{
	unsigned min, max;

	min = a->x > b->x ? a->x - b->x : b->x - a->x;
	max = a->y > b->y ? a->y - b->y : b->y - a->y;

	if (min > max) {
		unsigned tmp;
		tmp = min;
		min = max;
		max = tmp;
	}

	return 0.41421 * min + max;
}

static priority_t k_(context_t *context, cell_t *cell)
{
	priority_t p;
	double     g   = cell->priority.k1;
	double     rhs = cell->priority.k2;
	double     min = (g < rhs) ? g : rhs;
	p.k1           = min + h_(context->start, cell) + context->k_m;
	p.k2           = min;
	return p;
}

static double cost_(cell_t *a, cell_t *b)
{
	unsigned dx, dy;
	double   scale = 1;

	if (a->cost == CELL_UNWALKABLE || b->cost == CELL_UNWALKABLE)
		return CELL_UNWALKABLE;

	dx = a->x > b->x ? a->x - b->x : b->x - a->x;
	dy = a->y > b->y ? a->y - b->y : b->y - a->y;

	if ((dx + dy) > 1) {
		scale = 1.41421; /* sqrt(2) */
	}

	return scale * ((a->cost + b->cost) / 2);
}

static void min_successor_(context_t *context, cell_t *cell, cell_t **res_min_cell, double *res_min_cost)
{
	unsigned i;

	cell_t **neighbors = cell->neighbors;
	double   tmp_cost, tmp_g;

	cell_t *min_cell = NULL;
	double  min_cost = CELL_PRIORITY_MAX;

	for (i = 0; i < MAX_NEIGHBORS; i++) {
		if (neighbors[i] != NULL) {
			tmp_cost = cost_(cell, neighbors[i]);
			tmp_g    = neighbors[i]->priority.k1;

			if (tmp_cost == CELL_PRIORITY_MAX || tmp_g == CELL_PRIORITY_MAX)
				continue;

			tmp_cost += tmp_g;

			if (tmp_cost < min_cost) {
				min_cell = neighbors[i];
				min_cost = tmp_cost;
			}
		}
	}

	*res_min_cell = min_cell;
	*res_min_cost = min_cost;
}

static void update_cell_(context_t *context, cell_t *cell)
{
	int diff   = cell->priority.k1 != cell->priority.k2;
	int exists = cell->is_open;

	if (diff && exists) {
		priority_t k = k_(context, cell);
		heap_update_(context, cell, k.k1, k.k2);
	} else if (diff && !exists) {
		priority_t k = k_(context, cell);
		heap_insert_(context, cell, k.k1, k.k2);
	} else if (!diff && exists) {
		heap_remove_(context, cell);
	}
}

static int is_open_heap_empty(context_t *context)
{
	return context->open_heap.n_used == 0;
}

/* return 1 if priority "A" is lesser than priority "B" */
static int compare_priority(priority_t *a, priority_t *b)
{
	if (a->k1 < b->k1 || (a->k1 == b->k1 && a->k2 < b->k2))
		return -1;
	else if (a->k1 == b->k1 && a->k2 == b->k2)
		return 0;
	else
		return 1;
}

/* used to sort priority in open heap */
static int cmp(heap_t *heap, void *a, void *b)
{
	priority_t *pa = a;
	priority_t *pb = b;
	return compare_priority(pa, pb);
}

/* compute the shortest path */
static int compute_(context_t *context)
{
	unsigned attempts = 0;
	unsigned max_attempts = UINT_MAX / 2;

	cell_t    *u, **neighbors;
	priority_t k_old, k_new;
	double     g_old, tmp_g, tmp_rhs;

	cell_t *start = context->start;

	if (is_open_heap_empty(context)) return 0;

	for (;;) {
		if (attempts++ >= max_attempts) {
			printf("exceeds maximum number of attempts\n");
			return 0;
		}

		printf("%u, %u\n", u->x, u->y);

		u = get_cell_by_id(context, *(unsigned *) heap_top(&context->open_heap));
		if ((!is_open_heap_empty(context) && compare_priority(&u->priority, &start->priority)) ||
		    start->priority.k1 != start->priority.k2) {
			k_old = u->priority;
			k_new = k_(context, u);

			tmp_g   = u->priority.k1;
			tmp_rhs = u->priority.k2;

			if (compare_priority(&k_old, &k_new)) {
				heap_update_(context, u, k_new.k1, k_new.k2);
			} else if (tmp_g > tmp_rhs) {
				unsigned i;
				cell_t  *neighbor;

				g_(context, u, tmp_rhs);
				tmp_g = tmp_rhs;

				heap_remove_(context, u);

				neighbors = u->neighbors;
				for (i = 0; i < MAX_NEIGHBORS; i++) {
					neighbor = neighbors[i];
					if (neighbor == NULL) continue;

					if (neighbor != context->goal) {
						double rhs_a   = neighbor->priority.k2;
						double rhs_b   = cost_(neighbor, u) + tmp_g;
						double new_rhs = rhs_a < rhs_b ? rhs_a : rhs_b;
						rhs_(context, neighbor, new_rhs);
					}

					update_cell_(context, neighbor);
				}
			} else {
				unsigned i;

				g_old = tmp_g;

				if (u != context->goal) {
					double  new_rhs;
					cell_t *successor;
					min_successor_(context, u, &successor, &new_rhs);
					rhs_(context, u, new_rhs);
				}

				update_cell_(context, u);

				neighbors = u->neighbors;
				for (i = 0; i < MAX_NEIGHBORS; i++) {
					cell_t *neighbor = neighbors[i];
					if (!neighbor) continue;

					if (neighbor != context->goal) {
						if (neighbor->priority.k2 == cost_(neighbor, u) + g_old) {
							cell_t *successor;
							double  rhs;
							min_successor_(context, u, &successor, &rhs);
							rhs_(context, neighbor, rhs);
						}
					}

					update_cell_(context, neighbor);
				}
			}
		} else
			break;
	}

	return 1;
}

static void update_(context_t *context, cell_t *u, double cost)
{
	unsigned i;

	double cost_old, cost_new;
	double tmp_cost_old, tmp_cost_new;
	double tmp_rhs, tmp_g;

	cell_t **neighbors;

	if (u == context->goal) return;

	context->k_m += h_(context->last, context->start);
	context->last = context->start;

	cell_(u);

	cost_old = u->cost;
	cost_new = cost;
	u->cost  = cost;

	neighbors = u->neighbors;
	for (i = 0; i < MAX_NEIGHBORS; i++) {
		cell_t *neighbor = neighbors[i];
		if (!neighbor) continue;

		u->cost      = cost_old;
		tmp_cost_old = cost_(u, neighbor);
		u->cost      = cost_new;
		tmp_cost_new = cost_(u, neighbor);

		tmp_g   = neighbor->priority.k1;
		tmp_rhs = u->priority.k2;

		if (u != context->goal) {
			if (tmp_cost_old > tmp_cost_new) {
				double rhs = tmp_rhs < tmp_cost_new + tmp_g ? tmp_rhs : tmp_cost_new + tmp_g;
				rhs_(context, u, rhs);
			} else if (tmp_rhs == tmp_cost_old + tmp_g) {
				cell_t *successor;
				double  rhs;
				min_successor_(context, u, &successor, &rhs);
				rhs_(context, u, rhs);
			}
		}
	}

	update_cell_(context, u);

	neighbors = u->neighbors;
	for (i = 0; i < MAX_NEIGHBORS; i++) {
		cell_t *neighbor = neighbors[i];
		if (!neighbor) continue;

		u->cost      = cost_old;
		tmp_cost_old = cost_(u, neighbor);
		u->cost      = cost_new;
		tmp_cost_new = cost_(u, neighbor);

		tmp_g   = u->priority.k1;
		tmp_rhs = neighbor->priority.k2;

		if (u != context->goal) {
			if (tmp_cost_old > tmp_cost_new) {
				double rhs = tmp_rhs < tmp_cost_new + tmp_g ? tmp_rhs : tmp_cost_new + tmp_g;
				rhs_(context, neighbor, rhs);
			} else if (tmp_rhs == tmp_cost_old + tmp_g) {
				cell_t *successor;
				double  rhs;
				min_successor_(context, neighbor, &successor, &rhs);
				rhs_(context, neighbor, rhs);
			}
		}

		update_cell_(context, neighbor);
	}
}

static int re_plan_(context_t *context)
{
	cell_t *cur;
	int     res;

	array_clear(context->path);

	if (!(res = compute_(context))) return 0;

	cur = context->start;
	array_push(context->path, &cur);

	while (cur != context->goal) {
		cell_t *s;
		double  r;
		if (cur == NULL || cur->priority.k1 == CELL_PRIORITY_MAX) return 0;

		min_successor_(context, cur, &s, &r);
		cur = s;

		array_push(context->path, &cur);
	}

	return res;
}

static graph_status_t create_context(pathfinder_t *finder, void **args)
{
	context_t *context = finder->context_;

	if (!context) {
		unsigned rows, cols;

		rows = *(unsigned *) args[0];
		cols = *(unsigned *) args[1];

		context = (context_t *) malloc(sizeof(context_t));
		if (!context) return GRAPH_NO_MEM;

		finder->context_ = context;

		/* initialize map */
		if (!map_init(&context->map, rows, cols)) {
			free(context);
			return GRAPH_NO_MEM;
		}

		array_create(context->path, sizeof(cell_t *));
		array_create(context->open_heap_data_, sizeof(unsigned));
		context->open_heap.data_    = context->open_heap_data_;
		context->open_heap.ele_size = sizeof(unsigned);
		context->open_heap.capacity = rows * cols;
		context->open_heap.n_used   = 0;
		context->open_heap.compare_ = cmp;

		context->start = NULL;
		context->goal  = NULL;
		context->last  = NULL;

		context->k_m = 0;

		context->is_inited_ = 0;
	}

	return GRAPH_OK;
}

static graph_status_t destroy_context(pathfinder_t *finder)
{
	if (finder && finder->context_) {
		context_t *context = finder->context_;

		array_destroy(context->path);
		array_destroy(context->open_heap_data_);

		map_done(&context->map);

		free(context);
	}

	return GRAPH_OK;
}

static void init_(context_t *context, unsigned from, unsigned to)
{
	context->open_heap.n_used = 0;
	array_clear(context->path);

	context->k_m   = 0;
	context->start = get_cell_by_id(context, from);
	context->goal  = get_cell_by_id(context, to);
	context->last  = context->start;

	rhs_(context, context->goal, 0);

	heap_insert_(context, context->goal, h_(context->start, context->goal), 0);
}

static int search(pathfinder_t *finder, graph_id_t from, graph_id_t to)
{
	context_t *context = finder->context_;

	if (!context) return 0;

	if (!context->is_inited_) {
	initialize_context:
		init_(context, from, to);
		context->is_inited_ = 1;
		context->start_id   = from;
		context->goal_id    = to;
	}

	/* if the context has already been initialized but the start and goal position is not matched */
	if (from != context->start_id || to != context->goal_id) goto initialize_context;

	return re_plan_(context);
}

static graph_status_t get_path(pathfinder_t *finder, graph_id_t **res_path, graph_count_t *res_count)
{
	context_t *context;
	unsigned i;
	context    = finder->context_;
	*res_count = 0;
	*res_path  = (graph_id_t *) malloc(sizeof(graph_id_t) * array_len(context->path));
	if (!(*res_path)) return GRAPH_NO_MEM;
	*res_count = array_len(context->path);
	for (i = 0; i < array_len(context->path); i++) (*res_path)[i] = get_cell_id(context, context->path[i]);
	return GRAPH_OK;
}

pathfinder_t *pathfinder_d_star_lite_create(graph_t *graph, unsigned rows, unsigned cols)
{
	pathfinder_t te = {
	        NULL,
	        NULL,
	        create_context,
	        destroy_context,
	        search,
	        get_path};
	pathfinder_t *finder = (pathfinder_t *) malloc(sizeof(pathfinder_t));

	void *args[2];

	if (!finder) return NULL;

	memcpy(finder, &te, sizeof(pathfinder_t));
	finder->context_ = NULL;
	finder->graph_   = graph;

	args[0] = &rows;
	args[1] = &cols;

	if (finder->create_context_(finder, args)) {
		free(finder);
		return NULL;
	}

	return finder;
}

void pathfinder_d_star_lite_update_cost(pathfinder_t *finder, graph_id_t id, double cost)
{
	context_t *context = finder->context_;
	update_(context, get_cell_by_id(context, id), cost);
}
