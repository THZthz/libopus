/**
 * @file reeds_shepp_curve.h
 * Author:        _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/4/3 10:24
 *
 * @see 1990 Optimal paths for a car that goes both forwards and backwards. J. A. Reeds, L. A. Shepp. Pacific J.
 *
 * @note There are many notation errors in this paper, and there are many formula derivation errors.
 *        When reading the paper, please note that the errors! Errors mainly concentrated in Section 8.
 */
#ifndef PATHFINDING_REEDS_SHEPP_H
#define PATHFINDING_REEDS_SHEPP_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <float.h>
#include <limits.h>
#include <math.h>

#include "math/math.h"

typedef enum rs_seg_type {
	RS_N = 0,
	RS_L = 1,
	RS_S = 2,
	RS_R = 3
} rs_seg_type_t;

/* P 371: TABLE 1 */
extern enum rs_seg_type RS_seg_type[18][5];

typedef struct rs_path {
	vec3           start_state, goal_state;
	double         length[5];
	rs_seg_type_t *type;
	double         total_length;
	double         turning_radius;
} rs_path_t;

/*!
 * Refer to this paper:
 *   1990 Optimal paths for a car that goes both forwards and backwards. J. A. Reeds, L. A. Shepp. Pacific J.
 *
 * Notes: There are many notation errors in this paper, and there are many formula derivation errors.
 *        When reading the paper, please note that the errors! Errors mainly concentrated in Section 8.
 */

void   rs_path_set_data(rs_path_t *path, rs_seg_type_t type[5], double t, double u, double v, double w, double x);
double rs_path_distance(rs_path_t *path);
void   rs_path_calculate(rs_path_t *path, double x_0, double y_0, double yaw_0, double x_1, double y_1, double yaw_1, double turning_radius);
vec3  *rs_path_build_trajectory(rs_path_t *rs_path, double step_size);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* PATHFINDING_REEDS_SHEPP_H */
