/**
 * @file forward_star.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/12/8
 *
 * @brief
 *
 * @example
 *
 */

#include <stdlib.h>
#include <string.h>

#include "pathfinding/graph.h"
#include "utils/utils.h"

typedef struct context_f context_t;
typedef struct arc         arc_t;

struct arc {
	graph_weight_t weight;
	graph_id_t     next, to;
};

struct context_f {
	graph_count_t head_count, arc_count;
	graph_count_t max_head, max_arc;

	void      **vertex_data;
	graph_id_t *heads;
	arc_t      *arcs;
};

static graph_status_t create_context(graph_t *graph, void **args)
{
	if (graph->context_ == NULL) {
		context_t *context   = (context_t *) OPUS_MALLOC(sizeof(context_t));
		int        is_static = *(int *) args[0];

		if (!is_static) return GRAPH_FAIL;

		if (context) {
			graph->context_     = context;
			context->head_count = 0;
			context->arc_count  = 0;
			context->max_head   = *(graph_count_t *) args[1];
			context->max_arc    = *(graph_count_t *) args[2];

			context->heads       = (graph_id_t *) OPUS_MALLOC(context->max_head * sizeof(graph_id_t));
			context->arcs        = (arc_t *) malloc(context->max_arc * sizeof(arc_t));
			context->vertex_data = (void **) malloc(context->max_head * sizeof(void *));

			/* if we fail to allocate any memory, clean and exit */
			if (!context->heads || !context->arcs || !context->vertex_data) {
				if (context->heads) OPUS_FREE(context->heads);
				if (context->arcs) OPUS_FREE(context->arcs);
				if (context->vertex_data) OPUS_FREE(context->vertex_data);
				OPUS_FREE(context);
				graph->context_ = NULL;
				return GRAPH_NO_MEM;
			}
		}

		memset(context->heads, -1, context->max_head * sizeof(graph_id_t));
		memset(context->arcs, 0, context->max_arc * sizeof(arc_t));
	} else {
		graph->destroy_context_(graph);
		graph->create_context_(graph, args);
	}

	return GRAPH_OK;
}

static graph_status_t destroy_context(graph_t *graph)
{
	context_t *context = graph->context_;
	if (context) {
		graph->context_ = NULL;
		OPUS_FREE(context->vertex_data);
		OPUS_FREE(context->heads);
		OPUS_FREE(context->arcs);
		OPUS_FREE(context);
	}
	return GRAPH_OK;
}

static graph_vertex_t *add_vertex(graph_t *graph, graph_data_t data)
{
	context_t *context                        = graph->context_;

	/* check if we have enough memory */
	if (context->head_count >= context->max_head) return NULL;

	context->vertex_data[context->head_count] = data;
	context->heads[context->head_count]       = GRAPH_MAX_ID;
	context->head_count++;

	return &context->vertex_data[context->head_count - 1];
}

static graph_id_t get_vertex_id(graph_t *graph, graph_vertex_t *vertex)
{
	context_t *context = graph->context_;
	return (graph_id_t) (vertex - context->vertex_data);
}

static graph_vertex_t *get_vertex(graph_t *graph, graph_id_t id)
{
	context_t *context = graph->context_;
	return &context->vertex_data[id];
}

static graph_count_t get_vertex_count(graph_t *graph)
{
	context_t *context = graph->context_;
	return context->head_count;
}

static graph_status_t add_arc_by_id(graph_t *graph, graph_id_t from, graph_id_t to, graph_weight_t weight)
{
	context_t *context = graph->context_;

	/* bound check */
	if (context->arc_count >= context->max_arc) return GRAPH_FAIL;

	context->arcs[context->arc_count].weight = weight;
	context->arcs[context->arc_count].to     = to;
	context->arcs[context->arc_count].next   = context->heads[from];

	context->heads[from] = context->arc_count++;

	return GRAPH_OK;
}

static graph_status_t add_arc(graph_t *graph, graph_vertex_t *from, graph_vertex_t *to, graph_weight_t weight)
{
	return add_arc_by_id(graph, get_vertex_id(graph, from), get_vertex_id(graph, to), weight);
}

static graph_status_t get_neighbors_by_id(graph_t *graph, graph_id_t id, graph_id_t *res_neighbors, graph_count_t *res_count)
{
	context_t *context = graph->context_;
	graph_id_t next    = context->heads[id];

	*res_count = 0;
	while (next != GRAPH_MAX_ID) {
		arc_t *arc                  = &context->arcs[next];
		res_neighbors[(*res_count)++] = arc->to;
		next                        = arc->next;
	}

	return GRAPH_OK;
}

static graph_status_t get_neighbors(graph_t *graph, graph_vertex_t *vertex, graph_id_t *res_neighbors, graph_count_t *res_count)
{
	return get_neighbors_by_id(graph, get_vertex_id(graph, vertex), res_neighbors, res_count);
}

static graph_status_t get_neighbors_array_by_id(graph_t *graph, graph_id_t id, graph_id_t **res_neighbors, graph_count_t *res_count)
{
	context_t    *context = graph->context_;
	graph_id_t    next    = context->heads[id];
	graph_count_t cap     = 10;

	*res_count     = 0;
	*res_neighbors = (graph_id_t *) malloc(cap * sizeof(graph_id_t));
	if (*res_neighbors == NULL) return GRAPH_NO_MEM;
	while (next != GRAPH_MAX_ID) {
		arc_t *arc = &context->arcs[next];
		if (*res_count >= cap) {
			*res_neighbors = (graph_id_t *) OPUS_REALLOC(*res_neighbors, cap * 2 * sizeof(graph_id_t));
			cap *= 2;
			if (*res_neighbors == NULL) return GRAPH_NO_MEM;
		}
		(*res_neighbors)[(*res_count)++] = arc->to;

		next                         = arc->next;
	}

	return GRAPH_OK;
}

static graph_status_t get_neighbors_array(graph_t *graph, graph_vertex_t *vertex, graph_id_t **res_neighbors, graph_count_t *res_count)
{
	return get_neighbors_array_by_id(graph, get_vertex_id(graph, vertex), res_neighbors, res_count);
}

static graph_weight_t get_arc_weight_by_id(graph_t *graph, graph_id_t from, graph_id_t to)
{
	context_t *context = graph->context_;
	graph_id_t next    = context->heads[from];

	while (next != GRAPH_MAX_ID) {
		arc_t *arc = &context->arcs[next];
		if (arc->to == to) return arc->weight;
		next = arc->next;
	}

	return GRAPH_MAX_WEIGHT;
}

static graph_weight_t get_arc_weight(graph_t *graph, graph_vertex_t *from, graph_vertex_t *to)
{
	return get_arc_weight_by_id(graph, get_vertex_id(graph, from), get_vertex_id(graph, to));
}

static graph_status_t set_arc_weight_by_id(graph_t *graph, graph_id_t from, graph_id_t to, graph_weight_t weight)
{
	context_t *context = graph->context_;
	graph_id_t next    = context->heads[from];

	while (next != GRAPH_MAX_ID) {
		arc_t *arc = &context->arcs[next];
		if (arc->to == to) {
			arc->weight = weight;
			return GRAPH_OK;
		}
		next = arc->next;
	}

	return GRAPH_FAIL;
}

static graph_status_t set_arc_weight(graph_t *graph, graph_vertex_t *from, graph_vertex_t *to, graph_weight_t weight)
{
	return set_arc_weight_by_id(graph, get_vertex_id(graph, from), get_vertex_id(graph, to), weight);
}

/**
 * @brief
 * 		int is_static, graph_count_t max_vertex, graph_count_t max_arc
 * @param is_static
 * @param ...
 * @return
 */
graph_t *graph_forward_star_create(int is_static, ...)
{
	static graph_t te = {
	        NULL,
	        create_context, destroy_context, add_vertex, NULL, NULL, get_vertex_id, get_vertex, get_vertex_count,
	        add_arc, add_arc_by_id, NULL, NULL,
	        get_arc_weight, get_arc_weight_by_id, set_arc_weight, set_arc_weight_by_id,
	        NULL,
	        get_neighbors, get_neighbors_by_id, get_neighbors_array, get_neighbors_array_by_id,
			NULL, NULL};
	graph_t *graph;
	void    *args[3];
	va_list  va;

	if (!is_static) return NULL; /* no support for dynamic adjacency list for now */

	graph = (graph_t *) malloc(sizeof(graph_t));
	if (graph) {
		graph_count_t max_vertex, max_arc;

		va_start(va, is_static);
		max_vertex = va_arg(va, graph_count_t);
		max_arc    = va_arg(va, graph_count_t);
		va_end(va);

		args[0] = &is_static;
		args[1] = &max_vertex;
		args[2] = &max_arc;

		memcpy(graph, &te, sizeof(graph_t));

		/* create context */
		if (graph->create_context_(graph, args)) {
			OPUS_FREE(graph);
			return NULL;
		}
	}
	return graph;
}

void graph_forward_star_destroy(graph_t *graph)
{
	graph_destroy(graph);
}
