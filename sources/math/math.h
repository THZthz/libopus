/**
 * @file r_math.h
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/3/10
 */
#ifndef COLLISIONS_MATH_H
#define COLLISIONS_MATH_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <float.h>
#include <math.h>
#include <stdint.h>

#include "../config.h"

typedef CONFIG_REAL real;
typedef struct vec2 vec2;
typedef struct vec3 vec3;
typedef struct vec4 vec4;

struct vec2 {
	real x, y;
};

struct vec3 {
	real x, y, z;
};

struct vec4 {
	real x, y, z, w;
};

#define REAL_MAX CONFIG_REAL_MAX
#define REAL_MIN CONFIG_REAL_MIN
#define REAL_EPSILON CONFIG_REAL_EPSILON

#define R_PI M_PI
#define R_PI_2 (R_PI * R_PI)

#define r_1_(_func, _x) ((real) _func((double) (_x)))
#define r_2_(_func, _x1, _x2) ((real) _func((double) (_x1), (double) (_x2)))
#define r_abs(x) r_1_(fabs, (x))
#define r_pow(x, n) r_2_(pow, (x), (n))
#define r_pow2(x) r_pow((x), 2)
#define r_sqrt(x) r_1_(sqrt, (x))
#define r_hypot(a, b) r_sqrt(r_pow2(a) + r_pow2(b))
#define r_floor(x) r_1_(floor, (x))
#define r_ceil(x) r_1_(ceil, (x))
#define r_round(x) r_1_(round, (x))
#define r_sin(x) r_1_(sin, (x))
#define r_cos(x) r_1_(cos, (x))
#define r_tan(x) r_1_(tan, (x))
#define r_acos(x) r_1_(acos, (x))
#define r_asin(x) r_1_(asin, (x))
#define r_atan(x) r_1_(atan, (x))
#define r_atan2(y, x) r_2_(atan2, (y), (x))

#define r_deg2rad(deg) ((deg) *R_PI / 180)
#define r_rad2deg(rad) ((rad) *180 / R_PI)

#define r_random(n) (r_xrand() % (n))

#define r_min(a, b) ((a) < (b) ? (a) : (b))
#define r_max(a, b) ((a) > (b) ? (a) : (b))
#define r_sign(a) ((a) > 0 ? 1 : ((a) < 0 ? -1 : 0))
#define r_map(num, num_min, num_max, min, max) (((num) - (num_min)) * ((max) - (min)) / ((num_max) - (num_min)) + (min))
#define r_clamp(num, min, max) (r_max((min), r_min((num), (max))))

/*
 * the meaning of:
 * 	- loro (a, b)
 * 	- lcro [a, b)
 * 	- lorc (a, b]
 * 	- lcrc [a, b]
 */

#define r_random_loro(_min, _max) (r_rand() % ((_max) - (_min) + 1) + (_min) -1)
#define r_random_lorc(_min, _max) (r_rand() % ((_max) - (_min)) + (_min) + 1)
#define r_random_lcro(_min, _max) (r_rand() % ((_max) - (_min)) + (_min))
#define r_random_lcrc(_min, _max) (r_rand() % ((_max) - (_min) + 1) + (_min))
#define r_rand() rand()
/* random number [0, 1] */
#define r_random_01() (rand() / (double) RAND_MAX)
/* random number in range [-1,1] */
#define r_random_m11() (2.0 * (double) rand() / (double) RAND_MAX - 1.0)

#define r_normalize_angle(rad)            \
	while ((rad) < 0) (rad) += 2 * R_PI; \
	while ((rad) > 2 * R_PI) (rad) -= 2 * R_PI;

#define vec_x(vec) ((vec).x)
#define vec_y(vec) ((vec).y)
#define vec_z(vec) ((vec).z)
#define vec_w(vec) ((vec).w)

#define LINEAR_MNT_S() do {
#define LINEAR_MNT_E() \
	}                  \
	while (0)

#define vec2_add_xy(res, vec, x, y) \
	LINEAR_MNT_S()                  \
	vec_x(res) = vec_x(vec) + (x);  \
	vec_y(res) = vec_y(vec) + (y);  \
	LINEAR_MNT_E()
#define vec2_sub_xy(res, vec, x, y) \
	LINEAR_MNT_S()                  \
	vec_x(res) = vec_x(vec) - (x);  \
	vec_y(res) = vec_y(vec) - (y);  \
	LINEAR_MNT_E()
#define vec2_scale_xy(res, vec, x, y) \
	LINEAR_MNT_S()                    \
	vec_x(res) = vec_x(vec) * (x);    \
	vec_y(res) = vec_y(vec) * (y);    \
	LINEAR_MNT_E()

#define vec2_add(res, a, b)           \
	LINEAR_MNT_S()                    \
	vec_x(res) = vec_x(a) + vec_x(b); \
	vec_y(res) = vec_y(a) + vec_y(b); \
	LINEAR_MNT_E()
#define vec2_sub(res, a, b)           \
	LINEAR_MNT_S()                    \
	vec_x(res) = vec_x(a) - vec_x(b); \
	vec_y(res) = vec_y(a) - vec_y(b); \
	LINEAR_MNT_E()
/**
 * @brief generate a vector from `from` points to `to`
 */
#define vec2_to(res, from, to) vec2_sub((res), (to), (from))
#define vec2_scale(res, vec, scalar)    \
	LINEAR_MNT_S()                      \
	vec_x(res) = vec_x(vec) * (scalar); \
	vec_y(res) = vec_y(vec) * (scalar); \
	LINEAR_MNT_E()
#define vec2_mul_inner(a, b) (vec_x(a) * vec_x(b) + vec_y(a) * vec_y(b))
#define vec2_dot vec2_mul_inner
#define vec2_length(vec) (r_sqrt(vec2_dot((vec), (vec))))
#define vec2_len vec2_length
#define vec2_norm(res, vec)             \
	LINEAR_MNT_S()                      \
	real _k = vec2_len(vec);            \
	if (_k == 0) vec2_set((res), 0, 0); \
	else {                              \
		_k = 1.0 / _k;                  \
		vec2_scale((res), (vec), _k);   \
	}                                   \
	LINEAR_MNT_E()
#define vec2_dup(res, vec)   \
	LINEAR_MNT_S()           \
	vec_x(res) = vec_x(vec); \
	vec_y(res) = vec_y(vec); \
	LINEAR_MNT_E()
#define vec2_copy vec2_dup
#define vec2_set(vec, x, y) \
	LINEAR_MNT_S()          \
	vec_x(vec) = (x);       \
	vec_y(vec) = (y);       \
	LINEAR_MNT_E()

#define vec3_add(res, a, b)           \
	LINEAR_MNT_S()                    \
	vec_x(res) = vec_x(a) + vec_x(b); \
	vec_y(res) = vec_y(a) + vec_y(b); \
	vec_z(res) = vec_z(a) + vec_z(b); \
	LINEAR_MNT_E()
#define vec3_sub(res, a, b)           \
	LINEAR_MNT_S()                    \
	vec_x(res) = vec_x(a) - vec_x(b); \
	vec_y(res) = vec_y(a) - vec_y(b); \
	vec_z(res) = vec_z(a) - vec_z(b); \
	LINEAR_MNT_E()
#define vec3_scale(res, vec, scalar)    \
	LINEAR_MNT_S()                      \
	vec_x(res) = vec_x(vec) * (scalar); \
	vec_y(res) = vec_y(vec) * (scalar); \
	vec_z(res) = vec_z(vec) * (scalar); \
	LINEAR_MNT_E()
#define vec3_mul_inner(a, b) (vec_x(a) * vec_x(b) + vec_y(a) * vec_y(b) + vec_z(a) * vec_z(b))
#define vec3_dot vec3_mul_inner
#define vec2_cross(a, b) (vec_x(a) * vec_y(b) - vec_x(b) * vec_y(a))

#define vec3_length(vec) (r_sqrt(vec3_dot((vec), (vec))))
#define vec3_len vec3_length
#define vec3_norm(res, vec)        \
	LINEAR_MNT_S()                 \
	real _k = 1.0 / vec3_len(vec); \
	vec3_scale((res), (vec), _k);  \
	LINEAR_MNT_E()
#define vec3_dup(res, vec)   \
	LINEAR_MNT_S()           \
	vec_x(res) = vec_x(vec); \
	vec_y(res) = vec_y(vec); \
	vec_z(res) = vec_z(vec); \
	LINEAR_MNT_E()
#define vec3_copy vec3_dup
#define vec3_set(vec, x, y, z) \
	LINEAR_MNT_S()             \
	vec_x(vec) = (x);          \
	vec_y(vec) = (y);          \
	vec_z(vec) = (z);          \
	LINEAR_MNT_E()

#define vec4_add(res, a, b)           \
	LINEAR_MNT_S()                    \
	vec_x(res) = vec_x(a) + vec_x(b); \
	vec_y(res) = vec_y(a) + vec_y(b); \
	vec_z(res) = vec_z(a) + vec_z(b); \
	vec_w(res) = vec_w(a) + vec_w(b); \
	LINEAR_MNT_E()
#define vec4_sub(res, a, b)           \
	LINEAR_MNT_S()                    \
	vec_x(res) = vec_x(a) - vec_x(b); \
	vec_y(res) = vec_y(a) - vec_y(b); \
	vec_z(res) = vec_z(a) - vec_z(b); \
	vec_w(res) = vec_w(a) - vec_w(b); \
	LINEAR_MNT_E()
#define vec4_scale(res, vec, scalar)    \
	LINEAR_MNT_S()                      \
	vec_x(res) = vec_x(vec) * (scalar); \
	vec_y(res) = vec_y(vec) * (scalar); \
	vec_z(res) = vec_z(vec) * (scalar); \
	vec_w(res) = vec_w(vec) * (scalar); \
	LINEAR_MNT_E()
#define vec4_mul_inner(a, b) (vec_x(a) * vec_x(b) * vec_y(a) + vec_y(b) + vec_z(a) * vec_z(b) + vec_w(a) * vec_w(b))
#define vec4_dot vec4_mul_inner
#define vec4_length(vec) (r_sqrt(vec4_dot((vec), (vec))))
#define vec4_len vec4_length
#define vec4_norm(res, vec)               \
	LINEAR_MNT_S()                        \
	real _k = 1.0 / (real) vec4_len(vec); \
	vec4_scale((res), (vec), _k);         \
	LINEAR_MNT_E()
#define vec4_dup(res, vec)   \
	LINEAR_MNT_S()           \
	vec_x(res) = vec_x(vec); \
	vec_y(res) = vec_y(vec); \
	vec_z(res) = vec_z(vec); \
	vec_w(res) = vec_w(vec); \
	LINEAR_MNT_E()
#define vec4_copy vec4_dup
#define vec4_set(vec, x, y, z, w) \
	LINEAR_MNT_S()                \
	vec_x(vec) = (x);             \
	vec_y(vec) = (y);             \
	vec_z(vec) = (z);             \
	vec_w(vec) = (w);             \
	LINEAR_MNT_E()

#define vec2_abs(vec)                         \
	LINEAR_MNT_S()                            \
	vec_x(vec) = r_abs((real) vec_x(vec)); \
	vec_y(vec) = r_abs((real) vec_y(vec)); \
	LINEAR_MNT_E()
#define vec2_get_angle(vec) (r_atan2(vec_y(vec), vec_x(vec)))
#define vec2_set_angle(vec, angle)     \
	LINEAR_MNT_S()                     \
	real _len  = vec2_get_length(vec); \
	vec_x(vec) = _len * cos(angle);    \
	vec_y(vec) = _len * sin(angle);    \
	LINEAR_MNT_E()

/**
 * @return the angle between the vector `b - a` and the x-axis in radians.
 */
#define vec2_angle(a, b) (r_atan2(vec_y(b) - vec_y(a), vec_x(b) - vec_x(a)))

#define vec2_set_length(vec, length)                     \
	do {                                                 \
		real _ang  = r_atan2(vec_y(vec), vec_x(vec)); \
		vec_x(vec) = (real) (length) *r_cos(_ang);    \
		vec_y(vec) = (real) (length) *r_sin(_ang);    \
	} while (0)
#define vec2_get_length(vec) (r_sqrt(vec2_get_length2(vec)))
#define vec2_get_length2(vec) (vec2_dot((vec), (vec)))
#define vec2_length2 vec2_get_length2

#define vec2_dist(a, b) (r_hypot(vec_x(a) - vec_x(b), vec_y(a) - vec_y(b)))
#define vec2_dist2(a, b) ((vec_x(a) - vec_x(b)) * (vec_x(a) - vec_x(b)) + (vec_y(a) - vec_y(b)) * (vec_y(a) - vec_y(b)))

#define vec2_tame(vec, max_len)                      \
	LINEAR_MNT_S()                                   \
	if (vec2_length2(vec) > (max_len) * (max_len)) { \
		vec2_set_length((vec), (max_len));           \
	}                                                \
	LINEAR_MNT_E()

#define vec2_rotate(res, vec, rad)                        \
	LINEAR_MNT_S()                                        \
	real _x    = vec_x(vec);                              \
	real _y    = vec_y(vec);                              \
	vec_x(res) = _x * r_cos(rad) - _y * r_sin(rad); \
	vec_y(res) = _x * r_sin(rad) + _y * r_cos(rad); \
	LINEAR_MNT_E()

#define vec2_rotate_about(res, vec, point, angle)                                                      \
	LINEAR_MNT_S()                                                                                     \
	real c_ = r_cos(angle), s_ = r_sin(angle);                                                   \
	real x_    = vec_x(point) + ((vec_x(vec) - vec_x(point)) * c_ - (vec_y(vec) - vec_y(point)) * s_); \
	vec_y(res) = vec_y(point) + ((vec_x(vec) - vec_x(point)) * s_ + (vec_y(vec) - vec_y(point)) * c_); \
	vec_x(res) = x_;                                                                                   \
	LINEAR_MNT_E()


/**
 * @brief Change this vector to be perpendicular to what it was before. (Effectively
 * 		rotates it 90 degrees in a clockwise direction)
 */
#define vec2_perp(res, vec)  \
	LINEAR_MNT_S()           \
	real _x    = vec_x(vec); \
	vec_x(res) = vec_y(vec); \
	vec_y(res) = -_x;        \
	LINEAR_MNT_E()

/**
 * transpose skew a vector to be perp with itself (clockwise rotation)
 */
#define vec2_skewT vec2_perp

/**
 * skew a vector to be perp with itself (counter-clockwise rotation)
 */
#define vec2_skew(res, vec)   \
	LINEAR_MNT_S()            \
	real _x    = vec_x(vec);  \
	vec_x(res) = -vec_y(vec); \
	vec_y(res) = _x;          \
	LINEAR_MNT_E()

#define vec2_cross3(a, b, c) ((vec_x(b) - vec_x(a)) * (vec_y(c) - vec_y(a)) - (vec_y(b) - vec_y(a)) * (vec_x(c) - vec_x(a)))

#define vec2_reverse(res, vec) \
	LINEAR_MNT_S()             \
	vec_x(res) = -vec_x(vec);  \
	vec_y(res) = -vec_y(vec);  \
	LINEAR_MNT_E()

#define vec2_project(res, vec, axis)                                 \
	LINEAR_MNT_S()                                                   \
	real _amt  = vec2_mul_inner((vec), (axis)) / vec2_length2(axis); \
	vec_x(res) = _amt * vec_x(axis);                                 \
	vec_y(res) = _amt * vec_y(axis);                                 \
	LINEAR_MNT_E()

/**
 * @brief Project this vector onto a vector of unit length. This is slightly more efficient
 * 		than `vec2_project` when dealing with unit vectors.
 * @param {Vector} axis The unit vector to project onto.
 */
#define vec2_projectN(res, vec, axis)           \
	LINEAR_MNT_S()                              \
	real _amt  = vec2_mul_inner((vec), (axis)); \
	vec_x(res) = _amt * vec_x(axis);            \
	vec_y(res) = _amt * vec_y(axis);            \
	LINEAR_MNT_E()


/**
 * @brief Reflect this vector on an arbitrary axis.
 * @param {Vector} axis The vector representing the axis.
 */
#define vec2_reflect(res, vec, axis)    \
	LINEAR_MNT_S()                      \
	real _x = vec_x(vec);               \
	real _y = vec_y(vec);               \
	vec2_project((res), (vec), (axis)); \
	vec_x(res) = vec_x(res) * 2 - _x;   \
	vec_y(res) = vec_y(res) * 2 - _y;   \
	LINEAR_MNT_E()

/**
 * @brief Reflect this vector on an arbitrary axis (represented by a unit vector). This is
 * 	slightly more efficient than `reflect` when dealing with an axis that is a unit vector.
 * @param {Vector} axis The unit vector representing the axis.
 */
#define vec2_reflectN(res, vec, axis)    \
	LINEAR_MNT_S()                       \
	real _x = vec_x(vec);                \
	real _y = vec_y(vec);                \
	vec2_projectN((res), (vec), (axis)); \
	vec_x(res) = vec_x(res) * 2 - _x;    \
	vec_y(res) = vec_y(res) * 2 - _y;    \
	LINEAR_MNT_E()

/**
 * linearly interpolate between two vectors
 */
#define vec2_lerp(res, tail, head, t)               \
	LINEAR_MNT_S()                                  \
	real _t[2];                                     \
	_t[0]      = (t) * (vec_x(head) - vec_x(tail)); \
	_t[1]      = (t) * (vec_y(head) - vec_y(tail)); \
	vec_x(res) = vec_x(tail) + _t[0];               \
	vec_y(res) = vec_y(tail) + _t[1];               \
	LINEAR_MNT_E()

/**
 *@brief lerp ratio of the origin onto the vector h - t
 *@see http://www.geometrictools.com/Documentation/DistancePointLine.pdf
 */
#define vec2_lerp_ratio(res_t, tail, head)                                                                                \
	LINEAR_MNT_S()                                                                                                        \
	real _M[2];                                                                                                           \
	real _neg_tail[2];                                                                                                    \
	_neg_tail[0] = -vec_x(tail);                                                                                          \
	_neg_tail[1] = -vec_y(tail);                                                                                          \
	_M[0]        = vec_x(head) - vec_x(tail);                                                                             \
	_M[1]        = vec_y(head) - vec_y(tail);                                                                             \
	(res_t)      = r_clamp((_M[0] * _neg_tail[0] + _M[1] * _neg_tail[1]) / (_M[0] * _M[0] + _M[1] * _M[1]), 0.0, 1.0); \
	LINEAR_MNT_E()

#define vec2_equal(a, b) (r_abs((real) (vec_x(a) - vec_x(b))) < DBL_EPSILON && r_abs((real) (vec_y(a) - vec_y(b))) < DBL_EPSILON)

#define vec2_swap(a, b)   \
	LINEAR_MNT_S()        \
	vec2 _tmp;            \
	vec2_copy(_tmp, (a)); \
	vec2_copy((a), (b));  \
	vec2_copy((b), _tmp); \
	LINEAR_MNT_E()

vec2 vec2_(real x, real y);
vec3 vec3_(real x, real y, real z);
void vec3_mul_cross(vec3 r, vec3 a, vec3 b);

unsigned r_xrand(void);
real     r_round_n(real number, unsigned int bits);
real     r_sigmoid(real a);
void     r_init_sigmoid_lookup(void);
real     r_sigmoid_cached(real a);

void mat_print(real *mat, int y, int x);
void mat_identity(real *mat, int y, int x);

uint8_t linear_lup(real A[], real LU[], uint8_t P[], uint16_t row);
real    det(real A[], uint16_t row);
void    mat_mul(real A[], real B[], real C[], uint16_t row_a, uint16_t column_a, uint16_t column_b);
void    mat_transpose(real A[], uint16_t row, uint16_t column);
uint8_t mat_inv(real A[], uint16_t row);
uint8_t mat_qr(real *A, real *Q, real *R, uint16_t row_a, uint16_t column_a, int only_compute_R);
void    linear_solve_upper_triangular(const real A[], real x[], real b[], uint16_t column);
void    linear_solve_lower_triangular(const real A[], real x[], real b[], uint16_t row);
void    linear_solve_qr(real A[], real x[], real b[], uint16_t row, uint16_t column);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* COLLISIONS_MATH_H */
