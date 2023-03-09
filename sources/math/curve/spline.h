/**
 * @file interpolate.h
 *
 * Author:        _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/4/6 13:42
 *
 * @brief generate cubic spline and b-spline
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
 * 		opus_spline2d_init(&sp, x, y, 7);
 * 		for (i = 0; i < 7; i++) {
 * 			plutovg_circle(vg, x[i], y[i], 3);
 * 			plutovg_fill(vg);
 * 		}
 *
 * 		for (s = 0.0; s < sp.s[sp.n - 1];) {
 * 			opus_spline2d_calc_position(&sp, s, &pos);
 * 			if (s > 0.0) {
 * 				plutovg_move_to(vg, prev.x, prev.y);
 * 				plutovg_line_to(vg, pos.x, pos.y);
 * 			}
 * 			vec2_dup(prev, pos);
 * 			s += 0.1;
 * 		}
 * 		plutovg_stroke(vg);
 * 		opus_spline2d_done(&sp);
 */
#ifndef SPLINE_H
#define SPLINE_H

#include "math/math.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* one-dimensional interpolation */
typedef struct opus_spline {
	opus_real *x, *y;
	opus_real *a, *b, *c, *d, *w;
	int        n;
} opus_spline_t;
/* can be used in 2d plane */
typedef struct opus_spline2d {
	opus_real    *x, *y, *s, *ds;
	opus_spline_t sx, sy;
	int           n;
} opus_spline2d_t;

void      opus_spline_init(opus_spline_t *spline, opus_real *x_list, opus_real *y_list, int n);
void      opus_spline_done(opus_spline_t *spline);
opus_real opus_spline_calc_xt(opus_spline_t *spline, opus_real t);
opus_real opus_spline_calc_dxt(opus_spline_t *spline, opus_real t);
opus_real opus_spline_calc_ddxt(opus_spline_t *spline, opus_real t);

void      opus_spline2d_init(opus_spline2d_t *spline, opus_real *x_list, opus_real *y_list, int n);
void      opus_spline2d_done(opus_spline2d_t *spline);
void      opus_spline2d_calc_position(opus_spline2d_t *spline, opus_real s, opus_vec2 *pos);
opus_real opus_spline2d_calc_curvature(opus_spline2d_t *spline, opus_real s);
opus_real opus_spline2d_calc_yaw(opus_spline2d_t *spline, opus_real s);

int opus_interpolate_b_spline(opus_real t, int degree, opus_vec2 *points, const opus_real *knots, const opus_real *weights, opus_vec2 *result);
#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* SPLINE_H */
