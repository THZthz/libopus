/**
 * @file dubins_curve.c
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


#ifdef WIN32
#define _USE_MATH_DEFINES
#endif

#include <float.h>
#include <math.h>

#include "data_structure/array.h"
#include "math/dubins_curve.h"
#include "math/math.h"
#include "utils/utils.h"

INLINE real dubins_mod2pi(real theta)
{
	return theta - R_PI_2 * r_floor(theta / R_PI_2);
}

INLINE int dubins_LSL(real alpha, real beta, real d, vec3 *out)
{
	real tmp0      = d + r_sin(alpha) - r_sin(beta);
	real tmp1      = r_atan2((r_cos(beta) - r_cos(alpha)), tmp0);
	real p_squared = 2 + d * d - (2 * r_cos(alpha - beta)) + (2 * d * (r_sin(alpha) - r_sin(beta)));
	if (p_squared < 0) {
		return EDUBNOPATH;
	} else {
		out->x = dubins_mod2pi(tmp1 - alpha);
		out->y = r_sqrt(p_squared);
		out->z = dubins_mod2pi(beta - tmp1);
		return EDUBOK;
	}
}

INLINE int dubins_RSR(real alpha, real beta, real d, vec3 *out)
{
	real tmp0      = d - r_sin(alpha) + r_sin(beta);
	real tmp1      = r_atan2((r_cos(alpha) - r_cos(beta)), tmp0);
	real p_squared = 2 + d * d - (2 * r_cos(alpha - beta)) + (2 * d * (r_sin(beta) - r_sin(alpha)));
	if (p_squared < 0) {
		return EDUBNOPATH;
	} else {
		out->x = dubins_mod2pi(alpha - tmp1);
		out->y = r_sqrt(p_squared);
		out->z = dubins_mod2pi(tmp1 - beta);
		return EDUBOK;
	}
}

INLINE int dubins_RSL(real alpha, real beta, real d, vec3 *out)
{
	real tmp0      = d - r_sin(alpha) - r_sin(beta);
	real p_squared = -2 + d * d + 2 * r_cos(alpha - beta) - 2 * d * (r_sin(alpha) + r_sin(beta));
	if (p_squared < 0) {
		return EDUBNOPATH;
	} else {
		real tmp2;
		out->y = r_sqrt(p_squared);
		tmp2   = r_atan2((r_cos(alpha) + r_cos(beta)), tmp0) - r_atan2(2, out->y);
		out->x = dubins_mod2pi(alpha - tmp2);
		out->z = dubins_mod2pi(beta - tmp2);
		return EDUBOK;
	}
}

INLINE int dubins_LSR(real alpha, real beta, real d, vec3 *out)
{
	real tmp0      = d + r_sin(alpha) + r_sin(beta);
	real p_squared = -2 + d * d + 2 * r_cos(alpha - beta) + 2 * d * (r_sin(alpha) + r_sin(beta));
	if (p_squared < 0) {
		return EDUBNOPATH;
	} else {
		real tmp2;
		out->y = r_sqrt(p_squared);
		tmp2   = r_atan2((-r_cos(alpha) - r_cos(beta)), tmp0) - r_atan2(-2, out->y);
		out->x = dubins_mod2pi(tmp2 - alpha);
		out->z = dubins_mod2pi(tmp2 - beta);
		return EDUBOK;
	}
}

INLINE int dubins_RLR(real alpha, real beta, real d, vec3 *out)
{
	real tmp_rlr = (6 - d * d + 2 * r_cos(alpha - beta) + 2 * d * (r_sin(alpha) - r_sin(beta))) / 8;
	if (fabs(tmp_rlr) > 1) {
		return EDUBNOPATH;
	} else {
		out->y = dubins_mod2pi(R_PI_2 - r_acos(tmp_rlr));
		out->x = dubins_mod2pi(alpha - r_atan2((r_cos(alpha) - r_cos(beta)), d - r_sin(alpha) + r_sin(beta)) +
		                       dubins_mod2pi(out->y / 2));
		out->z = dubins_mod2pi(alpha - beta - out->x + dubins_mod2pi(out->y));
		return EDUBOK;
	}
}

INLINE int dubins_LRL(real alpha, real beta, real d, vec3 *out)
{
	real tmp_rlr = (6 - d * d + 2 * r_cos(alpha - beta) + 2 * d * (-r_sin(alpha) + r_sin(beta))) / 8;
	if (fabs(tmp_rlr) > 1) {
		return EDUBNOPATH;
	} else {
		out->y = dubins_mod2pi(R_PI_2 - r_acos(tmp_rlr));
		out->x = dubins_mod2pi(-alpha - r_atan2((r_cos(alpha) - r_cos(beta)), d + r_sin(alpha) - r_sin(beta)) + out->y / 2);
		out->z = dubins_mod2pi(dubins_mod2pi(beta) - alpha - out->x + dubins_mod2pi(out->y));
		return EDUBOK;
	}
}

int dubins_calculate(dubins_path_t *param, vec3 wpt1, vec3 wpt2, float r)
{
	vec3 ret[6];
	int  ret_code[6];

	real psi1 = wpt1.z;
	real psi2 = wpt2.z;
	real dx, dy, D, d;
	real theta, alpha, beta;
	int  best_word;
	real best_cost;
	int  i;

	param->start_vec[0] = wpt1.x;
	param->start_vec[1] = wpt1.y;
	param->start_vec[2] = wpt1.z;

	param->seg_lengths[0] = 0;
	param->seg_lengths[1] = 0;
	param->seg_lengths[2] = 0;

	param->turn_radius = r; /* (vel * vel) / (9.8 * tan(phi_lim)); */
	dx                 = wpt2.x - wpt1.x;
	dy                 = wpt2.y - wpt1.y;
	D                  = r_sqrt(dx * dx + dy * dy);
	d                  = D / param->turn_radius;

	theta     = dubins_mod2pi(r_atan2(dy, dx));
	alpha     = dubins_mod2pi(psi1 - theta);
	beta      = dubins_mod2pi(psi2 - theta);
	best_word = -1;
	best_cost = REAL_MAX;

	ret_code[0] = dubins_LSL(alpha, beta, d, &ret[0]);
	ret_code[1] = dubins_LSR(alpha, beta, d, &ret[1]);
	ret_code[2] = dubins_RSL(alpha, beta, d, &ret[2]);
	ret_code[3] = dubins_RSR(alpha, beta, d, &ret[3]);
	ret_code[4] = dubins_RLR(alpha, beta, d, &ret[4]);
	ret_code[5] = dubins_LRL(alpha, beta, d, &ret[5]);

	for (i = 0; i < 6; i++) {
		if (ret_code[i] != EDUBNOPATH) {
			real cost = ret[i].x + ret[i].y + ret[i].z;
			if (cost < best_cost) {
				best_cost = cost;
				best_word = i + 1;
			}
		}
	}

	if (best_word == -1) {
		return EDUBNOPATH;
	}

	param->type           = best_word;
	param->seg_lengths[0] = ret[best_word - 1].x;
	param->seg_lengths[1] = ret[best_word - 1].y;
	param->seg_lengths[2] = ret[best_word - 1].z;

	return EDUBOK;
}

void dubins_segment(real seg_param, vec3 seg_init, int type, vec3 *out)
{
	/* L_SEG 1 S_SEG 2 R_SEG 3 */
	switch (type) {
		case 1: {
			out->x = seg_init.x + r_sin(seg_init.z + seg_param) - r_sin(seg_init.z);
			out->y = seg_init.y - r_cos(seg_init.z + seg_param) + r_cos(seg_init.z);
			out->z = seg_init.z + seg_param;
			break;
		}
		case 2: {
			out->x = seg_init.x + r_cos(seg_init.z) * seg_param;
			out->y = seg_init.y + r_sin(seg_init.z) * seg_param;
			out->z = seg_init.z;
			break;
		}
		case 3: {
			out->x = seg_init.x - r_sin(seg_init.z - seg_param) + r_sin(seg_init.z);
			out->y = seg_init.y + r_cos(seg_init.z - seg_param) - r_cos(seg_init.z);
			out->z = seg_init.z - seg_param;
			break;
		}
		default:
			break;
	}
}

real dubins_get_length(dubins_path_t *path)
{
	return (path->seg_lengths[0] + path->seg_lengths[1] + path->seg_lengths[2]) * path->turn_radius;
}

void dubins_get_point(dubins_path_t *path, real t, vec3 *end_pt)
{
	real t_prime = t / path->turn_radius;
	vec3 p_init  = {0, 0, 0};
	vec3 mid_pt1, mid_pt2;

	enum { L_SEG = 1,
		   S_SEG = 2,
		   R_SEG = 3 };

	int dir_data[6][3] = {{L_SEG, S_SEG, L_SEG},
	                      {L_SEG, S_SEG, R_SEG},
	                      {R_SEG, S_SEG, L_SEG},
	                      {R_SEG, S_SEG, R_SEG},
	                      {R_SEG, L_SEG, R_SEG},
	                      {L_SEG, R_SEG, L_SEG}};

	real param1 = path->seg_lengths[0];
	real param2 = path->seg_lengths[1];
	int *types  = &(dir_data[path->type - 1][0]);

	p_init.z = path->start_vec[2];
	dubins_segment(param1, p_init, types[0], &mid_pt1);
	dubins_segment(param2, mid_pt1, types[1], &mid_pt2);

	if (t_prime < param1)
		dubins_segment(t_prime, p_init, types[0], end_pt);
	else if (t_prime < (param1 + param2))
		dubins_segment(t_prime - param1, mid_pt1, types[1], end_pt);
	else
		dubins_segment(t_prime - param1 - param2, mid_pt2, types[2], end_pt);

	end_pt->x = end_pt->x * path->turn_radius + path->start_vec[0];
	end_pt->y = end_pt->y * path->turn_radius + path->start_vec[1];
	end_pt->z = dubins_mod2pi(end_pt->z);
}

/**
 * build the trajectory from the lowest-cost path
 * @param out must be created by `array_create`
 */
vec3 *dubins_get_trajectory(dubins_path_t *path, real step)
{
	vec3 *out;
	vec3  pt;
	real  x      = 0;
	real  length = dubins_get_length(path);
	length       = r_floor(length / step);

	array_create(out, sizeof(vec3));
	while (x < length) {
		dubins_get_point(path, x, &pt);
		array_push(out, &pt);
		x += step;
	}

	return out;
}
