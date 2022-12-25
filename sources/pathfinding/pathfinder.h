/**
 * @file pathfinder.h
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
#ifndef PATHFINDER_H
#define PATHFINDER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "pathfinding/graph.h"

typedef struct pathfinder pathfinder_t;

typedef graph_status_t (*pathfinder_create_context_cb)(pathfinder_t *finder, void **args);
typedef graph_status_t (*pathfinder_destroy_context_cb)(pathfinder_t *finder);

typedef int (*pathfinder_search_cb)(pathfinder_t *finder, graph_id_t from, graph_id_t to);
typedef graph_status_t (*pathfinder_get_path_cb)(pathfinder_t *finder, graph_id_t **res_path, graph_count_t *res_count);

struct pathfinder {
	void *context_;
	graph_t *graph_;

	pathfinder_create_context_cb create_context_;
	pathfinder_destroy_context_cb destroy_context_;

	pathfinder_search_cb search;
	pathfinder_get_path_cb get_path;
};

int pathfinder_search(pathfinder_t *finder, graph_id_t from, graph_id_t to);
graph_status_t pathfinder_get_path(pathfinder_t *finder, graph_id_t **res_path, graph_count_t *res_count);
void pathfinder_destroy(pathfinder_t *finder);

pathfinder_t *pathfinder_a_star_create(graph_t *graph);
pathfinder_t *pathfinder_d_star_create(graph_t *graph);
pathfinder_t *pathfinder_d_star_lite_create(graph_t *graph, unsigned rows, unsigned cols);
pathfinder_t *pathfinder_floyd_create(graph_t *graph);

void pathfinder_d_star_mark_dirty(pathfinder_t *finder);
void pathfinder_d_star_lite_update_cost(pathfinder_t *finder, graph_id_t id, double cost);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* PATHFINDER_H */
