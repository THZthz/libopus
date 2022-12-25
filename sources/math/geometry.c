/**
 * @file geo.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/8/7
 *
 * @example
 *
 * @development_log
 *		for running speed:
 *			1` use less condition judgements
 *			2` use pointer
 */

#include "math/geometry.h"

#include <float.h>
#include <stdio.h>
#include <stdlib.h>

#include "data_structure/array.h"

#define vec_p_x(_v) (((vec2 *) (_v))->x)
#define vec_p_y(_v) (((vec2 *) (_v))->y)
#define vec_p_vx(_v) ((real) vec_p_x(_v))
#define vec_p_vy(_v) ((real) vec_p_y(_v))
#define vec_p(_v) ((vec2 *) (_v))
#define vec_p_n(_i) (vec_p((char *) vertices + (_i) *offset))

/**
 * @see https://en.wikipedia.org/wiki/Centroid#Centroid_of_a_polygon
 */
void vertices_center(void *vertices, size_t n, void *out_center, ptrdiff_t offset)
{
	void    *v1, *v2, *v_end = (char *) vertices + n * offset;
	uint64_t i;
	real     cx = 0, cy = 0, ar = 0, a;
	for (v1 = vertices, i = 0; v1 < v_end; (v1 = (char *) v1 + offset), (i++)) {
		v2 = (char *) vertices + offset * ((i + 1) % n);
		a  = vec2_cross(*(vec2 *) v1, *(vec2 *) v2);
		cx += a * (vec_p_vx(v1) + vec_p_vx(v2));
		cy += a * (vec_p_vy(v1) + vec_p_vy(v2));
		ar += a;
	}
	ar *= 3;
	vec_p_x(out_center) = cx / ar;
	vec_p_y(out_center) = cy / ar;
}

/**
 * @return the area of a polygon (convex)
 */
real vertices_area(void *vertices, size_t n, int is_signed, ptrdiff_t offset)
{
	real  area = 0;
	void *vi, *v_end = (char *) vertices + n * offset, *vj = (char *) v_end - offset;

	for (vi = vertices; vi < v_end; vi = (char *) vi + offset) {
		area += (vec_p_vx(vj) - vec_p_vx(vi)) * (vec_p_vy(vj) + vec_p_vy(vi));
		vj = vi;
	}

	if (is_signed)
		return area / 2;

	return r_abs(area) / 2;
}

/* get the average center of the set of vertices */
void vertices_mean(void *vertices, size_t n, void *out_mean, ptrdiff_t offset)
{
	real  sx = 0, sy = 0;
	void *vi, *v_end = (char *) vertices + n * offset;

	for (vi = vertices; vi < v_end; vi = (char *) vi + offset) {
		sx += vec_p_x(vi);
		sy += vec_p_y(vi);
	}

	vec_p_x(out_mean) = sx / (real) n;
	vec_p_y(out_mean) = sy / (real) n;
}

/* the moment of inertia (second moment of area) of the set of vertices given the total mass */
real vertices_inertia(void *vertices, size_t n, real mass, ptrdiff_t offset)
{
	real   cross, numerator = 0, denominator = 0;
	size_t i; /* index of vi */
	vec2  *vi, *vj, *v_end = (vec2 *) ((char *) vertices + n * offset);

	/*
	 * find the polygon's moment of inertia, using second moment of area
	 * from equations at http://www.physicsforums.com/showthread.php?t=25293
	 */
	for (vi = vertices, i = 0; vi < v_end; vi = (vec2 *) ((char *) vi + offset), i++) {
		vj    = (vec2 *) ((char *) vertices + ((i + 1) % n) * offset);
		cross = r_abs(vec2_cross(*vj, *vi));
		numerator += cross * (vec2_dot(*vj, *vj) + vec2_dot(*vj, *vi) + vec2_dot(*vi, *vi));
		denominator += cross;
	}

	return (mass / 6) * (numerator / denominator);
}

/* translate vertices in place */
void *vertices_translate(void *vertices, size_t n, vec2 vec, real scalar, ptrdiff_t offset)
{
	real tx = vec.x * scalar,
	     ty = vec.y * scalar;
	size_t i;
	vec2  *v;

	for (i = 0; i < n; i++) {
		v = (vec2 *) ((char *) vertices + i * offset);
		v->x += tx;
		v->y += ty;
	}

	return vertices;
}

/* rotate the set of vertices around a point with a certain angle */
void *vertices_rotate(void *vertices, size_t n, vec2 point, real angle, ptrdiff_t offset)
{
	real   c, s, dx, dy;
	size_t i;

	if (angle == 0) return vertices; /* avoid extra calculation */
	c = r_cos(angle);
	s = r_sin(angle);
	for (i = 0; i < n; i++) {
		vec2 *v = (vec2 *) ((char *) vertices + i * offset);
		dx      = v->x - point.x;
		dy      = v->y - point.y;
		v->x    = point.x + (dx * c - dy * s);
		v->y    = point.y + (dx * s + dy * c);
	}

	return vertices;
}

/* scale the set of vertices in place */
void *vertices_scale(void *vertices, size_t n, vec2 origin, real scalar_x, real scalar_y, ptrdiff_t offset)
{
	size_t i;
	if (scalar_x == 1 && scalar_y == 1) return vertices;

	for (i = 0; i < n; i++) {
		vec2 *v = vec_p_n(i);
		vec2  delta;
		vec2_sub(delta, *v, origin);
		v->x = origin.x + delta.x * scalar_x;
		v->y = origin.y + delta.y * scalar_y;
	}

	return vertices;
}

/* test if a set of vertices contains a point */
int vertices_contains(void *vertices, size_t n, vec2 point, ptrdiff_t offset)
{
	size_t i;
	vec2  *v = vec_p_n(n - 1), *v_next;

	for (i = 0; i < n; i++) {
		v_next = vec_p_n(i);

		if ((point.x - v->x) * (v_next->y - v->y) + (point.y - v->y) * (v->x - v_next->x) > 0) {
			return 0;
		}

		v = v_next;
	}

	return 1;
}

static vec2 center;
int         sort_clock_wise_callback_(const void *a, const void *b)
{
	vec2 *va = (vec2 *) a;
	vec2 *vb = (vec2 *) b;
	return r_sign(vec2_angle(center, *va) - vec2_angle(center, *vb));
}

/* sort the set of vertices in place */
void *vertices_sort_clockwise(void *vertices, size_t n, ptrdiff_t offset)
{
	vertices_center(vertices, n, &center, offset);
	qsort(vertices, n, offset, sort_clock_wise_callback_);
	return vertices;
}

int vertices_is_convex(void *vertices, size_t n, ptrdiff_t offset)
{
	/* http://paulbourke.net/geometry/polygonmesh/ */
	/* Copyright (c) Paul Bourke (use not permitted, but ...) */

	int    flag = 0;
	size_t i;
	real   z;
	vec2  *vi;

	if (n < 3)
		return 0;

	for (i = 0, vi = (vec2 *) vertices; i < n; i++, vi = vec_p((char *) vi + offset)) {
		vec2 *vj = vec_p_n((i + 1) % n),
		     *vk = vec_p_n((i + 2) % n);
		z        = (vj->x - vi->x) * (vk->y - vj->y);
		z -= (vj->y - vi->y) * (vk->x - vj->x);

		if (z < 0) {
			flag |= 1;
		} else if (z > 0) {
			flag |= 2;
		}

		if (flag == 3) {
			return 0;
		}
	}

	if (flag != 0) {
		return 1;
	} else {
		return 0;
	}
}

static int vertices_sort_x_axis(const void *a, const void *b)
{
	vec2 *pa = (vec2 *) a;
	vec2 *pb = (vec2 *) b;
	real  dx = pa->x - pb->x;
	dx       = dx != 0 ? dx : pa->y - pb->y;
	return r_sign(dx);
}

/**
 * @brief get the convex hull of the input vertices as a new array of points
 * @param vertices
 * @return remember to release its memory by using "array_destroy"
 */
vec2 *vertices_hull(void *vertices, size_t n, ptrdiff_t offset)
{
	/* algorithm from http://geomalgorithms.com/a10-_hull-1.html */

	int64_t i;
	vec2   *v;
	vec2   *vertices_copy, *upper, *lower;
	array_create(vertices_copy, sizeof(vec2));
	array_create(upper, sizeof(vec2));
	array_create(lower, sizeof(vec2));

	/* sort vertices on x-axis (y-axis for ties) */
	array_resize(vertices_copy, n);
	for (i = 0; i < n; i++) {
		v = vec_p_n(i);
		vec2_dup(vertices_copy[i], *v);
	}
	qsort(vertices_copy, n, sizeof(vec2), vertices_sort_x_axis);

	/* build lower hull */
	for (i = 0; i < n; i += 1) {
		v = &vertices_copy[i];

		while (array_len(lower) >= 2 && vec2_cross3(lower[array_len(lower) - 2], lower[array_len(lower) - 1], *v) <= 0)
			array_pop(lower);

		array_push(lower, v);
	}

	/* build upper hull */
	for (i = n - 1; i >= 0; i -= 1) {
		v = &vertices_copy[i];

		while (array_len(upper) >= 2 && vec2_cross3(upper[array_len(upper) - 2], upper[array_len(upper) - 1], *v) <= 0)
			array_pop(upper);

		array_push(upper, v);
	}

	/* concatenation of the lower and upper hulls gives the convex hull */
	/* omit last points because they are repeated at the beginning of the other list */
	array_pop(lower);
	array_pop(upper);
	array_concat(upper, lower);
	array_destroy(lower);
	array_destroy(vertices_copy);

	return upper;
}

/*
 * You can specify the 3 following options to - 1 to set it to default value
 * quality defaults to -1,
 * quality_min defaults to 2,
 * quality_max defaults to 14 */
vec2 *vertices_chamfer(void *vertices, size_t n, real *radius, size_t n_radius, real quality, real quality_min, real quality_max, ptrdiff_t offset)
{
	vec2  *new_vertices;
	size_t i, j;

	if (quality_min == -1) quality_min = 2;
	if (quality_max == -1) quality_max = 14;

	array_create(new_vertices, sizeof(vec2));
	for (i = 0; i < n; i++) {
		real  current_radius = radius[i < n_radius ? i : n_radius - 1];
		real  diagonal_radius;
		real  precision;
		real  alpha, theta;
		vec2 *prev_vertex = vec_p_n(i - 1 >= 0 ? i - 1 : n - 1),
		     *vertex      = vec_p_n(i),
		     *next_vertex = vec_p_n((i + 1) % n);
		vec2 prev_normal, next_normal;
		vec2 radius_vector, mid_normal, scaled_vertex;

		if (current_radius == 0) {
			array_push(new_vertices, vertex);
			continue;
		}

		vec2_set(prev_normal, vertex->y - prev_vertex->y, prev_vertex->x - vertex->x);
		vec2_norm(prev_normal, prev_normal);

		vec2_set(next_normal, next_vertex->y - vertex->y, vertex->x - next_vertex->x);
		vec2_norm(next_normal, next_normal);

		diagonal_radius = r_sqrt(2 * r_pow(current_radius, 2));
		vec2_set(radius_vector, prev_normal.x * current_radius, prev_normal.y * current_radius);
		vec2_set(mid_normal, (prev_normal.x + next_normal.x) / 2, (prev_normal.y + next_normal.y) / 2);
		vec2_norm(mid_normal, mid_normal);
		vec2_scale(scaled_vertex, mid_normal, diagonal_radius);
		vec2_sub(scaled_vertex, *vertex, scaled_vertex);

		precision = quality;

		if (quality == -1) {
			/* automatically decide precision */
			precision = r_pow(current_radius, 0.32) * 1.75;
		}

		precision = r_clamp(precision, quality_min, quality_max);

		/* use an even value for precision, more likely to reduce axes by using symmetry */
		if (fmod(precision, 2) == 1)
			precision += 1;

		alpha = r_acos(vec2_dot(prev_normal, next_normal));
		theta = alpha / precision;

		for (j = 0; j < (int) precision; j++) {
			vec2 v;
			vec2_rotate(v, radius_vector, theta * j);
			vec2_add(v, v, scaled_vertex);
			array_push(new_vertices, &v);
		}
	}

	return new_vertices;
}

/*              |     (middle)   |
 *     (left)  [S]--------------[E]  (right)
 *             |     (middle)   |
 */
/**
 * @brief determine which voronoi region a point is on a line segment.
 * 	Assume that both the line and the point are relative to `(0,0)`
 * @return r_geo_left_voronoi_region if in left voronoi region
 * 		   r_geo_middle_voronoi_region if in middle voronoi region
 * 		   r_geo_right_voronoi_region if in right voronoi region
 */
int get_point_voronoi_region(vec2 line, vec2 point)
{
	real len2 = vec2_length2(line);
	real dp   = vec2_dot(point, line);
	if (dp < 0) {
		return left_voronoi_region;
	} else if (dp > len2) {
		return right_voronoi_region;
	} else {
		return middle_voronoi_region;
	}
}

int get_point_voronoi_region2(vec2 line_s, vec2 line_e, vec2 point)
{
	real len2, dp;
	vec2 line, rel_point;
	vec2_sub(line, line_e, line_s);
	vec2_sub(rel_point, point, line_s);
	len2 = vec2_length2(line);
	dp   = vec2_dot(rel_point, line);
	if (dp < 0) {
		return left_voronoi_region;
	} else if (dp > len2) {
		return right_voronoi_region;
	} else {
		return middle_voronoi_region;
	}
}

void get_nearest_point_on_line(vec2 line_a, vec2 line_b, vec2 point, vec2 *res)
{
	real line_len2, proj, t;
	vec2 va_t_p, line;
	vec2_sub(va_t_p, point, line_a);
	vec2_sub(line, line_b, line_a);

	line_len2 = vec2_get_length2(line);

	/*if the length of the line is nearly 0*/
	if (fabs((real) line_len2 - 0) < DBL_EPSILON) {
		vec2_copy(*res, line_a);
		return;
	}

	proj = vec2_dot(va_t_p, line);
	t    = proj / line_len2;
	t    = r_clamp(t, .0, 1.0); /*TODO: is it necessary*/
	vec2_scale(*res, line, t);
	vec2_add(*res, *res, line_a);
}

void aabb_init_with_range(aabb_t *aabb, real x, real y, real w, real h)
{
	aabb->min_x = x;
	aabb->min_y = y;
	aabb->max_x = x + w;
	aabb->max_y = y + h;
}

real aabb_get_width(aabb_t *aabb)
{
	return aabb->max_x - aabb->min_x;
}

real rect_get_height(aabb_t *aabb)
{
	return aabb->max_y - aabb->min_y;
}

void aabb_copy(aabb_t *dest, aabb_t *src)
{
	dest->min_x = src->min_x;
	dest->min_y = src->min_y;
	dest->max_x = src->max_x;
	dest->max_y = src->max_y;
}

void aabb_combine(aabb_t *a, aabb_t *b, aabb_t *result)
{
	result->min_x = a->min_x < b->min_x ? a->min_x : b->min_x;
	result->max_x = a->max_x > b->max_x ? a->max_x : b->max_x;
	result->min_y = a->min_y < b->min_y ? a->min_y : b->min_y;
	result->max_y = a->max_y > b->max_y ? a->max_y : b->max_y;
}

real aabb_get_perimeter(aabb_t *aabb)
{
	real wx = aabb->max_x - aabb->min_x;
	real wy = aabb->max_y - aabb->min_y;
	return 2.0 * (wx + wy);
}

int aabb_is_overlap_with(aabb_t *a, aabb_t *b)
{
	return (a->min_x <= b->max_x && a->min_y <= b->max_y && a->max_x >= b->min_x && a->max_y >= b->min_y);
}

int aabb_is_contain_point(aabb_t *aabb, real x, real y)
{
	return (x <= aabb->max_x && x >= aabb->min_x && y <= aabb->max_y && y >= aabb->min_y);
}

int aabb_is_ray_intersect(vec2 origin, vec2 direction, aabb_t *aabb, real *t)
{
	return is_ray_intersect_rect(origin, direction,
	                             vec2_(aabb->min_x, aabb->min_y),
	                             aabb->max_x - aabb->min_x, aabb->max_y - aabb->min_y, t);
}

void aabb_init(aabb_t *aabb, real min_x, real min_y, real max_x, real max_y)
{
	aabb->min_x = min_x;
	aabb->min_y = min_y;
	aabb->max_x = max_x;
	aabb->max_y = max_y;
}

aabb_t *aabb_create(real min_x, real min_y, real max_x, real max_y)
{
	aabb_t *aabb = (aabb_t *) malloc(sizeof(aabb_t));
	aabb_init(aabb, min_x, min_y, max_x, max_y);

	return aabb;
}

void aabb_destroy(aabb_t *aabb)
{
	free(aabb);
}

/**
 * translate the AABB box
 */
void aabb_translate(aabb_t *aabb, real dx, real dy)
{
	aabb->min_x += dx;
	aabb->max_x += dx;
	aabb->min_y += dy;
	aabb->max_y += dy;
}

/**
 * shift the aabb bounding box to a designated position(bottom left).
 */
void aabb_shift(aabb_t *aabb, real x, real y)
{
	real width  = aabb->max_x - aabb->min_x;
	real height = aabb->max_y - aabb->min_y;

	aabb->min_x = x;
	aabb->min_y = y;
	aabb->max_x = x + width;
	aabb->max_y = y + height;
}

INLINE int get_line_line_intersection(real line_ax1, real line_ay1, real line_ax2, real line_ay2,
                                            real line_bx1, real line_by1, real line_bx2, real line_by2,
                                            real *x, real *y, real *t1, real *t2)
{
	real line_adx, line_ady, line_bdx, line_bdy;

	/* Line_a: (line_ax1, line_ay1) + T1 * (line_adx, line_ady) */
	line_adx = line_ax2 - line_ax1;
	line_ady = line_ay2 - line_ay1;

	/* Line_b: (line_bx1, line_by1) + T2 * (line_bdx, line_bdy) */
	line_bdx = line_bx2 - line_bx1;
	line_bdy = line_by2 - line_by1;

	/* if parallel, no intersect */
	if ((line_adx == 0 && line_bdx == 0) || (line_ady * line_bdx == line_bdy * line_adx)) return 0;

	/* solve for parametric T1 and T2 */
	/* r_px+r_dx*T1 = s_px+s_dx*T2 && r_py+r_dy*T1 =s_py+s_dy*T2 */
	*t2 = (line_adx * (line_by1 - line_ay1) + line_ady * (line_ax1 - line_bx1)) / (line_bdx * line_ady - line_bdy * line_adx);

	/* line_adx or line_ady might be zero */
	*t1 = line_adx != 0 ? (line_bx1 + line_bdx * (*t2) - line_ax1) / line_adx : (line_by1 + line_bdy * (*t2) - line_ay1) / line_ady;
	*x  = line_ax1 + line_adx * (*t1);
	*y  = line_ay1 + line_ady * (*t1);

	return 1;
}

INLINE int is_ray_intersect_rect(vec2 ray_ori, vec2 ray_dir, vec2 rect_pos, real width, real height, real *t)
{
	real dir_frac_x, dir_frac_y, t1, t2, t3, t4, temp, t_min, t_max;

	/*  TODO: this could be reused when testing a ray is intersect with multiple AABB boxes */
	dir_frac_x = 1.0f / ray_dir.x;
	dir_frac_y = 1.0f / ray_dir.y;

	t1 = (rect_pos.x - ray_ori.x) * dir_frac_x;
	t2 = (rect_pos.x + width - ray_ori.x) * dir_frac_x;
	t3 = (rect_pos.y - ray_ori.y) * dir_frac_y;
	t4 = (rect_pos.y + height - ray_ori.y) * dir_frac_y;

	if (t1 < t2) {
		temp = t1;
		t1   = t2;
		t2   = temp;
	}
	if (t3 < t4) {
		temp = t3;
		t3   = t4;
		t4   = temp;
	}

	t_min = t2 > t4 ? t2 : t4;
	t_max = t1 < t3 ? t1 : t3;

	/* if t_max < 0, ray (line) is intersecting AABB, but the whole AABB is behind us */
	if (t_max < 0) {
		*t = t_max;
		return 0;
	}

	/* if t_min > t_max, ray doesn't intersect AABB */
	if (t_min > t_max) {
		*t = t_max;
		return 0;
	}

	*t = t_min;
	return 1;
}

/*
 * One simple way to detect if point D lies in the circum-circle of points A, B, C (triangle)
 *  is to evaluate the determinant:
 *    |  Ax  Ay  (Ax * Ax + Ay * Ay)  1  |
 *    |  Bx  By  (Bx * Bx + By * By)  1  |
 *    |  Cx  Cy  (Cx * Cx + Cy * Cy)  1  |
 *    |  Dx  Dy  (Dx * Dx + Dy * Dy)  1  |
 *
 * Which equals to:
 *    |  (Ax - Dx)  (Ay - Dy)  (Ax * Ax - Dx * Dx)  |
 *    |  (Bx - Dx)  (By - Dy)  (Bx * Bx - Dx * Dx)  |
 *    |  (Cx - Dx)  (Cy - Dy)  (Cx * Cx - Dx * Dx)  |
 *
 * When A, B and C are sorted in a counter-clockwise order, this determinant is positive
 *  only if D lies in the circum-circle.
 *
 */
INLINE int is_point_in_triangle(real x, real y, real x1, real y1, real x2, real y2, real x3, real y3)
{
	real e1_x = x1 - x;
	real e1_y = y1 - y;
	real e2_x = x2 - x;
	real e2_y = y2 - y;
	real e3_x = x3 - x;
	real e3_y = y3 - y;
	real t1   = e1_x * e2_y - e2_x * e1_y;
	real t2   = e2_x * e3_y - e3_x * e2_y;
	real t3   = e3_x * e1_y - e1_x * e3_y;

	/* when lies in the bound of the triangle, we also think it is "in" triangle */
	return t1 * t2 >= 0 && t1 * t3 >= 0;
}

INLINE void get_circum_center_of_triangle(real ax, real ay, real bx, real by, real cx, real cy, real *xc, real *yc, real *r_sq)
{
	real dx, dy, ex, ey;
	real bl, cl, d, r;

	dx = bx - ax, dy = by - ay;
	ex = cx - ax, ey = cy - ay;
	cl = ex * ex + ey * ey;
	bl = dx * dx + dy * dy;
	d  = (real) 0.5 / (dx * ey - dy * ex);

	*xc = (ey * bl - dy * cl) * d;
	*yc = (dx * cl - ex * bl) * d;
	*r_sq = *xc * *xc + *yc * *yc;
	*xc += ax, *yc += ay;
}
