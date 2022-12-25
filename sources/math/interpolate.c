#include "math/interpolate.h"
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
int interpolate_b_spline(real t, int degree, vec2 *points, const real *knots, const real *weights, vec2 *result)
{
	int    i, j, s, l;                   /*  function-scoped iteration variables */
	int    n        = array_len(points); /*  points count */
	int    d        = 2;                 /*  point dimensionality */
	real  *weights_ = NULL;
	real  *knots_   = NULL;
	int    domain[2];
	real   low, high;
	real **v = NULL;
	real   alpha;


	weights_ = (real *) malloc(n * sizeof(real));
	knots_   = (real *) malloc((n + degree + 1) * sizeof(real));
	if (weights_ == NULL || knots_ == NULL) return 1;

	if (degree < 1) return 1;                                          /*  degree must be at least 1 (linear) */
	if (degree > (n - 1)) return 1;                                    /*  degree must be less than or equal to point count - 1 */
	if (knots != NULL && array_len(knots) != n + degree + 1) return 1; /*  bad knot vector length */

	if (weights == NULL) {
		weights = weights_;
		for (i = 0; i < n; i++)
			weights_[i] = 1.f;
	}
	if (knots == NULL) {
		knots = knots_;
		for (i = 0; i < n + degree + 1; i++) knots_[i] = (real) i;
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
	v = (real **) malloc(sizeof(real) * (n * (d + 1)));
	if (v == NULL) return 1;
	for (i = 0; i < n; i++) {
		for (j = 0; j < d; j++) {
			real *point = *((real **) &points[i]);
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
		((real *) result)[i] = v[s][i] / v[s][d];
	}

	return 0;
}

INLINE void spline_calc_diff(real *arr, int n, real *diff)
{
	size_t i;
	for (i = 0; i < n; i++) diff[i] = arr[i + 1] - arr[i];
}

void spline_init(spline_t *spline, real *x_list, real *y_list, int n)
{
#define A__(i, j) (A[(i) *n + (j)])
	size_t i;
	real  *h, *A, *B, *c;

	spline->x = (real *) malloc(sizeof(real) * n);
	spline->y = (real *) malloc(sizeof(real) * n);
	memcpy(spline->x, x_list, n * sizeof(real));
	memcpy(spline->y, y_list, n * sizeof(real));
	spline->n = n;
	spline->n = n;

	h = (real *) malloc(sizeof(real) * (n - 1));
	spline_calc_diff(x_list, n - 1, h);

	spline->a = (real *) malloc(sizeof(real) * n);
	memcpy(spline->a, y_list, sizeof(real) * n);

	A = (real *) malloc(sizeof(real) * n * n);
	memset(A, 0, sizeof(real) * n * n);

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

	B = (real *) malloc(sizeof(real) * n);
	memset(B, 0, sizeof(real) * n);
	for (i = 0; i < n - 2; i++)
		B[i + 1] = 3.f * (y_list[i + 2] - y_list[i + 1]) / h[i + 1] - 3.0 * (y_list[i + 1] - y_list[i]) / h[i];

	spline->c = (real *) malloc(sizeof(real) * n);
	c         = (real *) malloc(sizeof(real) * n);
	linear_solve_qr((real *) A, c, B, n, n);
	for (i = 0; i < n; i++) {
		spline->c[i] = c[i];
	}

	spline->b = (real *) malloc(sizeof(real) * (n - 1));
	spline->d = (real *) malloc(sizeof(real) * (n - 1));
	for (i = 0; i < n - 1; i++) {
		real tb;
		spline->d[i] = (spline->c[i + 1] - spline->c[i]) / (3.0 * h[i]);
		tb           = (spline->a[i + 1] - spline->a[i]) / h[i] - h[i] * (spline->c[i + 1] + 2.0 * spline->c[i]) / 3.0;
		spline->b[i] = tb;
	}

	free(h);
	free(A);
	free(B);
	free(c);
#undef A__
}

void spline_done(spline_t *spline)
{
	free(spline->a);
	free(spline->x);
	free(spline->y);
	free(spline->c);
	free(spline->b);
	free(spline->d);
}

INLINE size_t __spline_search_index(spline_t *spline, real t)
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
	size_t i;                                                             \
	real   dx;                                                            \
	if (t < spline->x[0] || t > spline->x[spline->n - 1]) return DBL_MAX; \
	i  = __spline_search_index(spline, t) - 1;                            \
	dx = t - spline->x[i];

/**
 * calculate position
 * if t is outside of the input x, return DBL_MAX
 */
real spline_calc_xt(spline_t *spline, real t)
{
	__SPLINE_CALC_DERIVATIVE_INIT
	return spline->a[i] + spline->b[i] * dx + spline->c[i] * dx * dx + spline->d[i] * dx * dx * dx;
}

/**
 * calculate first derivative
 * if t is outside of the input x, return DBL_MAX
 */
real spline_calc_dxt(spline_t *spline, real t)
{
	__SPLINE_CALC_DERIVATIVE_INIT
	return spline->b[i] + 2.0 * spline->c[i] * dx + 3.0 * spline->d[i] * dx * dx;
}

/**
 * calculate second derivative
 * if t is outside of the input x, return DBL_MAX
 */
real spline_calc_ddxt(spline_t *spline, real t)
{
	__SPLINE_CALC_DERIVATIVE_INIT
	return 2.0 * spline->c[i] + 6.0 * spline->d[i] * dx;
}

#undef __SPLINE_CALC_DERIVATIVE_INIT

void spline2d_init(spline2d_t *spline, real *x_list, real *y_list, int n)
{
	size_t i;
	real  *dx, *dy;
	spline->n = n;

	dx = (real *) malloc(sizeof(real) * (n - 1));
	dy = (real *) malloc(sizeof(real) * (n - 1));
	spline_calc_diff(x_list, n - 1, dx);
	spline_calc_diff(y_list, n - 1, dy);

	spline->ds = (real *) malloc(sizeof(real) * (n - 1));
	for (i = 0; i < n - 1; i++) spline->ds[i] = r_hypot(dx[i], dy[i]);

	spline->s    = (real *) malloc(sizeof(real) * n);
	spline->s[0] = 0;
	for (i = 0; i < n - 1; i++) spline->s[i + 1] = spline->ds[i] + spline->s[i];

	spline_init(&spline->sx, spline->s, x_list, n);
	spline_init(&spline->sy, spline->s, y_list, n);

	free(dx);
	free(dy);
}

void spline2d_done(spline2d_t *spline)
{
	spline_done(&spline->sx);
	spline_done(&spline->sy);
	free(spline->ds);
	free(spline->s);
}

void spline2d_calc_position(spline2d_t *spline, real s, vec2 *pos)
{
	pos->x = spline_calc_xt(&spline->sx, s);
	pos->y = spline_calc_xt(&spline->sy, s);
}

real spline2d_calc_curvature(spline2d_t *spline, real s)
{
	real dx  = spline_calc_dxt(&spline->sx, s);
	real ddx = spline_calc_ddxt(&spline->sx, s);
	real dy  = spline_calc_dxt(&spline->sy, s);
	real ddy = spline_calc_ddxt(&spline->sy, s);
	return (ddy * dx - ddx * dy) / (dx * dx + dy * dy);
}

real spline2d_calc_yaw(spline2d_t *spline, real s)
{
	return atan2(spline_calc_dxt(&spline->sx, s), spline_calc_dxt(&spline->sy, s));
}
