/**
 * @file polygon.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/12/29
 *
 * @brief tessellate a polygon (convex or concave) with holes
 *
 * @example
 *
 * @development_log
 *
 */


#include "math/math.h"
#include "math/geometry.h"
#include "math/polygon/polygon.h"
#include "data_structure/array.h"

static void build_index_array(size_t **array, size_t n)
{
	size_t i;
	opus_arr_resize(*array, n);
	for (i = 0; i < n; i++) (*array)[i] = i;
}

static int in_triangle(opus_vec2 p, opus_vec2 t0, opus_vec2 t1, opus_vec2 t2)
{
	return opus_is_point_in_triangle(p.x, p.y, t0.x, t0.y, t1.x, t1.y, t2.x, t2.y);
}

static void get_neighbors(size_t cur, size_t *prev, size_t *next, size_t n_polygon)
{
	*prev = (cur + n_polygon - 1) % n_polygon;
	*next = (cur + 1) % n_polygon;
}

static int is_reflex_index(size_t cur, const opus_vec2 *coords, const size_t *polygon)
{
	opus_vec2 ac, ab;
	size_t prev, next;
	get_neighbors(cur, &prev, &next, opus_arr_len(polygon));
	ac = opus_vec2_to(coords[polygon[prev]], coords[polygon[next]]);
	ab = opus_vec2_to(coords[polygon[prev]], coords[polygon[cur]]);
	return opus_vec2_cross(ac, ab) < 0;
}

static void get_slice(size_t slice[2], opus_vec2 *hole, opus_vec2 *coords, const size_t *polygon,
                      const size_t *reflex)
{
	size_t Mn, Pn, Rn, i, j, cur;
	opus_vec2 M, I, P, R;
	opus_real   x_rm = -OPUS_REAL_MAX; /* right-most X */
	opus_real   E    = 0.0001;    /* epsilon */
	opus_real   R_slope;
	size_t n;

	/* get right-most X */
	for (i = 0, n = opus_arr_len(hole); i < n; i++) {
		opus_vec2 c = hole[i];
		if (c.x > x_rm) {
			x_rm = c.x;
			Mn   = i;
		}
	}

	M = hole[Mn];
	opus_vec2_set(&I, OPUS_REAL_MAX, M.y);
	Pn = (size_t) -1;

	for (cur = 0, n = opus_arr_len(coords); cur < n; cur++) {
		opus_vec2   c0   = coords[cur];
		size_t next = (cur + 1) % n;
		opus_vec2   c1   = coords[next];
		if (c0.x < M.x && c1.x < M.x) continue;
		if (c0.x > I.x && c1.x > I.x) continue;
		if ((c0.y <= M.y && M.y <= c1.y) || (c1.y <= M.y && M.y <= c0.y)) {
			opus_real x, t;
			/* get segment intersection X */
			if (c0.y == c1.y) { x = c0.x; }
			if (c0.y < c1.y) {
				t = (M.y - c0.y) / (c1.y - c0.y);
				x = c0.x + t * (c1.x - c0.x);
			} else {
				t = (M.y - c1.y) / (c0.y - c1.y);
				x = c1.x + t * (c0.x - c1.x);
			}
			if (x < I.x) {
				I.x = x;
				if (c0.x > c1.x) {
					P  = c0;
					Pn = cur;
				} else {
					P  = c1;
					Pn = next;
				}
			}
		}
	}

	R_slope = OPUS_REAL_MAX;
	Rn      = (size_t) -1;
	for (i = 0, n = opus_arr_len(polygon); i < n; i++) {
		j = polygon[i];
		if (i < opus_arr_len(reflex) && !reflex[i]) continue;
		R = coords[j];
		if (in_triangle(R, M, I, P)) {
			opus_real dy = opus_abs(R.y - P.y);
			opus_real dx = opus_abs(R.x - P.x);
			opus_real slope;
			if (dx == 0) continue;
			slope = dy / dx;
			if (slope < R_slope) {
				R_slope = slope;
				Rn      = j;
			}
		}
	}

	if (Rn != (size_t) -1) { Pn = Rn; }
	slice[0] = Pn;
	slice[1] = Mn;
}

static int check_ear(size_t cur, size_t reflex_count, opus_vec2 *coords, const size_t *reflex,
                     const size_t *polygon)
{
	size_t i, n;
	size_t prev, next, t1, t2, t3;
	int    is_ear;

	if (reflex_count == 0) return 1;

	get_neighbors(cur, &prev, &next, opus_arr_len(polygon));
	t1 = polygon[prev];
	t2 = polygon[cur];
	t3 = polygon[next];

	is_ear = 1;
	n      = opus_arr_len(polygon);
	for (i = 0; i < n; i++) {
		size_t p = polygon[i];
		if (p == t1 || p == t2 || p == t3) continue;
		if (i < opus_arr_len(reflex) && !reflex[i]) continue;
		if (in_triangle(coords[p], coords[t1], coords[t2], coords[t3])) {
			is_ear = 0;
			break;
		}
	}
	return is_ear;
}

static void foo(size_t neighbor, size_t *reflex_count, size_t **ears_p, size_t *reflex,
                opus_vec2 *coords, size_t *polygon)
{
	size_t *ears = *ears_p;
	if (neighbor >= opus_arr_len(reflex)) return;
	if (reflex[neighbor] && !is_reflex_index(neighbor, coords, polygon)) {
		reflex[neighbor] = 0;
		(*reflex_count)--;
	}
	if (!reflex[neighbor]) {
		size_t i, n = opus_arr_len(ears), idx;
		int    is_ear  = check_ear(neighbor, *reflex_count, coords, reflex, polygon);
		int    was_ear = 0;
		for (i = 0; i < n; i++) {
			if (ears[i] == neighbor) {
				was_ear = 1;
				idx     = i;
				break;
			}
		}
		if (is_ear && !was_ear) {
			opus_arr_push(ears, &neighbor);
		} else if (!is_ear && was_ear) {
			opus_arr_remove(ears, idx);
		}
	}
}

void print(char *text, size_t *arr)
{
	size_t i, n;
	printf(text);
	for (i = 0, n = opus_arr_len(arr); i < n; i++) { printf("%lu  ", arr[i]); }
	printf("\n");
}

/**
 * @brief tessellate a polygon into a series of triangles.
 * 	 This algorithm is based on ear-cutting method.
 * @param coords_ptr in CCW
 * @param holes in CCW
 * @return a series of indices (3 form a group, which is a triangle)
 */
size_t *opus_tessellate(opus_vec2 **coords_ptr, opus_vec2 **holes)
{
	opus_vec2 *coords = *coords_ptr;
	size_t *triangles, *polygon, *reflex, *convex, *ears;
	size_t  i, j, n;
	size_t  reflex_count;

	n = opus_arr_len(coords);
	if (n < 3) return NULL;

	opus_arr_create(triangles, sizeof(size_t));
	if (n == 3 && (!holes || opus_arr_len(holes) == 0)) {
		build_index_array(&triangles, 3);
		return triangles;
	}

	opus_arr_create(polygon, sizeof(size_t));
	opus_arr_create(reflex, sizeof(size_t));
	build_index_array(&polygon, n);

	if (holes && opus_arr_len(holes) > 0 && opus_arr_len(holes[0]) >= 3) {
		opus_vec2 *hole, *t_coords;
		size_t slice[2], hole_start, *t_polygon, p;

		for (i = 0, n = opus_arr_len(polygon); i < n; i++) {
			size_t ir = is_reflex_index(i, coords, polygon);
			opus_arr_push(reflex, &ir);
		}

		hole = holes[0];
		get_slice(slice, hole, coords, polygon, reflex);

		/* create a copy of coords */
		n = opus_arr_len(coords);
		opus_arr_create(t_coords, sizeof(t_coords[0]));
		opus_arr_resize(t_coords, n);
		memcpy(t_coords, coords, sizeof(opus_vec2) * n);
		opus_arr_destroy(coords);
		coords = t_coords;

		hole_start = n;
		for (i = 0, n = opus_arr_len(hole); i < n; i++) {
			opus_vec2 h = hole[i];
			opus_arr_push(coords, &h);
		}

		opus_arr_create(t_polygon, sizeof(size_t));
		n = opus_arr_len(polygon);
		i = (slice[0] + 1) % n;
		for (j = 0; j < n; j++) {
			p = polygon[i];
			opus_arr_push(t_polygon, &p);
			i = (i + 1) % n;
		}

		n = opus_arr_len(hole);
		i = slice[1];
		for (j = 0; j < n; j++) {
			p = hole_start + i;
			opus_arr_push(t_polygon, &p);
			i = (i + 1) % n;
		}

		n = opus_arr_len(polygon);
		p = t_polygon[n];
		opus_arr_push(t_polygon, &p);
		p = t_polygon[n - 1];
		opus_arr_push(t_polygon, &p);

		opus_arr_destroy(polygon);
		polygon = t_polygon;
	}

	/* find all reflex vertex */
	reflex_count = 0;
	opus_arr_clear(reflex);
	opus_arr_create(convex, sizeof(size_t));
	n = opus_arr_len(polygon);
	for (i = 0; i < n; i++) {
		size_t p;
		if (is_reflex_index(i, coords, polygon)) {
			p = 1;
			opus_arr_push(reflex, &p);
			reflex_count++;
		} else {
			opus_arr_push(convex, &i);
			p = 0;
			opus_arr_push(reflex, &p);
		}
	}

	/* find all initial ears */
	opus_arr_create(ears, sizeof(size_t));
	n = opus_arr_len(convex);
	for (i = 0; i < n; i++) {
		size_t p = convex[i];
		if (check_ear(p, reflex_count, coords, reflex, polygon)) { opus_arr_push(ears, &p); }
	}

	/* remove ears incrementally */
	opus_arr_clear(triangles);
	while (opus_arr_len(polygon) > 0) {
		size_t cur = ears[opus_arr_len(ears) - 1];
		size_t prev, next;
		size_t p;

		opus_arr_pop(ears);

		/* insert the ear into triangle list */
		get_neighbors(cur, &prev, &next, opus_arr_len(polygon));
		p = polygon[prev];
		opus_arr_push(triangles, &p);
		p = polygon[cur];
		opus_arr_push(triangles, &p);
		p = polygon[next];
		opus_arr_push(triangles, &p);

		/* remove the ear */
		opus_arr_remove(polygon, cur);
		opus_arr_remove(reflex, cur);

		n = opus_arr_len(ears);
		for (i = 0; i < n; i++) {
			p = ears[i];
			if (p > cur) ears[i]--;
		}
		if (next > cur) next--;
		if (prev > cur) prev--;

		foo(prev, &reflex_count, &ears, reflex, coords, polygon);
		foo(next, &reflex_count, &ears, reflex, coords, polygon);
	}

	opus_arr_destroy(ears);
	opus_arr_destroy(convex);
	opus_arr_destroy(polygon);
	opus_arr_destroy(reflex);

	*coords_ptr = coords;
	return triangles;
}
