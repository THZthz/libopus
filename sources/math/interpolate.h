/**
 * @file interpolate.h
 *
 * Author:        _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/4/6 13:42
 *
 * @brief generate cubic spline and b-spline ?
 *
 * @example
 * 		int        i;
 * 		real       x[7] = {-2.5, 0.0, 2.5, 5.0, 7.5, 3.0, -1.0};
 * 		real       y[7] = {0.7, -6, 5, 6.5, 0.0, 5.0, -2.0};
 * 		spline2d_t sp;
 * 		vec2       prev, pos;
 * 		real       s;
 *
 * 		for (i = 0; i < 7; i++) {
 * 			x[i] *= 14;
 * 			x[i] += 100;
 * 		}
 * 		for (i = 0; i < 7; i++) {
 * 			y[i] *= 14;
 * 			y[i] += 100;
 * 		}
 *
 * 		spline2d_init(&sp, x, y, 7);
 * 		for (i = 0; i < 7; i++) {
 * 			plutovg_circle(vg, x[i], y[i], 3);
 * 			plutovg_fill(vg);
 * 		}
 *
 * 		for (s = 0.0; s < sp.s[sp.n - 1];) {
 * 			spline2d_calc_position(&sp, s, &pos);
 * 			if (s > 0.0) {
 * 				plutovg_move_to(vg, prev.x, prev.y);
 * 				plutovg_line_to(vg, pos.x, pos.y);
 * 			}
 * 			vec2_dup(prev, pos);
 * 			s += 0.1;
 * 		}
 * 		plutovg_stroke(vg);
 * 		spline2d_done(&sp);
 */
#ifndef INTERPOLATE_H
#define INTERPOLATE_H

#include "math/math.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* one-dimensional interpolation */
typedef struct spline
{
	real *x, *y;
	real *a, *b, *c, *d, *w;
	int   n;
} spline_t;
/* can be used in 2d plane */
typedef struct spline2d
{
	real     *x, *y, *s, *ds;
	spline_t  sx, sy;
	int       n;
} spline2d_t;

void spline_init(spline_t *spline, real *x_list, real *y_list, int n);
void spline_done(spline_t *spline);
real spline_calc_xt(spline_t *spline, real t);
real spline_calc_dxt(spline_t *spline, real t);
real spline_calc_ddxt(spline_t *spline, real t);

void spline2d_init(spline2d_t *spline, real *x_list, real *y_list, int n);
void spline2d_done(spline2d_t *spline);
void spline2d_calc_position(spline2d_t *spline, real s, vec2 *pos);
real spline2d_calc_curvature(spline2d_t *spline, real s);
real spline2d_calc_yaw(spline2d_t *spline, real s);

int interpolate_b_spline(real t, int degree, vec2 *points, const real *knots, const real *weights, vec2 *result);
#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* INTERPOLATE_H */
