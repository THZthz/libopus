/**
* @file pathfinding_polynomial.h
* Author:        _              _
*               / \   _ __ ___ (_)  __ _ ___
*              / _ \ | '_ ` _ \| |/ _` / __|
*             / ___ \| | | | | | | (_| \__ \
*            /_/   \_\_| |_| |_|_|\__,_|___/  2022/4/5 8:48
*
* @example
 * int  i;
 * real a = 300;
 * real MAX_ACCEL = 1.0;
 * real MAX_JERK  = 0.5;
 * real dt        = 0.1;
 *
 * real MIN_T  = 5;
 * real MAX_T  = 105;
 * real T_STEP = 10;
 *
 * real sx = a + 10.0, sy = a + 10.0, syaw = core_deg2rad(10), sv = 1, sa = 0.1;
 * real gx = a + 130.0, gy = a + -110.0, gyaw = 3.14, gv = 1, ga = 0.1;
 *
 * real sv_x = sv * cos(syaw);
 * real sv_y = sv * sin(syaw);
 * real gv_x = gv * cos(gyaw);
 * real gv_y = gv * sin(gyaw);
 *
 * real sa_x = sa * cos(syaw);
 * real sa_y = sa * sin(syaw);
 * real ga_x = ga * cos(gyaw);
 * real ga_y = ga * sin(gyaw);
 *
 * polynomial_quintic_t path_x, path_y;
 *
 * struct Po {
 * 	real x, y, v, vx, vy, a, ax, ay, j, jx, jy, t;
 * };
 *
 * struct Po *path;
 * real       max_a = -DBL_MAX, max_j = -DBL_MAX;
 * real       T, t;
 * array_create(path, sizeof(struct Po));
 *
 * for (T = MIN_T; T < MAX_T; T += T_STEP) {
 * 	array_clear(path);
 * 	quintic_polynomial(&path_x, sx, sv_x, sa_x, gx, gv_x, ga_x, T);
 * 	quintic_polynomial(&path_y, sy, sv_y, sa_y, gy, gv_y, ga_y, T);
 *
 * 	for (t = 0.0; t < T + dt; t += dt) {
 * 		struct Po p;
 * 		p.t = t;
 * 		p.x = quintic_calc_xt(&path_x, t);
 * 		p.y = quintic_calc_xt(&path_y, t);
 *
 * 		p.vx = quintic_calc_dxt(&path_x, t);
 * 		p.vy = quintic_calc_dxt(&path_y, t);
 * 		p.v  = core_hypot(p.vx, p.vy);
 *
 * 		p.ax = quintic_calc_ddxt(&path_x, t);
 * 		p.ay = quintic_calc_ddxt(&path_y, t);
 * 		p.a  = core_hypot(p.ax, p.ay);
 * 		if (p.a > max_a) max_a = p.a;
 * 		if (array_len(path) >= 2 && path[array_len(path) - 1].v < path[array_len(path) - 2].v) {
 * 			p.a *= -1;
 * 		}
 *
 * 		p.jx = quintic_calc_dddxt(&path_x, t);
 * 		p.jy = quintic_calc_dddxt(&path_y, t);
 * 		p.j  = core_hypot(p.jx, p.jy);
 * 		if (p.j > max_j) max_j = p.j;
 * 		if (array_len(path) >= 2 && path[array_len(path) - 1].a < path[array_len(path) - 2].a)
 * 			p.j *= -1;
 *
 * 		array_push(path, &p);
 * 	}
 *
 * 	if (max_a <= MAX_ACCEL && max_j <= MAX_JERK) break;
 * }
 * memset(eng->data, 255, 800 * 800 * 4);
 *
 * for (i = 0; i < array_len(path); i++) {
 * 	plutovg_circle(vg, path[i].x, path[i].y, 1);
 * 	plutovg_fill(vg);
 * }
 * array_destroy(path);
 */
#ifndef POLYNOMIAL_H
#define POLYNOMIAL_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "math/math.h"
#include "utils/utils.h"

#define __val_of_mat3x3(a, b, c, d, e, f, g, h, i) ((a) * (e) * (i) + (b) * (f) * (g) + (c) * (d) * (h) - (c) * (e) * (g) - (b) * (d) * (i) - (a) * (f) * (h))

typedef struct
{
	real a0, a1, a2, a3, a4;
} polynomial_quartic_t;
typedef struct
{
	real a0, a1, a2, a3, a4, a5;
} polynomial_quintic_t;

real quartic_calc_xt(polynomial_quartic_t *path, real t);
real quartic_calc_dxt(polynomial_quartic_t *path, real t);
real quartic_calc_ddxt(polynomial_quartic_t *path, real t);
real quartic_calc_dddxt(polynomial_quartic_t *path, real t);

real quintic_calc_xt(polynomial_quintic_t *path, real t);
real quintic_calc_dxt(polynomial_quintic_t *path, real t);
real quintic_calc_ddxt(polynomial_quintic_t *path, real t);
real quintic_calc_dddxt(polynomial_quintic_t *path, real t);

void quartic_polynomial(polynomial_quartic_t *path, real x0, real v0, real a0, real v1, real a1, real T);
void quintic_polynomial(polynomial_quintic_t *path, real x0, real v0, real a0, real x1, real v1, real a1, real T);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* POLYNOMIAL_H */
       
