#include "spline.h"
#include "data_structure/array.h"
#include "utils/utils.h"
#include "math/math.h"
#include <stdlib.h>


/**
 * @brief B-spline interpolation of control points of any dimensionality using de Boor's algorithm(http://en.wikipedia.org/wiki/De_Boor%27s_algorithm).
 * @development_log I am learning how does it work ... This version is adapted from https://github.com/thibauts/b-spline
 * @param t
 * @param degree
 * @param points
 * @param knots if NULL, all elements(n - degree + 1 total) will be knots[i] == i respectively
 * @param weights if NULL, all elements(n total) will be 1
 * @param result a point which divides the interpolated curve by `t`
 * @return 1 if error occurs, otherwise 0
 */
int opus_interpolate_b_spline(opus_real t, int degree, opus_vec2 *points, const opus_real *knots, const opus_real *weights, opus_vec2 *result)
{
	int    i, j, s, l;                   /*  function-scoped iteration variables */
	int    n        = opus_arr_len(points); /*  points count */
	int    d        = 2;                 /*  point dimensionality */
	opus_real  *weights_ = NULL;
	opus_real  *knots_   = NULL;
	int    domain[2];
	opus_real   low, high;
	opus_real **v = NULL;
	opus_real   alpha;


	weights_ = (opus_real *) OPUS_MALLOC(n * sizeof(opus_real));
	knots_   = (opus_real *) OPUS_MALLOC((n + degree + 1) * sizeof(opus_real));
	if (weights_ == NULL || knots_ == NULL) return 1;

	if (degree < 1) return 1;                                          /*  degree must be at least 1 (linear) */
	if (degree > (n - 1)) return 1;                                    /*  degree must be less than or equal to point count - 1 */
	if (knots != NULL && opus_arr_len(knots) != n + degree + 1) return 1; /*  bad knot vector length */

	if (weights == NULL) {
		weights = weights_;
		for (i = 0; i < n; i++)
			weights_[i] = 1.f;
	}
	if (knots == NULL) {
		knots = knots_;
		for (i = 0; i < n + degree + 1; i++) knots_[i] = (opus_real) i;
	}

	domain[0] = degree;
	domain[1] = n;

	/*  remap t to the domain where the spline is defined */
	low  = knots[domain[0]];
	high = knots[domain[1]];
	t    = t * (high - low) + low;

	if (t < low || t > high) return 1; /*  out of bounds */

	/*  find s (the spline segment) for the [t] value provided */
	for (s = domain[0]; s < domain[1]; s++) {
		if (t >= knots[s] && t <= knots[s + 1])
			break;
	}

	/*  convert points to homogeneous coordinates */
	v = (opus_real **) OPUS_MALLOC(sizeof(opus_real) * (n * (d + 1)));
	if (v == NULL) return 1;
	for (i = 0; i < n; i++) {
		for (j = 0; j < d; j++) {
			opus_real *point = *((opus_real **) &points[i]);
			v[i][j]     = point[j] * weights[i];
		}
		v[i][d] = weights[i];
	}

	/*  l (level) goes from 1 to the curve degree + 1 */
	for (l = 1; l <= degree + 1; l++) {
		/*  build level l of the pyramid */
		for (i = s; i > s - degree - 1 + l; i--) {
			alpha = (t - knots[i]) / (knots[i + degree + 1 - l] - knots[i]);

			/*  interpolate each component */
			for (j = 0; j < d + 1; j++) {
				v[i][j] = (1 - alpha) * v[i - 1][j] + alpha * v[i][j];
			}
		}
	}

	/* convert back to cartesian and return */
	for (i = 0; i < d; i++) {
		((opus_real *) result)[i] = v[s][i] / v[s][d];
	}

	return 0;
}

OPUS_INLINE void spline_calc_diff(opus_real *arr, int n, opus_real *diff)
{
	size_t i;
	for (i = 0; i < n; i++) diff[i] = arr[i + 1] - arr[i];
}

void opus_spline_init(opus_spline_t *spline, opus_real *x_list, opus_real *y_list, int n)
{
#define A__(i, j) (A[(i) *n + (j)])
	size_t i;
	opus_real *h, *A, *B, *c;

	spline->x = (opus_real *) OPUS_MALLOC(sizeof(opus_real) * n);
	spline->y = (opus_real *) OPUS_MALLOC(sizeof(opus_real) * n);
	memcpy(spline->x, x_list, n * sizeof(opus_real));
	memcpy(spline->y, y_list, n * sizeof(opus_real));
	spline->n = n;
	spline->n = n;

	h = (opus_real *) OPUS_MALLOC(sizeof(opus_real) * (n - 1));
	spline_calc_diff(x_list, n - 1, h);

	spline->a = (opus_real *) OPUS_MALLOC(sizeof(opus_real) * n);
	memcpy(spline->a, y_list, sizeof(opus_real) * n);

	A = (opus_real *) OPUS_MALLOC(sizeof(opus_real) * n * n);
	memset(A, 0, sizeof(opus_real) * n * n);

	A__(0, 0) = 1.0;
	for (i = 0; i < n - 1; i++) {
		if (i != n - 2)
			A__(i + 1, i + 1) = 2.f * (h[i] + h[i + 1]);
		A__(i + 1, i) = h[i];
		A__(i, i + 1) = h[i];
	}
	A__(0, 1)         = 0.f;
	A__(n - 1, n - 2) = 0.f;
	A__(n - 1, n - 1) = 1.f;

	B = (opus_real *) OPUS_MALLOC(sizeof(opus_real) * n);
	memset(B, 0, sizeof(opus_real) * n);
	for (i = 0; i < n - 2; i++)
		B[i + 1] = 3.f * (y_list[i + 2] - y_list[i + 1]) / h[i + 1] - 3.0 * (y_list[i + 1] - y_list[i]) / h[i];

	spline->c = (opus_real *) OPUS_MALLOC(sizeof(opus_real) * n);
	c         = (opus_real *) OPUS_MALLOC(sizeof(opus_real) * n);
	opus_linear_solve_qr((opus_real *) A, c, B, n, n);
	for (i = 0; i < n; i++) {
		spline->c[i] = c[i];
	}

	spline->b = (opus_real *) OPUS_MALLOC(sizeof(opus_real) * (n - 1));
	spline->d = (opus_real *) OPUS_MALLOC(sizeof(opus_real) * (n - 1));
	for (i = 0; i < n - 1; i++) {
		opus_real tb;
		spline->d[i] = (spline->c[i + 1] - spline->c[i]) / (3.0 * h[i]);
		tb           = (spline->a[i + 1] - spline->a[i]) / h[i] - h[i] * (spline->c[i + 1] + 2.0 * spline->c[i]) / 3.0;
		spline->b[i] = tb;
	}

	OPUS_FREE(h);
	OPUS_FREE(A);
	OPUS_FREE(B);
	OPUS_FREE(c);
#undef A__
}

void opus_spline_done(opus_spline_t *spline)
{
	OPUS_FREE(spline->a);
	OPUS_FREE(spline->x);
	OPUS_FREE(spline->y);
	OPUS_FREE(spline->c);
	OPUS_FREE(spline->b);
	OPUS_FREE(spline->d);
}

OPUS_INLINE size_t __spline_search_index(opus_spline_t *spline, opus_real t)
{
	size_t i;
	for (i = 0; i < spline->n; i++) {
		if (spline->x[i] == t) return i + 1;
		if (spline->x[i] > t) {
			return i;
		}
	}

	return i;
}

#define __SPLINE_CALC_DERIVATIVE_INIT                                     \
	size_t i;                                                          \
	opus_real dx;                                                            \
	if (t < spline->x[0] || t > spline->x[spline->n - 1]) return DBL_MAX; \
	i  = __spline_search_index(spline, t) - 1;                            \
	dx = t - spline->x[i];

/**
 * calculate position
 * if t is outside of the input x, return DBL_MAX
 */
opus_real opus_spline_calc_xt(opus_spline_t *spline, opus_real t)
{
	__SPLINE_CALC_DERIVATIVE_INIT
	return spline->a[i] + spline->b[i] * dx + spline->c[i] * dx * dx + spline->d[i] * dx * dx * dx;
}

/**
 * calculate first derivative
 * if t is outside of the input x, return DBL_MAX
 */
opus_real opus_spline_calc_dxt(opus_spline_t *spline, opus_real t)
{
	__SPLINE_CALC_DERIVATIVE_INIT
	return spline->b[i] + 2.0 * spline->c[i] * dx + 3.0 * spline->d[i] * dx * dx;
}

/**
 * calculate second derivative
 * if t is outside of the input x, return DBL_MAX
 */
opus_real opus_spline_calc_ddxt(opus_spline_t *spline, opus_real t)
{
	__SPLINE_CALC_DERIVATIVE_INIT
	return 2.0 * spline->c[i] + 6.0 * spline->d[i] * dx;
}

#undef __SPLINE_CALC_DERIVATIVE_INIT

void opus_spline2d_init(opus_spline2d_t *spline, opus_real *x_list, opus_real *y_list, int n)
{
	size_t i;
	opus_real *dx, *dy;
	spline->n = n;

	dx = (opus_real *) OPUS_MALLOC(sizeof(opus_real) * (n - 1));
	dy = (opus_real *) OPUS_MALLOC(sizeof(opus_real) * (n - 1));
	spline_calc_diff(x_list, n - 1, dx);
	spline_calc_diff(y_list, n - 1, dy);

	spline->ds = (opus_real *) OPUS_MALLOC(sizeof(opus_real) * (n - 1));
	for (i = 0; i < n - 1; i++) spline->ds[i] = opus_hypot(dx[i], dy[i]);

	spline->s    = (opus_real *) OPUS_MALLOC(sizeof(opus_real) * n);
	spline->s[0] = 0;
	for (i = 0; i < n - 1; i++) spline->s[i + 1] = spline->ds[i] + spline->s[i];

	opus_spline_init(&spline->sx, spline->s, x_list, n);
	opus_spline_init(&spline->sy, spline->s, y_list, n);

	OPUS_FREE(dx);
	OPUS_FREE(dy);
}

void opus_spline2d_done(opus_spline2d_t *spline)
{
	opus_spline_done(&spline->sx);
	opus_spline_done(&spline->sy);
	OPUS_FREE(spline->ds);
	OPUS_FREE(spline->s);
}

void opus_spline2d_calc_position(opus_spline2d_t *spline, opus_real s, opus_vec2 *pos)
{
	pos->x = opus_spline_calc_xt(&spline->sx, s);
	pos->y = opus_spline_calc_xt(&spline->sy, s);
}

opus_real opus_spline2d_calc_curvature(opus_spline2d_t *spline, opus_real s)
{
	opus_real dx  = opus_spline_calc_dxt(&spline->sx, s);
	opus_real ddx = opus_spline_calc_ddxt(&spline->sx, s);
	opus_real dy  = opus_spline_calc_dxt(&spline->sy, s);
	opus_real ddy = opus_spline_calc_ddxt(&spline->sy, s);
	return (ddy * dx - ddx * dy) / (dx * dx + dy * dy);
}

opus_real opus_spline2d_calc_yaw(opus_spline2d_t *spline, opus_real s)
{
	return atan2(opus_spline_calc_dxt(&spline->sx, s), opus_spline_calc_dxt(&spline->sy, s));
}
