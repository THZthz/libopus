/**
 * @file grid.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/12/8
 *
 * @example
 *
 * @development_log
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pathfinding/graph.h"

#define TILE_(data, x, y, w) ((char *) (data) + (y) * (w) + (x))

typedef struct context_grid context_t;

struct context_grid {
	double        x, y;             /* the top-left coordinate of the grid */
	double        tile_w, tile_h;   /* measurement of each tile in this grid */
	graph_count_t count_w, count_h; /* counts of tiles in horizontal and vertical direction */

	int is_01_map_;
	int allow_diagonal_;

	void **data_;
	void  *weights;
};

/* up right down left top-left top-right down-right down-left */
static int neighbor_offset[8][2] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}, {-1, -1}, {1, -1}, {1, 1}, {-1, 1}};

static graph_status_t create_context(graph_t *graph, void **args)
{
	int    mode_01        = *(int *) args[0];
	int    allow_diagonal = *(int *) args[1];
	double x              = *(double *) args[2];
	double y              = *(double *) args[3];
	double tile_w         = *(double *) args[4];
	double tile_h         = *(double *) args[5];

	graph_count_t count_w = *(graph_count_t *) args[6];
	graph_count_t count_h = *(graph_count_t *) args[7];

	context_t *context;

	if (graph->context_) {
		return GRAPH_OK;
	}

	context = (context_t *) malloc(sizeof(context_t));
	if (context) {
		graph->context_  = context;
		context->x       = x;
		context->y       = y;
		context->tile_w  = tile_w;
		context->tile_h  = tile_h;
		context->count_w = count_w;
		context->count_h = count_h;

		context->is_01_map_      = mode_01;
		context->allow_diagonal_ = allow_diagonal;

		/* TODO: we need more accurate weights ? */
		/*if (mode_01) {*/
		context->weights = (char *) malloc(count_w * count_h * sizeof(char));
		/*} else {
		    context->weights = (graph_weight_t *) malloc(count_w * count_h * sizeof(graph_weight_t));
		}*/

		if (!context->weights) {
			graph->context_ = NULL;
			free(context);
			return GRAPH_NO_MEM;
		}

		return GRAPH_OK;
	}

	return GRAPH_NO_MEM;
}

static graph_status_t destroy_context(graph_t *graph)
{
	if (graph->context_) {
		context_t *context = graph->context_;

		free(context->weights);
		free(context);

		graph->context_ = NULL;
	}

	return GRAPH_OK;
}

static graph_id_t get_vertex_id(graph_t *graph, graph_vertex_t *vertex)
{
	context_t *context = graph->context_;
	return (graph_id_t) (vertex - context->data_);
}

static graph_vertex_t *get_vertex(graph_t *graph, graph_id_t id)
{
	context_t *context = graph->context_;
	return &context->data_[id];
}

static graph_count_t get_vertex_count(graph_t *graph)
{
	context_t *context = graph->context_;
	return context->count_w * context->count_h;
}

static graph_weight_t get_arc_weight_by_id(graph_t *graph, graph_id_t from, graph_id_t to)
{
	context_t *context = graph->context_;
	return ((char *) context->weights)[from] == 0 ? 1 : GRAPH_MAX_WEIGHT;
}

static graph_weight_t get_arc_weight(graph_t *graph, graph_vertex_t *from, graph_vertex_t *to)
{
	return get_arc_weight_by_id(graph, get_vertex_id(graph, from), 0);
}

static graph_status_t set_arc_weight_by_id(graph_t *graph, graph_id_t from, graph_id_t to, graph_weight_t weight)
{
	context_t *context = graph->context_;
	char      *tile    = (char *) context->weights + from;
	*tile              = (char) (int) weight;
	return GRAPH_OK;
}

static graph_status_t set_arc_weight(graph_t *graph, graph_vertex_t *from, graph_vertex_t *to, graph_weight_t weight)
{
	return set_arc_weight_by_id(graph, get_vertex_id(graph, from), 0, weight);
}

static graph_status_t get_neighbors_by_id(graph_t *graph, graph_id_t id, graph_id_t *res_neighbors, graph_count_t *res_count)
{
	context_t    *context = graph->context_;
	graph_count_t all     = context->allow_diagonal_ ? 8 : 4;
	graph_count_t i;
	graph_id_t    x, y;
	int           ox, oy;

	*res_count = 0;
	graph_id2xy(id, context->count_w, &x, &y);
	for (i = 0; i < all; i++) {
		graph_id_t nx, ny, n_id;

		ox = neighbor_offset[i][0];
		oy = neighbor_offset[i][1];

		if (ox < 0 && x == 0) continue;
		if (oy < 0 && y == 0) continue;
		nx = x + ox;
		ny = y + oy;
		if (nx >= context->count_w) continue;
		if (ny >= context->count_h) continue;

		/* ignore un-accessible tile */
		if (context->is_01_map_ && ((char *) context->weights)[ny * context->count_w + nx] == 1) continue;

		graph_xy2id(nx, ny, context->count_w, &n_id);
		res_neighbors[(*res_count)++] = n_id;
	}

	return GRAPH_OK;
}

static graph_status_t get_neighbors(graph_t *graph, graph_vertex_t *vertex, graph_id_t *res_neighbors, graph_count_t *res_count)
{
	return get_neighbors_by_id(graph, get_vertex_id(graph, vertex), res_neighbors, res_count);
}

static graph_status_t get_neighbors_array_by_id(graph_t *graph, graph_id_t id, graph_id_t **res_neighbors, graph_count_t *res_count)
{
	context_t *context = graph->context_;
	*res_neighbors     = (graph_id_t *) malloc(sizeof(graph_id_t) * (context->allow_diagonal_ ? 8 : 4));
	return get_neighbors_by_id(graph, id, *res_neighbors, res_count);
}

static graph_status_t get_neighbors_array(graph_t *graph, graph_vertex_t *vertex, graph_id_t **res_neighbors, graph_count_t *res_count)
{
	return get_neighbors_array_by_id(graph, get_vertex_id(graph, vertex), res_neighbors, res_count);
}

static graph_weight_t get_estimated_cost_by_id(graph_t *graph, graph_id_t from, graph_id_t to)
{
	graph_id_t sx, sy, ex, ey;
	context_t *context = graph->context_;
	graph_id2xy(from, context->count_w, &sx, &sy);
	graph_id2xy(to, context->count_w, &ex, &ey);

	/* use Manhattan Heuristic */
	if (sx > ex) sx = sx - ex;
	else
		sx = ex - sx;
	if (sy > ey) sy = sy - ey;
	else
		sy = ey - sy;
	return (graph_weight_t) (sx + sy);
}

static graph_weight_t get_estimated_cost(graph_t *graph, graph_vertex_t *from, graph_vertex_t *to)
{
	return get_estimated_cost_by_id(graph, get_vertex_id(graph, from), get_vertex_id(graph, to));
}

graph_t *graph_grid_create(int mode_01, int allow_diagonal, double x, double y, double tile_w, double tile_h, graph_count_t count_w, graph_count_t count_h)
{
	static graph_t grid_template = {
	        NULL,
	        create_context, destroy_context, NULL, NULL, NULL, get_vertex_id, get_vertex, get_vertex_count,
	        NULL, NULL, NULL, NULL,
	        get_arc_weight, get_arc_weight_by_id, set_arc_weight, set_arc_weight_by_id, NULL,
	        get_neighbors, get_neighbors_by_id, get_neighbors_array, get_neighbors_array_by_id,
	        get_estimated_cost, get_estimated_cost_by_id};
	graph_t *graph;

	graph = (graph_t *) malloc(sizeof(graph_t));
	if (graph) {
		void *args[8];
		args[0] = &mode_01;
		args[1] = &allow_diagonal;
		args[2] = &x;
		args[3] = &y;
		args[4] = &tile_w;
		args[5] = &tile_h;
		args[6] = &count_w;
		args[7] = &count_h;

		memcpy(graph, &grid_template, sizeof(graph_t));

		/* create context */
		if (graph->create_context_(graph, args)) {
			free(graph);
			return NULL;
		}
	}
	return graph;
}

void graph_grid_map(graph_t *graph, char **map)
{
	context_t *context = graph->context_;
	memcpy(context->weights, map, context->count_w * context->count_h * sizeof(char));
}

void graph_grid_print_map01(graph_t *graph)
{
	graph_id_t x, y;
	context_t *context = graph->context_;
	for (y = 0; y < context->count_h; y++) {
		for (x = 0; x < context->count_w; x++) {
			char c = *TILE_(context->weights, x, y, context->count_w);
			printf("%c ", c ? '#' : '.');
		}
		printf("\n");
	}
}

double graph_grid_get_x(graph_t *graph)
{
	context_t *context = graph->context_;
	return context->x;
}

double graph_grid_get_y(graph_t *graph)
{
	context_t *context = graph->context_;
	return context->y;
}

double graph_grid_get_tile_width(graph_t *graph)
{
	context_t *context = graph->context_;
	return context->tile_w;
}

double graph_grid_get_tile_height(graph_t *graph)
{
	context_t *context = graph->context_;
	return context->tile_h;
}

graph_count_t graph_grid_get_width(graph_t *graph)
{
	context_t *context = graph->context_;
	return context->count_w;
}

graph_count_t graph_grid_get_height(graph_t *graph)
{
	context_t *context = graph->context_;
	return context->count_h;
}
