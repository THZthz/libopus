/**
 * @file delaunay.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/10/20
 *
 * @brief DIRECT PORT OF Delaunator written by mapbox from https://github.com/mapbox/delaunator.
 * 		Some explanation of the data structure can be found at https://mapbox.github.io/delaunator/.
 *
 * 		I have also written one by myself, but its running speed does not meet my requirement
 * 			(now it lays in the "trash" folder, you can find it if it is not deleted by me :P).
 *
 * @development_log
 *
 */



#include <stdlib.h>

#include "delaunay.h"
#include "math/geometry.h"
#include "math/math.h"
#include "utils/utils.h"

void opus_delaunay_init(opus_delaunay_data *data, opus_real *coords, uint32_t n)
{
#define ALLOC__(type, dst, ele_count)                                    \
	do {                                                                 \
		if ((dst) == NULL)                                               \
			(dst) = (type *) malloc((ele_count) * sizeof(type));         \
		else                                                             \
			(dst) = (type *) realloc((dst), (ele_count) * sizeof(type)); \
	} while (0)

	uint64_t max_triangles, i;

	data->coords = coords;
	data->n      = n;

	if (n >> 1 < 3) return; /* already completed the triangulation */

	n             = n >> 1;
	max_triangles = 2 * n - 5;
	ALLOC__(uint32_t, data->triangles_, max_triangles * 3);
	ALLOC__(uint32_t, data->half_edges_, max_triangles * 3);

	data->hash_size_ = opus_ceil(opus_sqrt(n));
	ALLOC__(uint32_t, data->hull_prev_, n);
	ALLOC__(uint32_t, data->hull_next_, n);
	ALLOC__(uint32_t, data->hull_tri_, n);
	ALLOC__(uint32_t, data->hull_hash_, data->hash_size_);

	for (i = 0; i < data->hash_size_; i++) data->hull_hash_[i] = -1;

	ALLOC__(uint32_t, data->ids_, n);
	ALLOC__(opus_real, data->dists_, n);

	data->hull_         = NULL; /* allocate space when calling "delaunay_triangulate" */
	data->n_triangles_  = 0;
	data->n_half_edges_ = 0;
	data->hull_start_   = -1;

#undef ALLOC__
}

static OPUS_INLINE void get_bounding_box_(const opus_real *coords, uint32_t n, uint32_t *ids,
                                     opus_real *min_x, opus_real *min_y, opus_real *max_x,
                                     opus_real *max_y, opus_real *cx, opus_real *cy)
{
	uint32_t i;

	*min_x = OPUS_REAL_MAX;
	*min_y = OPUS_REAL_MAX;
	*max_x = -OPUS_REAL_MAX;
	*max_y = -OPUS_REAL_MAX;
	for (i = 0; i < n; i++) {
		opus_real x = coords[2 * i];
		opus_real y = coords[2 * i + 1];
		if (x < *min_x) *min_x = x;
		else if (x > *max_x)
			*max_x = x;
		if (y < *min_y) *min_y = y;
		else if (y > *max_y)
			*max_y = y;
		ids[i] = i;
	}
	*cx = (*min_x + *max_x) / 2;
	*cy = (*min_y + *max_y) / 2;
}

static OPUS_INLINE opus_real square_dist(opus_real ax, opus_real ay, opus_real bx, opus_real by)
{
	return (ax - bx) * (ax - bx) + (ay - by) * (ay - by);
}

static OPUS_INLINE uint32_t get_closet_point0_(const opus_real *coords, uint32_t n, opus_real x,
                                          opus_real y)
{
	uint32_t i, min_i = -1;

	opus_real min_dist = OPUS_REAL_MAX;
	for (i = 0; i < n; i++) {
		opus_real d = square_dist(x, y, coords[2 * i], coords[2 * i + 1]);
		if (d < min_dist) {
			min_i    = i;
			min_dist = d;
		}
	}
	return min_i;
}

static OPUS_INLINE uint32_t get_closet_point1_(const opus_real *coords, uint32_t n, opus_real x,
                                          opus_real y, uint32_t f)
{
	uint32_t i, min_i = -1;

	opus_real min_dist = OPUS_REAL_MAX;
	for (i = 0; i < n; i++) {
		opus_real d;
		if (i == f) continue;
		d = square_dist(x, y, coords[2 * i], coords[2 * i + 1]);
		if (d < min_dist) {
			min_i    = i;
			min_dist = d;
		}
	}
	return min_i;
}

static OPUS_INLINE uint32_t get_smallest_point_(const opus_real *coords, uint32_t n,
                                           uint32_t a, uint32_t b, opus_real ax, opus_real ay, opus_real bx,
                                           opus_real by, opus_real *min_r)
{
	uint32_t i, min_i;
	opus_real cx, cy, r_sq, min_r_sq = OPUS_REAL_MAX;
	for (i = 0; i < n; i++) {
		if (i == a || i == b) continue;
		opus_triangle_cricumcenter(ax, ay, bx, by, coords[2 * i], coords[2 * i + 1], &cx, &cy,
		                           &r_sq);
		if (r_sq < min_r_sq) {
			min_i    = i;
			min_r_sq = r_sq;
		}
	}

	*min_r = min_r_sq;

	return min_i;
}

static OPUS_INLINE void swap_id_(uint32_t *ids, uint32_t i, uint32_t j)
{
	uint32_t t = ids[i];
	ids[i]     = ids[j];
	ids[j]     = t;
}

static void quicksort_(uint32_t *ids, opus_real *dists, uint32_t left, uint32_t right)
{
	int64_t  i, j;
	uint32_t temp;
	opus_real temp_dist;
	if (right - left <= 20) {
		for (i = left + 1; i <= right; i++) {
			temp      = ids[i];
			temp_dist = dists[temp];
			j         = i - 1;
			while (j >= left && dists[ids[j]] > temp_dist) {
				ids[j + 1] = ids[j];
				j--;
			}
			ids[j + 1] = temp;
		}
	} else {
		uint32_t median;

		median = (left + right) >> 1;
		i      = left + 1;
		j      = right;

		swap_id_(ids, median, i);
		if (dists[ids[left]] > dists[ids[right]]) swap_id_(ids, left, right);
		if (dists[ids[i]] > dists[ids[right]]) swap_id_(ids, i, right);
		if (dists[ids[left]] > dists[ids[i]]) swap_id_(ids, left, i);

		temp      = ids[i];
		temp_dist = dists[temp];
		while (1) {
			do i++;
			while (dists[ids[i]] < temp_dist);
			do j--;
			while (dists[ids[j]] > temp_dist);
			if (j < i) break;
			swap_id_(ids, i, j);
		}
		ids[left + 1] = ids[j];
		ids[j]        = temp;

		if (right - i + 1 >= j - left) {
			quicksort_(ids, dists, i, right);
			quicksort_(ids, dists, left, j - 1);
		} else {
			quicksort_(ids, dists, left, j - 1);
			quicksort_(ids, dists, i, right);
		}
	}
}

static OPUS_INLINE opus_real orient2d_(opus_real x1, opus_real y1, opus_real x2, opus_real y2,
                                  opus_real x3, opus_real y3)
{
	extern double orient2d(double *pa, double *pb, double *pc);
	double        pa[2], pb[2], pc[2];
	pa[0] = x1;
	pa[1] = y1;
	pb[0] = x2;
	pb[1] = y2;
	pc[0] = x3;
	pc[1] = y3;
	return orient2d(pa, pb, pc);
}

static OPUS_INLINE void sort_counter_clock_wise_(opus_real i0x, opus_real i0y, opus_real *i1x,
                                            opus_real *i1y, opus_real *i2x, opus_real *i2y, uint32_t *i1, uint32_t *i2)
{
	if (orient2d_(i0x, i0y, *i1x, *i1y, *i2x, *i2y) < 0) {
		uint32_t i = *i1;
		opus_real     x = *i1x;
		opus_real     y = *i1y;
		*i1        = *i2;
		*i1x       = *i2x;
		*i1y       = *i2y;
		*i2        = i;
		*i2x       = x;
		*i2y       = y;
	}
}

static OPUS_INLINE uint32_t get_hash_key_(opus_real x, opus_real y, opus_real cx, opus_real cy, uint32_t hash_size)
{
	opus_real dx = x - cx, dy = y - cy;
	opus_real p = dx / opus_abs(dx) + opus_abs(dy);
	p      = (dy > 0 ? 3 - p : 1 + p) / 4;
	return (uint32_t) opus_floor(p * hash_size) % hash_size;
}

static OPUS_INLINE void add_linK_(opus_delaunay_data *data, uint32_t a, uint32_t b)
{
	data->half_edges_[a] = b;
	if (b != (uint32_t) -1) data->half_edges_[b] = a;
}

/* add a new triangle given vertex indices and adjacent half-edge ids */
static OPUS_INLINE uint32_t add_triangle_(opus_delaunay_data *data,
                                     uint32_t i0, uint32_t i1, uint32_t i2,
                                     uint32_t a, uint32_t b, uint32_t c)
{
	uint32_t t = data->n_triangles_;

	data->triangles_[t]     = i0;
	data->triangles_[t + 1] = i1;
	data->triangles_[t + 2] = i2;

	add_linK_(data, t, a);
	add_linK_(data, t + 1, b);
	add_linK_(data, t + 2, c);

	data->n_triangles_ += 3;

	return t;
}

static const uint32_t EDGE_STACK_LEN = 512;
static uint32_t       EDGE_STACK[512];

static OPUS_INLINE uint32_t legalize_(opus_delaunay_data *data, uint32_t a)
{
	uint32_t i  = 0;
	uint32_t ar = 0;

	uint32_t *triangles  = data->triangles_;
	uint32_t *half_edges = data->half_edges_;
	opus_real *coords     = data->coords;

	/* recursion eliminated with a fixed-size stack */
	while (1) {
		uint32_t b = data->half_edges_[a];

		uint32_t a0, b0, al, bl, p0, pr, pl, p1;
		int      illegal;

		/* if the pair of triangles doesn't satisfy the Delaunay condition
		 * (p1 is inside the circum-circle of [p0, pl, pr]), flip them,
		 * then do the same check/flip recursively for the new pair of triangles:
		 *
		 *           pl                    pl
		 *          /||\                  /  \
		 *       al/ || \bl            al/    \a
		 *        /  ||  \              /      \
		 *       /  a||b  \    flip    /___ar___\
		 *     p0\   ||   /p1   =>   p0\---bl---/p1
		 *        \  ||  /              \      /
		 *       ar\ || /br             b\    /br
		 *          \||/                  \  /
		 *           pr                    pr
		 */
		a0 = a - a % 3;
		ar = a0 + (a + 2) % 3;

		if (b == -1) { /* convex hull edge */
			if (i == 0) break;
			a = EDGE_STACK[--i];
			continue;
		}

		b0 = b - b % 3;
		al = a0 + (a + 1) % 3;
		bl = b0 + (b + 2) % 3;

		p0 = triangles[ar];
		pr = triangles[a];
		pl = triangles[al];
		p1 = triangles[bl];

		illegal = opus_is_point_in_triangle(coords[2 * p0], coords[2 * p0 + 1], coords[2 * pr],
		                                    coords[2 * pr + 1], coords[2 * pl], coords[2 * pl + 1],
		                                    coords[2 * p1], coords[2 * p1 + 1]);

		if (illegal) {
			uint32_t hbl;
			uint32_t br;

			triangles[a] = p1;
			triangles[b] = p0;

			hbl = half_edges[bl];

			/* edge swapped on the other side of the hull (rare); fix the half edge reference */
			if (hbl == -1) {
				uint32_t e = data->hull_start_;
				do {
					if (data->hull_tri_[e] == bl) {
						data->hull_tri_[e] = a;
						break;
					}
					e = data->hull_prev_[e];
				} while (e != data->hull_start_);
			}
			add_linK_(data, a, hbl);
			add_linK_(data, b, half_edges[ar]);
			add_linK_(data, ar, bl);

			br = b0 + (b + 1) % 3;

			/* don't worry about hitting the cap: it can only happen on extremely degenerate input */
			if (i < EDGE_STACK_LEN) {
				EDGE_STACK[i++] = br;
			}
		} else {
			if (i == 0) break;
			a = EDGE_STACK[--i];
		}
	}

	return ar;
}

void opus_delaunay_triangulate(opus_delaunay_data *data)
{
	opus_real     *coords;
	uint32_t  n;
	uint32_t  hash_size, *hull_prev, *hull_next, *hull_tri;
	uint32_t *hull_hash;
	uint32_t *ids;
	opus_real     *dists;

	opus_real min_x, max_x;
	opus_real min_y, max_y;
	opus_real cx, cy, r_sq;

	uint32_t i0, i1, i2;
	opus_real     i0x, i0y, i1x, i1y, i2x, i2y;
	opus_real     min_r;

	uint32_t h, i, j, k, hull_size;
	uint32_t start, key, e, q, t;

	opus_real x, y, xp, yp;

	if (data->n >> 1 < 3) return;

	coords    = data->coords;
	hash_size = data->hash_size_;
	hull_prev = data->hull_prev_;
	hull_next = data->hull_next_;
	hull_tri  = data->hull_tri_;
	hull_hash = data->hull_hash_;
	ids       = data->ids_;
	dists     = data->dists_;

	/* update the ids of the coords in the meantime */
	get_bounding_box_(coords, data->n >> 1, ids, &min_x, &min_y, &max_x, &max_y, &cx, &cy);

	/* get the seed point closest to the center of the bounding box */
	i0  = get_closet_point0_(coords, data->n >> 1, cx, cy);
	i0x = coords[2 * i0];
	i0y = coords[2 * i0 + 1];

	/* get the point closest to the seed */
	i1  = get_closet_point1_(coords, data->n >> 1, i0x, i0y, i0);
	i1x = coords[2 * i1];
	i1y = coords[2 * i1 + 1];

	OPUS_ASSERT(i0 != i1);

	/* find the third point which forms the smallest circum-circle with the first two */
	i2  = get_smallest_point_(coords, data->n >> 1, i0, i1, i0x, i0y, i1x, i1y, &min_r);
	i2x = coords[2 * i2];
	i2y = coords[2 * i2 + 1];

	if (min_r == OPUS_REAL_MAX) {
		uint32_t *hull;
		opus_real d;

		/*
		 * order collinear points by dx (or dy if all x are identical)
		 * and return the list as a hull
		 */
		for (i = 0; i < data->n >> 1; i++)
			dists[i] = (opus_real) ((coords[2 * i] - coords[0]) || (coords[2 * i + 1] - coords[1]));

		quicksort_(ids, dists, 0, (data->n >> 1) - 1);
		hull = (uint32_t *) malloc(sizeof(uint32_t) * (data->n >> 1));
		j    = 0;
		for (i = 0, d = -OPUS_REAL_MAX; i < data->n >> 1; i++) {
			uint32_t id = ids[i];
			if (dists[id] > d) {
				hull[j] = id;
				d       = dists[id];
				j++;
			}
		}
		/* actually the memory space after "j" will never be used */
		data->hull_         = hull;
		data->n_hull_       = j;
		data->n_triangles_  = 0; /* no triangles generated */
		data->n_half_edges_ = 0;
		return;
	}

	/* swap the order of the seed points for counter-clockwise orientation */
	sort_counter_clock_wise_(i0x, i0y, &i1x, &i1y, &i2x, &i2y, &i1, &i2);

	opus_triangle_cricumcenter(i0x, i0y, i1x, i1y, i2x, i2y, &cx, &cy, &r_sq);

	for (i = 0; i < data->n >> 1; i++)
		dists[i] = square_dist(coords[2 * i], coords[2 * i + 1], cx, cy);

	/* sort the points by distance from the seed triangle circum-center */
	quicksort_(ids, dists, 0, (data->n >> 1) - 1);

	/* set up the seed triangle as the starting hull */
	data->hull_start_ = i0;
	hull_size         = 3;

	hull_next[i0] = hull_prev[i2] = i1;
	hull_next[i1] = hull_prev[i0] = i2;
	hull_next[i2] = hull_prev[i1] = i0;

	hull_tri[i0] = 0;
	hull_tri[i1] = 1;
	hull_tri[i2] = 2;

	for (i = 0; i < hash_size; i++) hull_hash[i] = -1;
	hull_hash[get_hash_key_(i0x, i0y, cx, cy, hash_size)] = i0;
	hull_hash[get_hash_key_(i1x, i1y, cx, cy, hash_size)] = i1;
	hull_hash[get_hash_key_(i2x, i2y, cx, cy, hash_size)] = i2;

	data->n_triangles_ = 0;
	add_triangle_(data, i0, i1, i2, -1, -1, -1);

	xp = 0;
	yp = 0;
	for (k = 0; k < data->n >> 1; k++) {
		i = ids[k];
		x = coords[2 * i];
		y = coords[2 * i + 1];

		/* skip near-duplicate points */
		if (k > 0 && opus_abs(x - xp) <= DELAUNAY_EPSILON && opus_abs(y - yp) <= DELAUNAY_EPSILON) continue;
		xp = x;
		yp = y;

		/* skip seed triangle points */
		if (i == i0 || i == i1 || i == i2) continue;

		/* find a visible edge on the convex hull using edge hash */
		start = 0;
		key   = get_hash_key_(x, y, cx, cy, hash_size);
		for (j = 0; j < hash_size; j++) {
			start = hull_hash[(key + j) % hash_size];
			if (start != -1 && start != hull_next[start]) break;
		}

		start = hull_prev[start];
		e     = start;
		while (q = hull_next[e], orient2d_(x, y, coords[2 * e], coords[2 * e + 1], coords[2 * q], coords[2 * q + 1]) >= 0) {
			e = q;
			if (e == start) {
				e = -1;
				break;
			}
		}
		if (e == -1) continue; /* likely a near-duplicate point; skip it */

		/* add the first triangle from the point */
		t = add_triangle_(data, e, i, hull_next[e], -1, -1, hull_tri[e]);

		/* recursively flip triangles from the point until they satisfy the Delaunay condition */
		hull_tri[i] = legalize_(data, t + 2);
		hull_tri[e] = t; /* keep track of boundary triangles on the hull */
		hull_size++;

		/* walk forward through the hull, adding more triangles and flipping recursively */
		n = hull_next[e];
		while (q = hull_next[n], orient2d_(x, y, coords[2 * n], coords[2 * n + 1], coords[2 * q], coords[2 * q + 1]) < 0) {
			t            = add_triangle_(data, n, i, q, hull_tri[i], -1, hull_tri[n]);
			hull_tri[i]  = legalize_(data, t + 2);
			hull_next[n] = n; /* mark as removed */
			hull_size--;
			n = q;
		}

		/* walk backward from the other side, adding more triangles and flipping */
		if (e == start) {
			while (q = hull_prev[e], orient2d_(x, y, coords[2 * q], coords[2 * q + 1], coords[2 * e], coords[2 * e + 1]) < 0) {
				t = add_triangle_(data, q, i, e, -1, hull_tri[e], hull_tri[q]);
				legalize_(data, t + 2);
				hull_tri[q]  = t;
				hull_next[e] = e; /* mark as removed */
				hull_size--;
				e = q;
			}
		}

		/* update the hull indices */
		data->hull_start_ = hull_prev[i] = e;
		hull_next[e] = hull_prev[n] = i;
		hull_next[i]                = n;

		/* save the two new edges in the hash table */
		hull_hash[get_hash_key_(x, y, cx, cy, hash_size)]                             = i;
		hull_hash[get_hash_key_(coords[2 * e], coords[2 * e + 1], cx, cy, hash_size)] = e;
	}

	data->hull_   = (uint32_t *) malloc(sizeof(uint32_t) * hull_size);
	data->n_hull_ = hull_size;
	for (i = 0, e = data->hull_start_; i < hull_size; i++) {
		data->hull_[i] = e;

		e = hull_next[e];
	}
}

void opus_delaunay_done(opus_delaunay_data *data)
{
	if (data) {
		free(data->triangles_);
		free(data->half_edges_);
		free(data->hull_prev_);
		free(data->hull_next_);
		free(data->hull_tri_);
		free(data->hull_hash_);
		free(data->ids_);
		free(data->dists_);

		if (data->hull_) free(data->hull_);
	}
}
