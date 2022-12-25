/**
 * @file polynomial.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/8/24
 *
 * @example
 *
 * @development_log
 *
 */

#include "math/polynomial.h"


real quartic_calc_xt(polynomial_quartic_t *path, real t)
{
	return path->a0 + path->a1 * t + path->a2 * t * t + path->a3 * t * t * t + path->a4 * t * t * t * t;
}

real quartic_calc_dxt(polynomial_quartic_t *path, real t)
{
	return path->a1 + 2 * path->a2 * t + 3 * path->a3 * t * t + 4 * path->a4 * t * t * t;
}

real quartic_calc_ddxt(polynomial_quartic_t *path, real t)
{
	return 2 * path->a2 + 6 * path->a3 * t + 12 * path->a4 * t * t;
}

real quartic_calc_dddxt(polynomial_quartic_t *path, real t)
{
	return 6 * path->a3 + 24 * path->a4 * t;
}

real quintic_calc_xt(polynomial_quintic_t *path, real t)
{
	return path->a0 + path->a1 * t + path->a2 * t * t + path->a3 * t * t * t + path->a4 * t * t * t * t + path->a5 * t * t * t * t * t;
}

real quintic_calc_dxt(polynomial_quintic_t *path, real t)
{
	return path->a1 + 2 * path->a2 * t + 3 * path->a3 * t * t + 4 * path->a4 * t * t * t + 5 * path->a5 * t * t * t * t;
}

real quintic_calc_ddxt(polynomial_quintic_t *path, real t)
{
	return 2 * path->a2 + 6 * path->a3 * t + 12 * path->a4 * t * t + 20 * path->a5 * t * t * t;
}

real quintic_calc_dddxt(polynomial_quintic_t *path, real t)
{
	return 6 * path->a3 + 24 * path->a4 * t + 60 * path->a5 * t * t;
}

void quartic_polynomial(polynomial_quartic_t *path, real x0, real v0, real a0, real v1, real a1, real T)
{
	real a = 3 * T * T, b = 4 * T * T * T, c = 6 * T, d = 12 * T * T, e = v1 - v0 - a0 * T, f = a1 - a0, mat = a * d - b * c;
	path->a0 = x0;
	path->a1 = v0;
	path->a2 = a0 / 2.0;
	path->a3 = (e * d - b * f) / mat;
	path->a4 = (a * f - e * c) / mat;
}

void quintic_polynomial(polynomial_quintic_t *path, real x0, real v0, real a0, real x1, real v1, real a1, real T)
{
	real a = T * T * T, b = T * T * T * T, c = T * T * T * T * T,
	       d = 3 * T * T, e = 4 * T * T * T, f = 5 * T * T * T * T,
	       g = 6 * T, h = 12 * T * T, i = 20 * T * T * T;
	real j   = x1 - x0 - v0 * T - a0 * T * T / 2.0,
	       k   = v1 - v0 - a0 * T,
	       l   = a1 - a0;
	real mat = __val_of_mat3x3(a, b, c, d, e, f, g, h, i);

	path->a0 = x0;
	path->a1 = v0;
	path->a2 = a0 / 2.0;
	path->a3 = __val_of_mat3x3(j, b, c, k, e, f, l, h, i) / mat;
	path->a4 = __val_of_mat3x3(a, j, c, d, k, f, g, l, i) / mat;
	path->a5 = __val_of_mat3x3(a, b, j, d, e, k, g, h, l) / mat;
}

