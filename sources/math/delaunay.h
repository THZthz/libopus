/**
 * @file delaunay.h
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/10/20
 *
 * @brief a blazing fast delaunay triangulation library
 *
 * @example A simple workflow might be:
 * 		uint32_t n;
 * 		real *coords;
 * 		delaunay_data_t data = {0};
 * 		delaunay_init(&data, coords, n);
 * 		delaunay_triangulate(&data);
 * 		delaunay_done(&data);
 *
 * To construct triangles:
 * 		Some conventions: "e" is the id of a half edge, "t" is the id of triangle.
 * 		So data.triangles[e] tells the point id where the half edge "e" starts,
 * 			data.half_edges_[e] tells the id of the opposite half edge of "e",
 * 			data.triangles[data.half_edges[e]] tells the point id where the "e" ends,
 * 			[3 * t, 3 * t + 1, 3 * t + 2] is all the id of edges belongs to triangle "t".
 *
 * To get all the edges (except hull edges):
 * 		uint32_t e;
 * 		for (e = 0; e < data.n_triangles_; e++) {
 * 			if (e > data.half_edges_[e]) {
 * 				vec2 p = paras.points[data.triangles_[e]];
 * 				vec2 q = paras.points[data.triangles_[delaunay_next_half_edge(e)]];
 *
 * 				... do something with edge starting point "p" and ending point "q" ...
 *
 * 			}
 * 		}
 *
 * To get the adjacent triangles:
 * 		Because "data.half_edges_[e]" is the opposite half edge of "e",
 * 			and "floor(e / 3)" tells you which triangle the half edge belongs to, you can easily find all
 * 			the adjacent triangles.
 *
 * @development_log
 *
 */
#ifndef DELAUNAY_H
#define DELAUNAY_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include "math/math.h"

#define EPSILON 2E-52

typedef struct delaunay_data delaunay_data_t;

struct delaunay_data {
	real    *coords;
	uint32_t n;

	uint32_t  n_hull_;
	uint32_t  hull_start_;
	uint32_t *hull_;

	uint32_t  n_triangles_, n_half_edges_;
	uint32_t *triangles_;
	uint32_t  *half_edges_;

	uint32_t  hash_size_;
	uint32_t *hull_prev_; /* edge to prev edge */
	uint32_t *hull_next_; /* edge to next edge */
	uint32_t *hull_tri_;  /* edge to adjacent triangle */
	uint32_t  *hull_hash_; /* angular edge hash */

	uint32_t *ids_; /* temporary arrays for sorting points */
	real     *dists_;
};

void delaunay_triangulate(delaunay_data_t *data);
void delaunay_done(delaunay_data_t *data);
void delaunay_init(delaunay_data_t *data, real *coords, uint32_t n);

#define delaunay_edge(tri_id, i) (3 * (tri_id) + (i))
#define delaunay_triangle_of_edge(edge_id) core_floor((edge_id) / 3)
#define delaunay_next_half_edge(edge_id) (((edge_id) % 3 == 2) ? (edge_id) - 2 : (edge_id) + 1)
#define delaunay_prev_half_edge(edge_id) (((edge_id) % 3 == 0) ? (edge_id) + 2 : (edge_id) - 1)

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* DELAUNAY_H */
