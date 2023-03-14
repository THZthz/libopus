/**
 * @file r_math.h
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/3/10
 */
#ifndef MATH_H
#define MATH_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include "config.h"

typedef OPUS_CONFIG_REAL opus_real;
typedef struct opus_vec2 opus_vec2;
typedef struct opus_vec3 opus_vec3;
typedef struct opus_vec4 opus_vec4;
typedef float            opus_mat2d[6];

struct opus_vec2 {
	opus_real x, y;
};

struct opus_vec3 {
	opus_real x, y, z;
};

struct opus_vec4 {
	opus_real x, y, z, w;
};

#define OPUS_REAL_MAX OPUS_CONFIG_REAL_MAX
#define OPUS_REAL_MIN OPUS_CONFIG_REAL_MIN
#define OPUS_REAL_EPSILON OPUS_CONFIG_REAL_EPSILON

#define OPUS_PI ((opus_real) M_PI)
#define OPUS_PI2 (OPUS_PI * OPUS_PI)

#define OPUS_MAX_STATIC_MATRIX_DIMENSION (15)

// opus_real opus_random(opus_real n) { return r_xrand() % n; }


opus_vec2 opus_vec2_(opus_real x, opus_real y);
opus_vec3 opus_vec3_(opus_real x, opus_real y, opus_real z);

opus_vec3 opus_vec3_cross(opus_vec3 a, opus_vec3 b);

int       opus_vec2_equal_(opus_vec2 vec_a, opus_vec2 vec_b, opus_real epsilon);
int       opus_vec2_equal(opus_vec2 vec_a, opus_vec2 vec_b);
void      opus_vec2_set(opus_vec2 *vec, opus_real x, opus_real y);
opus_vec2 opus_vec2_add(opus_vec2 x, opus_vec2 to_add);
opus_vec2 opus_vec2_sub(opus_vec2 x, opus_vec2 to_sub);
opus_vec2 opus_vec2_to(opus_vec2 x, opus_vec2 to);
opus_vec2 opus_vec2_to_xy(opus_vec2 vec, opus_real x, opus_real y);
opus_vec2 opus_vec2_scale(opus_vec2 a, opus_real scalar);
opus_vec2 opus_vec2_div(opus_vec2 a, opus_real d);
opus_real opus_vec2_dot(opus_vec2 a, opus_vec2 b);
opus_real opus_vec2_cross(opus_vec2 a, opus_vec2 b);
opus_real opus_vec2_len2(opus_vec2 a);
opus_real opus_vec2_len(opus_vec2 a);
opus_vec2 opus_vec2_norm(opus_vec2 a);
opus_vec2 opus_vec2_copy(opus_vec2 *a, opus_vec2 b);
opus_vec2 opus_vec2_inv(opus_vec2 a);
opus_vec2 opus_vec2_neg(opus_vec2 a);
opus_real opus_vec2_get_angle(opus_vec2 a);
opus_vec2 opus_vec2_set_angle(opus_vec2 *a, opus_real radian);
opus_real opus_vec2_get_length(opus_vec2 a);
opus_vec2 opus_vec2_set_length(opus_vec2 *a, opus_real len);
opus_real opus_vec2_angle(opus_vec2 a, opus_vec2 b);
opus_real opus_vec2_dist2(opus_vec2 a, opus_vec2 b);
opus_real opus_vec2_dist(opus_vec2 a, opus_vec2 b);
void      opus_vec2_swap(opus_vec2 *a, opus_vec2 *b);
opus_real opus_vec2_lerp_ratio(opus_vec2 tail, opus_vec2 head);
opus_vec2 opus_vec2_lerp(opus_vec2 head, opus_vec2 tail, opus_real t);
opus_vec2 opus_vec2_project(opus_vec2 vec, opus_vec2 axis);
opus_vec2 opus_vec2_reflect(opus_vec2 vec, opus_vec2 axis);
opus_vec2 opus_vec2_projectN(opus_vec2 vec, opus_vec2 axis);
opus_vec2 opus_vec2_reflectN(opus_vec2 vec, opus_vec2 axis);
opus_vec2 opus_vec2_rotate(opus_vec2 vec, opus_real rad);
opus_vec2 opus_vec2_rotateT(opus_vec2 vec, opus_real rad);
opus_vec2 opus_vec2_rotate_about(opus_vec2 vec, opus_vec2 point, opus_real rad);
opus_vec2 opus_vec2_perp(opus_vec2 v);
opus_vec2 opus_vec2_skewT(opus_vec2 v);
opus_vec2 opus_vec2_skew(opus_vec2 v);
opus_real opus_vec2_cross3(opus_vec2 a, opus_vec2 b, opus_vec2 c);

opus_real opus_max(opus_real a, opus_real b);
opus_real opus_min(opus_real a, opus_real b);
float     opus_min_f(float a, float b);
float     opus_max_f(float a, float b);
double    opus_max_d(double a, double b);
double    opus_min_d(double a, double b);
int       opus_min_i(int a, int b);
int       opus_max_i(int a, int b);
int       opus_sign(opus_real a);
opus_real opus_map(opus_real x, opus_real x_min, opus_real x_max, opus_real min, opus_real max);
opus_real opus_clamp(opus_real num, opus_real min, opus_real max);
int       opus_clamp_i(int num, int min, int max);

unsigned  opus_xrand(void);
int       opus_rand(void);
int       opus_rand_loro(int min, int max);
int       opus_rand_lorc(int min, int max);
int       opus_rand_lcro(int min, int max);
int       opus_rand_lcrc(int min, int max);
opus_real opus_rand_01(void);
opus_real opus_rand_m11(void);

opus_real opus_inv_sqrt(opus_real x, int iterations);
opus_real opus_pow(opus_real x, opus_real n);
opus_real opus_pow2(opus_real x);
opus_real opus_sqrt(opus_real x);
opus_real opus_mod(opus_real a, opus_real b);
opus_real opus_hypot2(opus_real a, opus_real b);
opus_real opus_hypot(opus_real a, opus_real b);
opus_real opus_floor(opus_real x);
opus_real opus_ceil(opus_real x);
opus_real opus_round(opus_real x);
opus_real opus_round_n(opus_real number, unsigned int bits);
opus_real opus_sin(opus_real x);
opus_real opus_cos(opus_real x);
opus_real opus_tan(opus_real x);
opus_real opus_asin(opus_real x);
opus_real opus_acos(opus_real x);
opus_real opus_atan2(opus_real y, opus_real x);
opus_real opus_abs(opus_real x);
opus_real opus_exp(opus_real x);
opus_real opus_deg2rad(opus_real deg);
opus_real opus_rad2deg(opus_real rad);
int       opus_fuzzy_equal(opus_real a, opus_real b, opus_real epsilon);
int       opus_equal(opus_real a, opus_real b);
void      opus_swap(opus_real *a, opus_real *b);
opus_real opus_sigmoid(opus_real a);
void      opus_init_sigmoid_lookup(void);
opus_real opus_sigmoid_cached(opus_real a);

void      opus_mat_print(opus_real *mat, int n, int m);
void      opus_mat_identity(opus_real *mat, uint16_t n, uint16_t m);
void      mat_add(opus_real *A, opus_real *B, uint16_t n, uint16_t m);
void      opus_mat_sub(opus_real *A, opus_real *B, uint16_t n, uint16_t m);
void      opus_mat_scale(opus_real *A, opus_real k, uint16_t n, uint16_t m);
void      opus_mat_mul(opus_real *A, opus_real *B, opus_real *C, uint16_t n, uint16_t r, uint16_t m);
void      opus_mat_fast_pow(opus_real *A, uint64_t n);
uint8_t   opus_mat_lup(opus_real *A, opus_real *LU, uint8_t *P, uint16_t n);
opus_real opus_mat_det(opus_real *A, uint16_t n);
void      opus_mat_transpose(opus_real *A, uint16_t n, uint16_t m);
uint8_t   opus_mat_inv(opus_real *A, uint16_t row);
uint8_t   opus_mat_qr(opus_real *A, opus_real *Q, opus_real *R, uint16_t r, uint16_t c, int r_only);
void      opus_linear_solve_ut(const opus_real *A, opus_real *x, opus_real *b, uint16_t row);
void      opus_linear_solve_lt(const opus_real *A, opus_real *x, opus_real *b, uint16_t row);
void      opus_linear_solve_qr(opus_real *A, opus_real *x, opus_real *b, uint16_t row, uint16_t column);

void      opus_mat2d_identity(opus_mat2d mat);
void      opus_mat2d_translate(opus_mat2d mat, float tx, float ty);
void      opus_mat2d_scale(opus_mat2d mat, float sx, float sy);
void      opus_mat2d_rotate(opus_mat2d mat, float a);
void      opus_mat2d_rotate_about(opus_mat2d mat, float a, opus_vec2 p);
void      opus_mat2d_skew_x(opus_mat2d mat, float a);
void      opus_mat2d_skew_y(opus_mat2d mat, float a);
void      opus_mat2d_mul(opus_mat2d mat, const opus_mat2d to_mult);
void      opus_mat2d_pre_mul(opus_mat2d mat, opus_mat2d to_mult);
int       opus_mat2d_inv(opus_mat2d inv, const opus_mat2d t);
void      opus_mar2d_pre_mul_xy(opus_real *dx, opus_real *dy, const opus_mat2d mat, opus_real sx, opus_real sy);
opus_vec2 opus_mat2d_pre_mul_vec(opus_mat2d mat, opus_vec2 src);
void      opus_mat2d_copy(opus_mat2d dst, opus_mat2d src);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* MATH_H */
