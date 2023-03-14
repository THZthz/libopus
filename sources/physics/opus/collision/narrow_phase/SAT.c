/**
 * @file sat.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2023/2/28
 *
 * @example
 *
 * @development_log
 *
 */

#include <string.h>
#include <stdlib.h>
#include "physics/opus/physics.h"
#include "physics/opus/physics_private.h"
#include "math/geometry.h"

struct overlap_ {
	opus_real overlap;
	opus_vec2 axis;
};

static opus_vec2 *SAT_get_transformed_vertices_(opus_polygon *polygon,
                                                opus_mat2d    transform)
{
	size_t     i;
	opus_vec2 *vertices = malloc(sizeof(opus_vec2) * polygon->n);
	if (vertices) {
		memcpy(vertices, polygon->vertices, sizeof(opus_vec2) * polygon->n);
		for (i = 0; i < polygon->n; i++)
			opus_mar2d_pre_mul_xy(&vertices[i].x, &vertices[i].y, transform, vertices[i].x,
			                      vertices[i].y);
	}
	return vertices;
}

/**
 * @brief check all the axes of verts_b to get minimum overlap
 * @param verts_a
 * @param verts_b
 * @param na
 * @param nb
 * @return 1 if overlaps
 */
static int SAT_overlap_axes_(struct overlap_ *result, opus_vec2 *verts_a, opus_vec2 *verts_b, size_t na,
                             size_t nb)
{
	size_t    i, j;
	opus_vec2 axis;
	opus_real dot;
	opus_real overlap, overlap_ab, overlap_ba;
	opus_vec2 projection_a, projection_b; /* [min, max] */

	result->overlap = OPUS_REAL_MAX;

	for (i = 0; i < nb; i++) {
		/* get axis */
		axis = opus_vec2_sub(verts_b[i], verts_b[(i + 1) % nb]);
		axis = opus_vec2_norm(opus_vec2_perp(axis));

		/* project A's vertex to the axis */
		opus_vec2_set(&projection_a, OPUS_REAL_MAX, -OPUS_REAL_MAX);
		for (j = 0; j < na; j++) {
			dot = opus_vec2_dot(verts_a[j], axis);
			if (dot > projection_a.y) projection_a.y = dot;
			if (dot < projection_a.x) projection_a.x = dot;
		}

		/* project B's vertex to the axis */
		opus_vec2_set(&projection_b, OPUS_REAL_MAX, -OPUS_REAL_MAX);
		for (j = 0; j < nb; j++) {
			dot = opus_vec2_dot(verts_b[j], axis);
			if (dot > projection_b.y) projection_b.y = dot;
			if (dot < projection_b.x) projection_b.x = dot;
		}

		overlap_ab = projection_a.y - projection_b.x;
		overlap_ba = projection_b.y - projection_a.x;
		overlap    = opus_min(overlap_ab, overlap_ba);

		if (overlap < result->overlap) {
			result->overlap = overlap;
			result->axis    = axis;

			/* no overlap for sure, exit */
			if (overlap <= 0) return 0;
		}
	}

	return 1; /* overlap */
}

static opus_overlap_result SAT_polygon_polygon_(opus_polygon *A,
                                                opus_polygon *B,
                                                opus_mat2d transform_a, opus_mat2d transform_b)
{
	opus_overlap_result result = {0};
	struct overlap_     r, ra, rb;

	opus_vec2  c1, c2;
	opus_vec2 *verts_a = NULL, *verts_b = NULL;

	int swap_factor = 1;

	/* when using SAT, we must first translate the local vertices to world vertices */
	verts_a = SAT_get_transformed_vertices_(A, transform_a);
	verts_b = SAT_get_transformed_vertices_(B, transform_b);
	if (!verts_a || !verts_b)
		goto EXIT_AND_CLEANUP; /* no enough memory, can not proceed this algorithm */

	/* check overlapping axes */
	SAT_overlap_axes_(&rb, verts_a, verts_b, A->n, B->n);
	if (rb.overlap <= 0) goto EXIT_AND_CLEANUP;
	SAT_overlap_axes_(&ra, verts_b, verts_a, B->n, A->n);
	if (ra.overlap <= 0) goto EXIT_AND_CLEANUP;
	result.is_overlap = 1; /* has an axis, then it is overlapping */

	/* meet detector result requirements */
	/* 1st: make sure A is where reference edge lies */
	if (ra.overlap < rb.overlap) {
		r = ra;
		opus_mat2d_copy(result.transform_a, transform_a);
		opus_mat2d_copy(result.transform_b, transform_b);
		result.A = (opus_shape *) A;
		result.B = (opus_shape *) B;
	} else {
		r           = rb;
		swap_factor = -1;
		opus_mat2d_copy(result.transform_a, transform_b);
		opus_mat2d_copy(result.transform_b, transform_a);
		result.A = (opus_shape *) B;
		result.B = (opus_shape *) A;
	}
	/* 2nd: make sure normal is pointing to B */
	opus_mar2d_pre_mul_xy(&c1.x, &c1.y, transform_a, 0, 0);
	opus_mar2d_pre_mul_xy(&c2.x, &c2.y, transform_b, 0, 0);
	result.normal     = opus_vec2_dot(opus_vec2_to(c1, c2), r.axis) * swap_factor < 0 ? opus_vec2_neg(r.axis) : r.axis;
	result.separation = r.overlap;

	return result;

EXIT_AND_CLEANUP:
	if (verts_a) free(verts_a);
	if (verts_b) free(verts_b);

	return result;
}

static opus_overlap_result SAT_polygon_circle_(opus_polygon *A,
                                               opus_circle  *B,
                                               opus_mat2d transform_a, opus_mat2d transform_b)
{
	opus_overlap_result result = {0};

	opus_vec2  center_a, center_b;
	opus_vec2 *verts_a;

	size_t    i, j;
	opus_vec2 axis;
	opus_vec2 projection_a, projection_b, min_axis;
	opus_real dot, overlap, min_overlap, overlap_ab, overlap_ba;
	opus_vec2 p;

	/* when using SAT, we must first translate the local vertices to world vertices */
	verts_a = SAT_get_transformed_vertices_(A, transform_a);
	if (!verts_a) return result; /* no enough memory, can not proceed this algorithm */
	center_a = opus_mat2d_pre_mul_vec(transform_a, opus_vec2_(0, 0));
	center_b = opus_mat2d_pre_mul_vec(transform_b, opus_vec2_(0, 0));

	/* check overlapping axes */
	min_overlap = OPUS_REAL_MAX;
	for (i = 0; i < A->n; i++) {
		axis = opus_vec2_sub(verts_a[i], verts_a[(i + 1) % A->n]);
		axis = opus_vec2_norm(opus_vec2_perp(axis));

		/* project polygon A's vertex on the axis */
		opus_vec2_set(&projection_a, OPUS_REAL_MAX, -OPUS_REAL_MAX);
		for (j = 0; j < A->n; j++) {
			dot = opus_vec2_dot(verts_a[j], axis);
			if (dot > projection_a.y) projection_a.y = dot;
			if (dot < projection_a.x) projection_a.x = dot;
		}

		/* project circle B on the axis */
		p              = opus_vec2_scale(axis, B->radius);
		projection_b.y = opus_vec2_dot(opus_vec2_add(center_b, p), axis);
		projection_b.x = opus_vec2_dot(opus_vec2_sub(center_b, p), axis);
		if (projection_b.x > projection_b.y) opus_swap(&projection_b.x, &projection_b.y);

		overlap_ab = projection_a.y - projection_b.x;
		overlap_ba = projection_b.y - projection_a.x;
		overlap    = opus_min(overlap_ab, overlap_ba);

		if (overlap < min_overlap) {
			min_overlap = overlap;
			min_axis    = axis;

			/* no overlap for sure, exit */
			if (overlap <= 0) return result;
		}
	}
	result.is_overlap = 1; /* has an axis, then it is overlapping */

	/* meet detector result requirements */
	result.normal     = opus_vec2_dot(opus_vec2_to(center_a, center_b), min_axis) < 0 ? opus_vec2_inv(min_axis) : min_axis;
	result.separation = min_overlap;

	/* set basic information */
	opus_mat2d_copy(result.transform_a, transform_a);
	opus_mat2d_copy(result.transform_b, transform_b);
	result.A = (opus_shape *) A;
	result.B = (opus_shape *) B;

	return result;
}


opus_overlap_result opus_SAT(opus_shape *A, opus_shape *B, opus_mat2d transform_a,
                             opus_mat2d transform_b)
{
	opus_overlap_result r0 = {0}; /* just for invalid return */
	if (A->type_ == OPUS_SHAPE_POLYGON && B->type_ == A->type_)
		return SAT_polygon_polygon_((void *) A, (void *) B, transform_a, transform_b);
	if (A->type_ == OPUS_SHAPE_POLYGON && B->type_ == OPUS_SHAPE_CIRCLE)
		return SAT_polygon_circle_((void *) A, (void *) B, transform_a, transform_b);
	if (A->type_ == OPUS_SHAPE_CIRCLE && B->type_ == OPUS_SHAPE_POLYGON)
		return SAT_polygon_circle_((void *) B, (void *) A, transform_b, transform_a);
	return r0;
}
