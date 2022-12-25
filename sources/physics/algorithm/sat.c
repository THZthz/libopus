/**
 * @file sat.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/11/4
 *
 * @example
 *
 * @development_log
 *
 */

#include "physics/physics.h"
#include "utils/utils.h"
#include "data_structure/array.h"

struct collision_overlap {
	vec2    axis;
	count_t i;
	real    overlap;
};

vertex_t supports[2] = {0};

static INLINE int contains(vertices_t vertices, real x, real y)
{
	count_t   i;
	vertex_t *v, *nv;

	count_t n = array_len(vertices);

	v = &vertices[n - 1];
	for (i = 0; i < n; ++i) {
		nv = &vertices[i];

		if ((x - v->x) * (nv->y - v->y) + (y - v->y) * (v->x - nv->x) > 0) return 0;

		v = nv;
	}

	return 1;
}

/* get reference edge(the most perpendicular edge relative to the normal */
static INLINE vertex_t *find_supports(vertex_t supports[2], body_t *body_a, body_t *body_b, vec2 normal, real direction)
{
	real nx = normal.x * direction,
	     ny = normal.y * direction;
	count_t j;

	vertices_t vertices     = body_b->vertices;
	count_t    vertices_len = array_len(vertices);
	real       bax = body_a->position.x, bay = body_a->position.y;
	real       dist, nearest_dist            = REAL_MAX;
	vertex_t  *va, *vb, *vc;

	/*
	    support_point = body_b->vertices[collision_get_support_point(body_b->vertices, body_a->position, vec2_(-normal_x, -normal_y)).index];
	    collision_get_supporting_edge(support_point, body_b->vertices, body_a->position, vec2_(normal_x, normal_y), supports);
	*/
	for (j = 0; j < vertices_len; j++) {
		vb   = &vertices[j];
		dist = nx * (bax - vb->x) + ny * (bay - vb->y);
		if (dist < nearest_dist) {
			nearest_dist = dist;

			va = vb;
		}
	}

	vc   = &vertices[(vertices_len + va->index - 1) % vertices_len];
	dist = nx * (bax - vc->x) + ny * (bay - vc->y);

	vb = &vertices[(va->index + 1) % vertices_len];
	if (nx * (bax - vb->x) + ny * (bay - vb->y) < dist) {
		supports[0] = *va;
		supports[1] = *vb;
	} else {
		supports[0] = *va;
		supports[1] = *vc;
	}

	return supports;
}

/* check overlapped axes to find penetration */
static INLINE void overlap_axes(struct collision_overlap *res, vertices_t vertices_a, vertices_t vertices_b, axes_t axes)
{
	count_t va_len   = array_len(vertices_a),
	        vb_len   = array_len(vertices_b),
	        axes_len = array_len(axes);

	real vax = vertices_a[0].x,
	     vay = vertices_a[0].y,
	     vbx = vertices_b[0].x,
	     vby = vertices_b[0].y;

	real    overlap_min = REAL_MAX, overlap_ab, overlap_ba;
	real    overlap;
	count_t overlap_axis_idx = 0;
	count_t i, j;

	for (i = 0; i < axes_len; i++) {
		vec2 axis   = axes[i];
		real axis_x = axis.x,
		     axis_y = axis.y;
		real min_a  = vax * axis_x + vay * axis_y,
		     min_b  = vbx * axis_x + vby * axis_y,
		     max_a  = min_a,
		     max_b  = min_b;
		real dot;

		for (j = 1; j < va_len; ++j) {
			dot = vertices_a[j].x * axis_x + vertices_a[j].y * axis_y;
			if (dot > max_a) {
				max_a = dot;
			} else if (dot < min_a) {
				min_a = dot;
			}
		}

		for (j = 1; j < vb_len; ++j) {
			dot = vertices_b[j].x * axis_x + vertices_b[j].y * axis_y;
			if (dot > max_b) {
				max_b = dot;
			} else if (dot < min_b) {
				min_b = dot;
			}
		}

		overlap_ab = max_a - min_b;
		overlap_ba = max_b - min_a;
		overlap    = overlap_ab < overlap_ba ? overlap_ab : overlap_ba;

		if (overlap < overlap_min) {
			overlap_min      = overlap;
			overlap_axis_idx = i;

			if (overlap <= 0) {
				/* can not be intersecting */
				break;
			}
		}
	}

	res->axis    = axes[overlap_axis_idx];
	res->overlap = overlap_min;
	res->i       = overlap_axis_idx;
}

collision_t *collision_sat(body_t *body_a, body_t *body_b, collision_pairs_t *pairs)
{
	collision_t *c; /* collision result */

	real      dot;
	vertex_t *supports_a, *supports_b;
	int       n_supports = 0;

	struct collision_overlap overlap_ab, overlap_ba, min_overlap;

	overlap_axes(&overlap_ab, body_a->vertices, body_b->vertices, body_a->axes);
	if (overlap_ab.overlap <= 0) {
		return NULL;
	}

	overlap_axes(&overlap_ba, body_b->vertices, body_a->vertices, body_b->axes);
	if (overlap_ba.overlap <= 0) {
		return NULL;
	}

	c = collision_get_pair_record(body_a, body_b, pairs);

	body_a = c->body_a;
	body_b = c->body_b;

	if (overlap_ab.overlap < overlap_ba.overlap) {
		min_overlap = overlap_ab;
	} else {
		min_overlap = overlap_ba;
	}

	/* ensure normal is facing away from body_a */
	dot = min_overlap.axis.x * (body_b->position.x - body_a->position.x) + min_overlap.axis.y * (body_b->position.y - body_a->position.y);
	if (dot < 0) {
		c->normal.x = min_overlap.axis.x;
		c->normal.y = min_overlap.axis.y;
	} else {
		c->normal.x = -min_overlap.axis.x;
		c->normal.y = -min_overlap.axis.y;
	}

	c->tangent.x = -c->normal.y;
	c->tangent.y = c->normal.x;

	c->depth = min_overlap.overlap;

	c->penetration.x = c->normal.x * c->depth;
	c->penetration.y = c->normal.y * c->depth;

	supports_b = find_supports(supports, body_a, body_b, c->normal, 1);

	if (contains(body_a->vertices, supports_b[0].x, supports_b[0].y)) {
		c->supports[n_supports++] = supports_b[0];
	}

	if (contains(body_a->vertices, supports_b[1].x, supports_b[1].y)) {
		c->supports[n_supports++] = supports_b[1];
	}

	if (n_supports < 2) {
		supports_a = find_supports(supports, body_b, body_a, c->normal, -1);

		if (contains(body_b->vertices, supports_a[0].x, supports_a[0].y)) {
			c->supports[n_supports++] = supports_a[0];
		}

		if (n_supports < 2 && contains(body_b->vertices, supports_a[1].x, supports_a[1].y)) {
			c->supports[n_supports++] = supports_a[1];
		}
	}

	/* account for the edge case of overlapping but no vertex containment */
	if (n_supports == 0) {
		c->supports[n_supports++] = supports[0];
	}
	ASSERT(n_supports <= 2);

	/* update supports array size */
	c->n_supports = n_supports;

	return c;
}

