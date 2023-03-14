/**
 * @file graph.h
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
#ifndef GRAPH_H
#define GRAPH_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <float.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

#define GRAPH_OK (0)
#define GRAPH_FAIL (1)
#define GRAPH_NO_MEM (2)
#define GRAPH_MAX_ID (UINT32_MAX)
#define GRAPH_MAX_WEIGHT (FLT_MAX)

#define GRAPH_GRID_CHAR (1)
#define GRAPH_GRID_FLOAT (2)

#define GRAPH_GET_BIT(src_char, n) (((char) (src_char) >> (n)) & 0x01)
#define GRAPH_SET_BIT0(src_char, n) ((char) (src_char) & ~(1 << (n)))
#define GRAPH_SET_BIT1(src_char, n) ((char) (src_char) | (1 << (n)))

typedef int      graph_status_t; /* error code(0 means OK) */
typedef uint32_t graph_id_t;     /* identifier of vertex, arc, etc. */
typedef uint32_t graph_count_t;
typedef void    *graph_data_t; /* pointer to the real location of the data(no specific type) */
typedef float    graph_weight_t;
typedef void    *graph_vertex_t;

typedef struct graph graph_t;

typedef graph_status_t (*graph_create_context_cb)(graph_t *graph, void **args);
typedef graph_status_t (*graph_destroy_context_cb)(graph_t *graph);

typedef graph_vertex_t *(*graph_add_vertex_cb)(graph_t *graph, graph_data_t data);
typedef graph_status_t (*graph_delete_vertex_cb)(graph_t *graph, graph_vertex_t *vertex);
typedef graph_status_t (*graph_delete_vertex_by_id_cb)(graph_t *graph, graph_id_t id);
typedef graph_id_t (*graph_get_vertex_id_cb)(graph_t *graph, graph_vertex_t *vertex);
typedef graph_vertex_t *(*graph_get_vertex_cb)(graph_t *graph, graph_id_t id);
typedef graph_count_t (*graph_get_vertex_count_cb)(graph_t *graph);

typedef graph_status_t (*graph_add_arc_cb)(graph_t *graph, graph_vertex_t *from, graph_vertex_t *to, graph_weight_t weight);
typedef graph_status_t (*graph_add_arc_by_id_cb)(graph_t *graph, graph_id_t from, graph_id_t to, graph_weight_t weight);
typedef graph_status_t (*graph_delete_arc_cb)(graph_t *graph, graph_vertex_t *from, graph_vertex_t *to);
typedef graph_status_t (*graph_delete_arc_by_id_cb)(graph_t *graph, graph_id_t from, graph_id_t to);
typedef graph_weight_t (*graph_get_arc_weight_cb)(graph_t *graph, graph_vertex_t *from, graph_vertex_t *to);
typedef graph_weight_t (*graph_get_arc_weight_by_id_cb)(graph_t *graph, graph_id_t from, graph_id_t to);
typedef graph_status_t (*graph_set_arc_weight_cb)(graph_t *graph, graph_vertex_t *from, graph_vertex_t *to, graph_weight_t weight);
typedef graph_status_t (*graph_set_arc_weight_by_id_cb)(graph_t *graph, graph_id_t from, graph_id_t to, graph_weight_t weight);
typedef graph_count_t (*graph_get_arc_count_cb)(graph_t *graph);

typedef graph_status_t (*graph_get_neighbors_cb)(graph_t *graph, graph_vertex_t *vertex, graph_id_t *res_neighbors, graph_count_t *res_count);
typedef graph_status_t (*graph_get_neighbors_by_id_cb)(graph_t *graph, graph_id_t id, graph_id_t *res_neighbors, graph_count_t *res_count);
typedef graph_status_t (*graph_get_neighbors_array_cb)(graph_t *graph, graph_vertex_t *vertex, graph_id_t **res_neighbors, graph_count_t *res_count);
typedef graph_status_t (*graph_get_neighbors_array_by_id_cb)(graph_t *graph, graph_id_t id, graph_id_t **res_neighbors, graph_count_t *res_count);

typedef graph_weight_t (*graph_get_estimated_cost_cb)(graph_t *graph, graph_vertex_t *from, graph_vertex_t *to);
typedef graph_weight_t (*graph_get_estimated_cost_by_id_cb)(graph_t *graph, graph_id_t from, graph_id_t to);

struct graph {
	void *context_;

	graph_create_context_cb  create_context_;
	graph_destroy_context_cb destroy_context_;

	graph_add_vertex_cb          add_vertex;
	graph_delete_vertex_cb       delete_vertex;
	graph_delete_vertex_by_id_cb delete_vertex_by_id;
	graph_get_vertex_id_cb       get_vertex_id;
	graph_get_vertex_cb          get_vertex;
	graph_get_vertex_count_cb    get_vertex_count;

	graph_add_arc_cb              add_arc;
	graph_add_arc_by_id_cb        add_arc_by_id;
	graph_delete_arc_cb           delete_arc;
	graph_delete_arc_by_id_cb     delete_arc_by_id;
	graph_get_arc_weight_cb       get_arc_weight;
	graph_get_arc_weight_by_id_cb get_arc_weight_by_id;
	graph_set_arc_weight_cb       set_arc_weight;
	graph_set_arc_weight_by_id_cb set_arc_weight_by_id;
	graph_get_arc_count_cb        get_arc_count;

	graph_get_neighbors_cb             get_neighbors;
	graph_get_neighbors_by_id_cb       get_neighbors_by_id;
	graph_get_neighbors_array_cb       get_neighbors_array;
	graph_get_neighbors_array_by_id_cb get_neighbors_array_by_id;

	graph_get_estimated_cost_cb       get_estimated_cost;
	graph_get_estimated_cost_by_id_cb get_estimated_cost_by_id;
};

void graph_destroy(graph_t *graph);

graph_t *graph_forward_star_create(int is_static, ...);
void     graph_forward_star_destroy(graph_t *graph);

graph_t *graph_grid_create(int mode_01, int allow_diagonal, double x, double y, double tile_w, double tile_h, graph_count_t count_w, graph_count_t count_h);
void     graph_grid_map(graph_t *graph, char **map);

double        graph_grid_get_x(graph_t *graph);
double        graph_grid_get_y(graph_t *graph);
double        graph_grid_get_tile_width(graph_t *graph);
double        graph_grid_get_tile_height(graph_t *graph);
graph_count_t graph_grid_get_width(graph_t *graph);
graph_count_t graph_grid_get_height(graph_t *graph);
void          graph_grid_print_map01(graph_t *graph);

void graph_xy2id(graph_id_t x, graph_id_t y, graph_count_t width, graph_id_t *id);
void graph_id2xy(graph_id_t id, graph_count_t width, graph_id_t *x, graph_id_t *y);

graph_id_t graph_bisect_asc_left(void *arr, size_t ele_size, void *ele, graph_id_t low, graph_id_t high, int (*cmp)(void *a, void *b));
graph_id_t graph_bisect_asc_right(void *arr, size_t ele_size, void *ele, graph_id_t low, graph_id_t high, int (*cmp)(void *a, void *b));

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* GRAPH_H */

/*******************************
 * interface implements for graph
 *******************************/
#if 0
static graph_status_t create_context(graph_t *graph, void **args);
static graph_status_t destroy_context(graph_t *graph);

static graph_vertex_t *add_vertex(graph_t *graph, graph_data_t data);
static graph_status_t  delete_vertex(graph_t *graph, graph_vertex_t *vertex);
static graph_status_t  delete_vertex_by_id(graph_t *graph, graph_id_t id);
static graph_id_t      get_vertex_id(graph_t *graph, graph_vertex_t *vertex);
static graph_vertex_t *get_vertex(graph_t *graph, graph_id_t id);
static graph_count_t   get_vertex_count(graph_t *graph);

static graph_status_t add_arc(graph_t *graph, graph_vertex_t *from, graph_vertex_t *to, graph_weight_t weight);
static graph_status_t add_arc_by_id(graph_t *graph, graph_id_t from, graph_id_t to, graph_weight_t weight);
static graph_status_t delete_arc(graph_t *graph, graph_vertex_t *from, graph_vertex_t *to);
static graph_status_t delete_arc_by_id(graph_t *graph, graph_id_t from, graph_id_t to);
static graph_weight_t get_arc_weight(graph_t *graph, graph_vertex_t *from, graph_vertex_t *to);
static graph_weight_t get_arc_weight_by_id(graph_t *graph, graph_id_t from, graph_id_t to);
static graph_status_t set_arc_weight(graph_t *graph, graph_vertex_t *from, graph_vertex_t *to, graph_weight_t weight);
static graph_status_t set_arc_weight_by_id(graph_t *graph, graph_id_t from, graph_id_t to, graph_weight_t weight);
static graph_count_t  get_arc_count(graph_t *graph);

static graph_status_t  get_neighbors(graph_t *graph, graph_vertex_t *vertex, graph_id_t *res_neighbors, graph_count_t *res_count);
static graph_status_t  get_neighbors_by_id(graph_t *graph, graph_id_t id, graph_id_t *res_neighbors, graph_count_t *res_count);
static graph_status_t  get_neighbors_array(graph_t *graph, graph_vertex_t *vertex, graph_id_t **res_neighbors, graph_count_t *res_count);
static graph_status_t  get_neighbors_array_by_id(graph_t *graph, graph_id_t id, graph_id_t **res_neighbors, graph_count_t *res_count);

static graph_weight_t get_estimated_cost(graph_t *graph, graph_vertex_t *from, graph_vertex_t *to);
static graph_weight_t get_estimated_cost_by_id(graph_t *graph, graph_id_t from, graph_id_t to);

graph_t *grid_create(int is_static, ...)
{
	static graph_t adjacency_list_template = {
	        NULL,
	        create_context, destroy_context, add_vertex, delete_vertex, delete_vertex_by_id, get_vertex_id, get_vertex, get_vertex_count,
	        add_arc, add_arc_by_id, delete_arc, delete_arc_by_id,
	        get_arc_weight, get_arc_weight_by_id, set_arc_weight, set_arc_weight_by_id, get_arc_count,
	        get_neighbors, get_neighbors_by_id, get_neighbors_array, get_neighbors_array_by_id,
			get_estimated_cost, get_estimated_cost_by_id};
	graph_t *graph;
	void    *args[3];
	va_list  va;

	if (!is_static) return NULL; /* no support for dynamic adjacency list for now */

	graph = (graph_t *) OPUS_MALLOC(sizeof(graph_t));
	if (graph) {
		graph_count_t max_vertex, max_arc;

		va_start(va, is_static);
		max_vertex = va_arg(va, graph_count_t);
		max_arc    = va_arg(va, graph_count_t);
		va_end(va);

		args[0] = &is_static;
		args[1] = &max_vertex;
		args[2] = &max_arc;

		memcpy(graph, &adjacency_list_template, sizeof(graph_t));

		/* create context */
		if (graph->create_context_(graph, args)) {
			OPUS_FREE(graph);
			return NULL;
		}
	}
	return graph;
}

void adjacency_list_destroy(graph_t *graph)
{
	if (graph) {
		graph->destroy_context_(graph);
	}
}
#endif
