/**
 * @file utils.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/12/31
 *
 * @brief backend for basic operations on polygons
 *
 * @example
 *
 * @development_log
 *
 */

#include "utils/utils.h"
#include "data_structure/array.h"
#include "math/polygon/polygon.h"

static opus_vec2 center__;
static ptrdiff_t coord_offset__ = 0;
static ptrdiff_t ele_offset__   = sizeof(opus_vec2);

static OPUS_INLINE void *vec_p_n(void *vertices, size_t i)
{
	return (char *) vertices + i * ele_offset__;
}

static OPUS_INLINE opus_vec2 *vec_p(const void *vp) { return (opus_vec2 *) ((char *) vp + coord_offset__); }

static OPUS_INLINE opus_vec2 *vec_n(void *vertices, size_t i) { return vec_p(vec_p_n(vertices, i)); }

static OPUS_INLINE opus_vec2 *vec_next(opus_vec2 *v)
{
	return (opus_vec2 *) ((char *) v - coord_offset__ + ele_offset__);
}

static OPUS_INLINE opus_vec2 *vec_prev(opus_vec2 *v)
{
	return (opus_vec2 *) ((char *) v - coord_offset__ - ele_offset__);
}

void opus_set_polygon_offset(ptrdiff_t coord_offset, ptrdiff_t ele_offset)
{
	coord_offset__ = coord_offset;
	ele_offset__   = ele_offset;
}

/**
 * @see https://en.wikipedia.org/wiki/Centroid#Centroid_of_a_polygon
 */
void opus_center_(void *vertices, size_t n, void *out_center)
{
	size_t i;
	opus_vec2 *v1, *v2, *v_end;
	opus_real  cx = 0, cy = 0, ar = 0, a;
	i     = 0;
	v_end = vec_n(vertices, n);
	for (v1 = vec_p(vertices); v1 < v_end; v1 = vec_next(v1)) {
		v2 = vec_n(vertices, (i + 1) % n);
		a  = opus_vec2_cross(*v1, *v2);
		cx += a * (v1->x + v2->x);
		cy += a * (v1->y + v2->y);
		ar += a;
		i++;
	}
	ar *= 3;
	opus_vec2_set(vec_p(out_center), cx / ar, cy / ar);
}

/**
 * @return the area of a polygon (convex)
 */
opus_real opus_area_(void *vertices, size_t n, int is_signed)
{
	opus_real  area = 0;
	opus_vec2 *vi, *v_end = vec_n(vertices, n), *vj = vec_prev(v_end);

	for (vi = vertices; vi < v_end; vi = vec_next(vi)) {
		area += (vj->x - vi->x) * (vj->y + vi->y);
		vj = vi;
	}

	if (is_signed) return area / 2;

	return opus_abs(area) / 2;
}

/* get the average center of the set of vertices */
void opus_mean_(void *vertices, size_t n, void *out_mean)
{
	opus_real  d = 1 / (opus_real) n;
	opus_vec2 *vi, *v_end = vec_n(vertices, n), s = {0, 0};

	for (vi = vertices; vi < v_end; vi = vec_next(vi)) s = opus_vec2_add(s, *vi);

	opus_vec2_set(vec_p(out_mean), s.x * d, s.y * d);
}

/* the moment of inertia (second moment of area) of the set of vertices given the total mass */
opus_real opus_inertia_(void *vertices, size_t n, opus_real mass)
{
	opus_real  cross, numerator = 0, denominator = 0;
	size_t i; /* index of vi */
	opus_vec2 *vi, *vj, *v_end = vec_n(vertices, n);

	/*
	 * find the polygon's moment of inertia, using second moment of area
	 * from equations at http://www.physicsforums.com/showthread.php?t=25293
	 */
	for (vi = vertices, i = 0; vi < v_end; (vi = vec_next(vi)), i++) {
		vj    = vec_n(vertices, (i + 1) % n);
		cross = opus_vec2_cross(*vj, *vi);
		cross = opus_abs(cross);
		numerator += cross * (opus_vec2_dot(*vj, *vj) + opus_vec2_dot(*vj, *vi) + opus_vec2_dot(*vi, *vi));
		denominator += cross;
	}

	return (mass / 6) * (numerator / denominator);
}

/* translate vertices in place */
void opus_translate_(void *vertices, size_t n, opus_vec2 vec, opus_vec2 scalar)
{
	size_t i;
	opus_vec2 *v, t;
	t.x = vec.x * scalar.x;
	t.y = vec.y * scalar.y;

	for (i = 0; i < n; i++) {
		v  = vec_n(vertices, i);
		*v = opus_vec2_add(*v, t);
	}
}

/* rotate the set of vertices around a point with a certain angle */
void opus_rotate_(void *vertices, size_t n, opus_vec2 point, opus_real angle)
{
	opus_real c, s, dx, dy;
	size_t i;

	if (angle == 0) return; /* avoid extra calculation */
	c = opus_cos(angle);
	s = opus_sin(angle);
	for (i = 0; i < n; i++) {
		opus_vec2 *v;
		v    = vec_n(vertices, i);
		dx   = v->x - point.x;
		dy   = v->y - point.y;
		v->x = point.x + (dx * c - dy * s);
		v->y = point.y + (dx * s + dy * c);
	}
}

/* scale the set of vertices in place */
void opus_scale_(void *vertices, size_t n, opus_vec2 origin, opus_vec2 scalar)
{
	size_t i;
	opus_vec2 delta, *v;

	if (scalar.x == 1 && scalar.y == 1) return;

	for (i = 0; i < n; i++) {
		v     = vec_n(vertices, i);
		delta = opus_vec2_sub(*v, origin);
		v->x  = origin.x + delta.x * scalar.x;
		v->y  = origin.y + delta.y * scalar.y;
	}
}

/* test if a set of vertices contains a point */
int opus_contains_(void *vertices, size_t n, opus_vec2 point)
{
	size_t i;
	opus_vec2 *v = vec_n(vertices, n - 1), *v_next;

	for (i = 0; i < n; i++) {
		v_next = vec_n(vertices, i);

		if ((point.x - v->x) * (v_next->y - v->y) + (point.y - v->y) * (v->x - v_next->x) > 0) {
			return 0;
		}

		v = v_next;
	}

	return 1;
}

static int sort_CW(const void *a, const void *b)
{
	opus_vec2 *va = vec_p(a);
	opus_vec2 *vb = vec_p(b);
	return opus_sign(opus_vec2_angle(center__, *va) - opus_vec2_angle(center__, *vb));
}

static int sort_CCW(const void *a, const void *b) { return -sort_CW(a, b); }

/* sort the set of vertices in place */
void *opus_make_cw_(void *vertices, size_t n)
{
	opus_center_(vertices, n, &center__);
	qsort(vertices, n, ele_offset__, sort_CW);
	return vertices;
}

void *opus_make_ccw_(void *vertices, size_t n)
{
	opus_center_(vertices, n, &center__);
	qsort(vertices, n, ele_offset__, sort_CCW);
	return vertices;
}

int opus_is_convex_(void *vertices, size_t n)
{
	/* http://paulbourke.net/geometry/polygonmesh/ */
	/* Copyright (c) Paul Bourke (use not permitted, but ...) */

	int    flag = 0;
	size_t i;
	opus_real  z;
	opus_vec2 *vi;

	if (n < 3) return 0;

	for (i = 0, vi = (opus_vec2 *) vertices; i < n;) {
		opus_vec2 *vj = vec_n(vertices, (i + 1) % n), *vk = vec_n(vertices, (i + 2) % n);
		z = (vj->x - vi->x) * (vk->y - vj->y);
		z -= (vj->y - vi->y) * (vk->x - vj->x);

		if (z < 0) flag |= 1;
		else if (z > 0)
			flag |= 2;

		if (flag == 3) return 0;

		i++;
		vi = vec_next(vi);
	}

	if (flag != 0) {
		return 1;
	} else {
		return 0;
	}
}

static int vertices_sort_x_axis(const void *a, const void *b)
{
	opus_vec2 *pa = vec_p(a);
	opus_vec2 *pb = vec_p(b);
	opus_real  dx = pa->x - pb->x;
	dx       = dx != 0 ? dx : pa->y - pb->y;
	return opus_sign(dx);
}

/**
 * @brief get the convex hull of the input vertices as a new array of points
 * @param vertices
 * @return remember to release its memory by using "array_destroy"
 */
opus_vec2 *opus_contour_(void *vertices, size_t n)
{
	/* algorithm from http://geomalgorithms.com/a10-_hull-1.html */

	int64_t i;
	opus_vec2   *v;
	opus_vec2   *vertices_copy, *upper, *lower;
	opus_arr_create(vertices_copy, sizeof(opus_vec2));
	opus_arr_create(upper, sizeof(opus_vec2));
	opus_arr_create(lower, sizeof(opus_vec2));

	/* sort vertices on x-axis (y-axis for ties) */
	opus_arr_resize(vertices_copy, n);
	for (i = 0; i < n; i++) {
		v = vec_n(vertices, i);
		opus_vec2_copy(&vertices_copy[i], *v);
	}
	qsort(vertices_copy, n, sizeof(opus_vec2), vertices_sort_x_axis);

	/* build lower hull */
	for (i = 0; i < n; i += 1) {
		v = &vertices_copy[i];

		while (opus_arr_len(lower) >= 2 &&
		       opus_vec2_cross3(lower[opus_arr_len(lower) - 2], lower[opus_arr_len(lower) - 1], *v) <= 0)
			opus_arr_pop(lower);

		opus_arr_push(lower, v);
	}

	/* build upper hull */
	for (i = (int64_t) n - 1; i >= 0; i -= 1) {
		v = &vertices_copy[i];

		while (opus_arr_len(upper) >= 2 &&
		       opus_vec2_cross3(upper[opus_arr_len(upper) - 2], upper[opus_arr_len(upper) - 1], *v) <= 0)
			opus_arr_pop(upper);

		opus_arr_push(upper, v);
	}

	/* concatenation of the lower and upper hulls gives the convex hull */
	/* omit last points because they are repeated at the beginning of the other list */
	opus_arr_pop(lower);
	opus_arr_pop(upper);
	opus_arr_concat(upper, lower);
	opus_arr_destroy(lower);
	opus_arr_destroy(vertices_copy);

	return upper;
}

/*
 * You can specify the 3 following options to - 1 to set it to default value
 * quality defaults to -1,
 * quality_min defaults to 2,
 * quality_max defaults to 14 */
opus_vec2 *opus_chamfer_(void *vertices, size_t n, const opus_real *radius, size_t n_radius,
                            opus_real quality, opus_real quality_min, opus_real quality_max)
{
	opus_vec2 *new_vertices;
	size_t i, j;

	if (quality_min == -1) quality_min = 2;
	if (quality_max == -1) quality_max = 14;

	opus_arr_create(new_vertices, sizeof(opus_vec2));
	for (i = 0; i < n; i++) {
		opus_real  current_radius = radius[i < n_radius ? i : n_radius - 1];
		opus_real  diagonal_radius;
		opus_real  precision;
		opus_real  alpha, theta;
		opus_vec2 *prev_vertex = vec_n(vertices, i - 1 >= 0 ? i - 1 : n - 1),
		     *vertex = vec_n(vertices, i), *next_vertex = vec_n(vertices, (i + 1) % n);
		opus_vec2 prev_normal, next_normal;
		opus_vec2 radius_vector, mid_normal, scaled_vertex;

		if (current_radius == 0) {
			opus_arr_push(new_vertices, vertex);
			continue;
		}

		opus_vec2_set(&prev_normal, vertex->y - prev_vertex->y, prev_vertex->x - vertex->x);
		prev_normal = opus_vec2_norm(prev_normal);

		opus_vec2_set(&next_normal, next_vertex->y - vertex->y, vertex->x - next_vertex->x);
		next_normal = opus_vec2_norm(next_normal);

		diagonal_radius = opus_sqrt(2 * opus_pow(current_radius, 2));
		opus_vec2_set(&radius_vector, prev_normal.x * current_radius,
		              prev_normal.y * current_radius);
		opus_vec2_set(&mid_normal, (prev_normal.x + next_normal.x) / 2,
		              (prev_normal.y + next_normal.y) / 2);
		mid_normal    = opus_vec2_norm(mid_normal);
		scaled_vertex = opus_vec2_scale(mid_normal, diagonal_radius);
		scaled_vertex = opus_vec2_sub(*vertex, scaled_vertex);

		precision = quality;

		if (quality == -1) {
			/* automatically decide precision */
			precision = opus_pow(current_radius, 0.32) * 1.75;
		}

		precision = opus_clamp(precision, quality_min, quality_max);

		/* use an even value for precision, more likely to reduce axes by using symmetry */
		if (fmod(precision, 2) == 1) precision += 1;

		alpha = opus_acos(opus_vec2_dot(prev_normal, next_normal));
		theta = alpha / precision;

		for (j = 0; j < (int) precision; j++) {
			opus_vec2 v;
			v = opus_vec2_rotate(radius_vector, theta * j);
			v = opus_vec2_add(v, scaled_vertex);
			opus_arr_push(new_vertices, &v);
		}
	}

	return new_vertices;
}

void opus_center(opus_vec2 *coords, size_t n, opus_vec2 *out_center)
{
	opus_set_polygon_offset(0, sizeof(opus_vec2));
	opus_center_(coords, n, out_center);
}

void opus_mean(opus_vec2 *coords, size_t n, opus_vec2 *out_mean)
{
	opus_set_polygon_offset(0, sizeof(opus_vec2));
	opus_mean_(coords, n, out_mean);
}

opus_real polygon_area(opus_vec2 *coords, size_t n, int is_signed)
{
	opus_set_polygon_offset(0, sizeof(opus_vec2));
	return opus_area_(coords, n, is_signed);
}

opus_real opus_inertia(opus_vec2 *coords, size_t n, opus_real mass)
{
	opus_set_polygon_offset(0, sizeof(opus_vec2));
	return opus_inertia_(coords, n, mass);
}

void opus_translate(opus_vec2 *coords, size_t n, opus_vec2 t, opus_vec2 scalar)
{
	opus_set_polygon_offset(0, sizeof(opus_vec2));
	opus_translate_(coords, n, t, scalar);
}

void opus_rotate(opus_vec2 *coords, size_t n, opus_vec2 point, opus_real angle)
{
	opus_set_polygon_offset(0, sizeof(opus_vec2));
	opus_rotate_(coords, n, point, angle);
}

void opus_scale(opus_vec2 *coords, size_t n, opus_vec2 origin, opus_vec2 scalar)
{
	opus_set_polygon_offset(0, sizeof(opus_vec2));
	opus_scale_(coords, n, origin, scalar);
}

int opus_contains(opus_vec2 *coords, size_t n, opus_vec2 point)
{
	opus_set_polygon_offset(0, sizeof(opus_vec2));
	return opus_contains_(coords, n, point);
}

void opus_make_cw(opus_vec2 *coords, size_t n)
{
	opus_set_polygon_offset(0, sizeof(opus_vec2));
	opus_make_cw_(coords, n);
}

void opus_make_ccw(opus_vec2 *coords, size_t n)
{
	opus_set_polygon_offset(0, sizeof(opus_vec2));
	opus_make_ccw_(coords, n);
}

int opus_is_convex(opus_vec2 *coords, size_t n)
{
	opus_set_polygon_offset(0, sizeof(opus_vec2));
	return opus_is_convex_(coords, n);
}

opus_vec2 *opus_contour(opus_vec2 *coords, size_t n)
{
	opus_set_polygon_offset(0, sizeof(opus_vec2));
	return opus_contour_(coords, n);
}

opus_vec2 *opus_chamfer(opus_vec2 *coords, size_t n, const opus_real *radius, size_t n_radius,
                           opus_real quality, opus_real quality_min, opus_real quality_max)
{
	opus_set_polygon_offset(0, sizeof(opus_vec2));
	return opus_chamfer_(coords, n, radius, n_radius, quality, quality_min, quality_max);
}
