
#include "math/math.h"
#include "utils/utils.h"
#include <string.h>

#define SIGMOID_LOOKUP_SIZE (4096)
#define SIGMOID_RANGE_MIN (-15.0)
#define SIGMOID_RANGE_MAX (15.0)

static int          sigmoid_is_inited_ = 0;
static double       sigmoid_interval_  = 0.12;
static double       sigmoid_lookup_table_[SIGMOID_LOOKUP_SIZE];
static unsigned int r_seed = 0x11223344;

/**
 * @brief https://zhuanlan.zhihu.com/p/445813662
 * @param x
 * @param iterations
 * @return
 */
opus_real opus_inv_sqrt(opus_real x, int iterations) { return 1 / opus_sqrt(x); }

opus_real opus_deg2rad(opus_real deg) { return deg * OPUS_PI / 180; }

opus_real opus_rad2deg(opus_real rad) { return rad * 180 / OPUS_PI; }

int opus_fuzzy_equal(opus_real a, opus_real b, opus_real epsilon) { return opus_abs(a - b) < epsilon; }

/**
 * @brief fuzzy equal
 * @param a
 * @param b
 * @return
 */
int opus_equal(opus_real a, opus_real b) { return opus_fuzzy_equal(a, b, OPUS_REAL_EPSILON); }

unsigned int opus_xrand(void) { return (((r_seed = r_seed * 214013L + 2531011L) >> 16) & 0x7fffffff); }

opus_real opus_pow(opus_real x, opus_real n) { return (opus_real) pow((double) x, (double) n); }

opus_real opus_pow2(opus_real x) { return x * x; }

opus_real opus_sqrt(opus_real x) { return (opus_real) sqrt((double) x); }

opus_real opus_mod(opus_real a, opus_real b) { return (opus_real) fmod((double) a, (double) b); }

/* a*a+b*b */
opus_real opus_hypot2(opus_real a, opus_real b) { return opus_pow2(a) + opus_pow2(b); }

/* sqrt(a*a+b*b) */
opus_real opus_hypot(opus_real a, opus_real b) { return opus_sqrt(opus_hypot2(a, b)); }

opus_real opus_floor(opus_real x) { return (opus_real) floor((double) x); }

opus_real opus_ceil(opus_real x) { return (opus_real) ceil((double) x); }

opus_real opus_round(opus_real x) { return (opus_real) round((double) x); }

opus_real opus_sin(opus_real x) { return (opus_real) sin((double) x); }

opus_real opus_cos(opus_real x) { return (opus_real) cos((double) x); }

opus_real opus_tan(opus_real x) { return (opus_real) tan((double) x); }

opus_real opus_asin(opus_real x) { return (opus_real) asin((double) x); }

opus_real opus_acos(opus_real x) { return (opus_real) acos((double) x); }

opus_real opus_atan2(opus_real y, opus_real x) { return (opus_real) atan2((double) y, (double) x); }

opus_real opus_abs(opus_real x) { return x < 0 ? -x : x; }

opus_real opus_exp(opus_real x) { return (opus_real) exp((double) x); }

void opus_swap(opus_real *a, opus_real *b)
{
	opus_real t = *a;
	*a          = *b;
	*b          = t;
}

opus_real opus_round_n(opus_real number, unsigned int bits)
{
	int64_t int_part = (int64_t) number;

	number -= (opus_real) int_part;
	number *= opus_pow(10, bits);
	number = (opus_real) ((int64_t) (number + 0.5));
	number *= opus_pow(1.0 / 10.0, bits);
	return (opus_real) int_part + number;
}

opus_real opus_sigmoid(opus_real a)
{
	if (a < -45.0) return 0;
	if (a > 45.0) return 1;
	return 1.0 / (1 + opus_exp(-a));
}

void opus_init_sigmoid_lookup(void)
{
	if (!sigmoid_is_inited_) {
		double f = (SIGMOID_RANGE_MAX - SIGMOID_RANGE_MIN) / SIGMOID_LOOKUP_SIZE;
		int    i;

		for (i = 0; i < SIGMOID_LOOKUP_SIZE; ++i) {
			sigmoid_lookup_table_[i] = opus_sigmoid(SIGMOID_RANGE_MIN + f * i);
		}

		sigmoid_interval_  = SIGMOID_LOOKUP_SIZE / (SIGMOID_RANGE_MAX - SIGMOID_RANGE_MIN);
		sigmoid_is_inited_ = 1;
	}
}

opus_real opus_sigmoid_cached(opus_real a)
{
	uint64_t j;

	if (a < SIGMOID_RANGE_MIN) return sigmoid_lookup_table_[0];
	if (a >= SIGMOID_RANGE_MAX) return sigmoid_lookup_table_[SIGMOID_LOOKUP_SIZE - 1];

	j = (uint64_t) ((a - SIGMOID_RANGE_MIN) * sigmoid_interval_ + 0.5);

	if (OPUS_UNLIKELY(j >= SIGMOID_LOOKUP_SIZE)) return sigmoid_lookup_table_[SIGMOID_LOOKUP_SIZE - 1];

	return sigmoid_lookup_table_[j];
}

opus_real opus_max(opus_real a, opus_real b) { return a > b ? a : b; }

opus_real opus_min(opus_real a, opus_real b) { return a < b ? a : b; }

float opus_min_f(float a, float b) { return a < b ? a : b; }

float opus_max_f(float a, float b) { return a > b ? a : b; }

double opus_max_d(double a, double b) { return a > b ? a : b; }

double opus_min_d(double a, double b) { return a < b ? a : b; }

int opus_max_i(int a, int b) { return a > b ? a : b; }

int opus_min_i(int a, int b) { return a < b ? a : b; }

int opus_sign(opus_real a) { return (a > 0 ? 1 : (a < 0 ? -1 : 0)); }

opus_real opus_map(opus_real x, opus_real x_min, opus_real x_max, opus_real min, opus_real max)
{
	return (x - x_min) * (max - min) / (x_max - x_min) + min;
}

opus_real opus_clamp(opus_real num, opus_real min, opus_real max)
{
	return opus_max(min, opus_min(num, max));
}

int opus_clamp_i(int num, int min, int max)
{
	if (num < min) return min;
	if (num > max) return max;
	return num;
}

/*
 * the meaning of:
 * 	- loro (a, b)
 * 	- lcro [a, b)
 * 	- lorc (a, b]
 * 	- lcrc [a, b]
 */

int opus_rand() { return rand(); }

int opus_rand_loro(int min, int max) { return opus_rand() % (max - min + 1) + min - 1; }

int opus_rand_lorc(int min, int max) { return opus_rand() % (max - min) + min + 1; }

int opus_rand_lcro(int min, int max) { return opus_rand() % (max - min) + min; }

int opus_rand_lcrc(int min, int max) { return opus_rand() % (max - min + 1) + min; }

/* random number [0, 1] */
opus_real opus_rand_01() { return opus_rand() / (double) RAND_MAX; }

/* random number in range [-1,1] */
opus_real opus_rand_m11() { return 2.0 * opus_rand() / (double) RAND_MAX - 1.0; }

void opus_vec2_set(opus_vec2 *vec, opus_real x, opus_real y)
{
	vec->x = x;
	vec->y = y;
}

opus_vec2 opus_vec2_(opus_real x, opus_real y)
{
	opus_vec2 v;
	v.x = x;
	v.y = y;
	return v;
}

opus_vec3 opus_vec3_(opus_real x, opus_real y, opus_real z)
{
	opus_vec3 v;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

opus_vec3 opus_vec3_cross(opus_vec3 a, opus_vec3 b)
{
	opus_vec3 r;
	r.x = a.y * b.z - a.z * b.y;
	r.y = a.z * b.x - a.x * b.z;
	r.z = a.x * b.y - a.y * b.x;
	return r;
}

int opus_vec2_equal_(opus_vec2 vec_a, opus_vec2 vec_b, opus_real epsilon)
{
	return opus_fuzzy_equal(vec_a.x, vec_b.x, epsilon) && opus_fuzzy_equal(vec_a.y, vec_b.y, epsilon);
}

int opus_vec2_equal(opus_vec2 vec_a, opus_vec2 vec_b)
{
	return opus_vec2_equal_(vec_a, vec_b, OPUS_REAL_EPSILON);
}

opus_vec2 opus_vec2_add(opus_vec2 x, opus_vec2 to_add)
{
	return opus_vec2_(x.x + to_add.x, x.y + to_add.y);
}

opus_vec2 opus_vec2_sub(opus_vec2 x, opus_vec2 to_sub)
{
	return opus_vec2_(x.x - to_sub.x, x.y - to_sub.y);
}

opus_vec2 opus_vec2_to(opus_vec2 x, opus_vec2 to)
{
	return opus_vec2_(to.x - x.x, to.y - x.y);
}

opus_vec2 opus_vec2_to_xy(opus_vec2 vec, opus_real x, opus_real y)
{
	return opus_vec2_to(vec, opus_vec2_(x, y));
}

opus_vec2 opus_vec2_scale(opus_vec2 a, opus_real scalar)
{
	return opus_vec2_(a.x * scalar, a.y * scalar);
}

opus_vec2 opus_vec2_div(opus_vec2 a, opus_real d)
{
	opus_real inv_d = 1.0 / d;
	return opus_vec2_scale(a, inv_d);
}

opus_real opus_vec2_dot(opus_vec2 a, opus_vec2 b) { return a.x * b.x + a.y * b.y; }

opus_real opus_vec2_cross(opus_vec2 a, opus_vec2 b) { return a.x * b.y - b.x * a.y; }

opus_real opus_vec2_len2(opus_vec2 a) { return opus_vec2_dot(a, a); }

opus_real opus_vec2_len(opus_vec2 a) { return opus_sqrt(opus_vec2_len2(a)); }

opus_vec2 opus_vec2_norm(opus_vec2 a)
{
	opus_real len_inv = opus_inv_sqrt(opus_vec2_len2(a), 3);
	return opus_vec2_scale(a, len_inv);
}

/* return b */
opus_vec2 opus_vec2_copy(opus_vec2 *a, opus_vec2 b)
{
	*a = b;
	return b;
}

opus_vec2 opus_vec2_neg(opus_vec2 a) { return opus_vec2_inv(a); }

opus_vec2 opus_vec2_inv(opus_vec2 a) { return opus_vec2_(-a.x, -a.y); }

opus_real opus_vec2_get_angle(opus_vec2 a) { return opus_atan2(a.y, a.x); }

opus_vec2 opus_vec2_set_angle(opus_vec2 *a, opus_real radian)
{
	opus_real len = opus_vec2_len(*a);
	opus_vec2_set(a, len * opus_cos(radian), len * opus_sin(radian));
	return *a;
}

opus_real opus_vec2_get_length(opus_vec2 a) { return opus_vec2_len(a); }

opus_vec2 opus_vec2_set_length(opus_vec2 *a, opus_real len)
{
	opus_real angle = opus_vec2_get_angle(*a);
	opus_vec2_set(a, len * opus_cos(angle), len * opus_sin(angle));
	return *a;
}

/**
 * @return the angle between the vector `b - a` and the x-axis in radians.
 */
opus_real opus_vec2_angle(opus_vec2 a, opus_vec2 b)
{
	return opus_vec2_get_angle(opus_vec2_to(a, b));
}

opus_real opus_vec2_dist2(opus_vec2 a, opus_vec2 b)
{
	opus_vec2 c = opus_vec2_sub(a, b);
	return opus_vec2_dot(c, c);
}

opus_real opus_vec2_dist(opus_vec2 a, opus_vec2 b) { return opus_sqrt(opus_vec2_dist2(a, b)); }

void opus_vec2_swap(opus_vec2 *a, opus_vec2 *b)
{
	opus_vec2 t;
	t  = *a;
	*a = *b;
	*b = t;
}

/**
 * @brief lerp ratio of the origin onto the vector h - t
 * @see http://www.geometrictools.com/Documentation/DistancePointLine.pdf
 */
opus_real opus_vec2_lerp_ratio(opus_vec2 tail, opus_vec2 head)
{
	opus_real t;
	opus_vec2 M;
	tail = opus_vec2_neg(tail);
	M    = opus_vec2_sub(head, tail);
	t    = opus_vec2_dot(M, tail) / opus_vec2_len2(M);
	return opus_clamp(t, 0.0, 1.0);
}

/**
 * linearly interpolate between two vectors
 */
opus_vec2 opus_vec2_lerp(opus_vec2 head, opus_vec2 tail, opus_real t)
{
	opus_vec2 p;
	p = opus_vec2_scale(opus_vec2_to(head, tail), t);
	return opus_vec2_add(head, p);
}

opus_vec2 opus_vec2_project(opus_vec2 vec, opus_vec2 axis)
{
	return opus_vec2_scale(axis, opus_vec2_dot(vec, axis) / opus_vec2_len2(axis));
}

/**
 * @brief Project this vector onto a vector of unit length. This is slightly more efficient
 * 		than `opus_vec2_project` when dealing with unit vectors.
 * @param axis The unit vector to project onto.
 */
opus_vec2 opus_vec2_projectN(opus_vec2 vec, opus_vec2 axis)
{
	return opus_vec2_scale(axis, opus_vec2_dot(vec, axis));
}

/**
 * @brief Reflect this vector on an arbitrary axis (represented by a unit vector). This is
 * 	slightly more efficient than `reflect` when dealing with an axis that is a unit vector.
 * @param {Vector} axis The unit vector representing the axis.
 */
opus_vec2 opus_vec2_reflectN(opus_vec2 vec, opus_vec2 axis)
{
	opus_vec2 res;
	res = opus_vec2_projectN(vec, axis);
	res = opus_vec2_scale(res, 2);
	return opus_vec2_sub(res, vec);
}

/**
 * @brief Reflect this vector on an arbitrary axis.
 * @param {Vector} axis The vector representing the axis.
 */
opus_vec2 opus_vec2_reflect(opus_vec2 vec, opus_vec2 axis)
{
	opus_vec2 res;
	res = opus_vec2_project(vec, axis);
	res = opus_vec2_scale(res, 2);
	return opus_vec2_sub(res, vec);
}

opus_vec2 opus_vec2_rotate(opus_vec2 vec, opus_real rad)
{
	opus_real x = vec.x, y = vec.y, c = opus_cos(rad), s = opus_sin(rad);
	return opus_vec2_(x * c - y * s, x * s + y * c);
}

opus_vec2 opus_vec2_rotate_about(opus_vec2 vec, opus_vec2 point, opus_real rad)
{
	opus_vec2 res;
	opus_real c = opus_cos(rad), s = opus_sin(rad);
	opus_real x = point.x + ((vec.x - point.x) * c - (vec.y - point.y) * s);
	res.y       = point.y + ((vec.x - point.x) * s + (vec.y - point.y) * c);
	res.x       = x;
	return res;
}

/**
 * @brief Change this vector to be perpendicular to what it was before. (Effectively
 * 		rotates it 90 degrees in a clockwise direction)
 */
opus_vec2 opus_vec2_perp(opus_vec2 v) { return opus_vec2_(v.y, -v.x); }

/**
 * transpose skew a vector to be perp with itself (clockwise rotation)
 */
opus_vec2 opus_vec2_skewT(opus_vec2 v) { return opus_vec2_perp(v); }

/**
 * skew a vector to be perp with itself (counter-clockwise rotation)
 */
opus_vec2 opus_vec2_skew(opus_vec2 v) { return opus_vec2_(-v.y, v.x); }

opus_real opus_vec2_cross3(opus_vec2 a, opus_vec2 b, opus_vec2 c)
{
	return ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x));
}

static opus_real ele__(opus_real *mat, uint16_t column, int i, int j)
{
	return mat[i * column + j];
}

static void ele_set__(opus_real *mat, uint16_t column, int i, int j, opus_real value)
{
	mat[i * column + j] = value;
}

/**
 * @brief set a matrix to identity matrix
 * @param mat [n*m]
 * @param n
 * @param m
 */
void opus_mat_identity(opus_real *mat, uint16_t n, uint16_t m)
{
	uint16_t i, j;
	memset(mat, 0, n * m * sizeof(opus_real));
	for (i = 0; i < n; i++)
		for (j = 0; j < m; j++)
			if (i == j) ele_set__(mat, m, i, j, 1);
}

/**
 * @brief square matrix fast power (inplace)
 * @param A [n*n]
 * @param n
 */
void opus_mat_fast_pow(opus_real *A, uint64_t n)
{
	opus_real  temp[OPUS_MAX_STATIC_MATRIX_DIMENSION * OPUS_MAX_STATIC_MATRIX_DIMENSION];
	opus_real *res = temp;

	int flag = 0;

	if (n > OPUS_MAX_STATIC_MATRIX_DIMENSION) {
		flag = 1;
		res  = (opus_real *) malloc(sizeof(opus_real) * n * n);
	}

	opus_mat_identity(res, n, n);

	while (n) {
		if (n & 1) opus_mat_mul(res, A, res, n, n, n);
		opus_mat_mul(A, A, A, n, n, n);
		n >>= 1;
	}

	memcpy(A, res, sizeof(opus_real) * n * n);

	if (flag) free(res);
}

/**
 * @brief
 * @param mat [n*m]
 * @param n
 * @param m
 */
void opus_mat_print(opus_real *mat, int n, int m)
{
	int iy, ix;
	for (iy = 0; iy < n; iy++) {
		for (ix = 0; ix < m; ix++) printf("%lf\t", mat[iy * m + ix]);
		printf("\n");
	}
	printf("\n");
}

/**
 * @brief calculate A + B, and store the result in A
 * @param A [n*m]
 * @param B [n*m]
 */
void mat_add(opus_real *A, opus_real *B, uint16_t n, uint16_t m)
{
	uint16_t i;
	for (i = 0; i < n * m; i++) A[i] += B[i];
}

/**
 * @brief calculate A - B, and store the result in A
 * @param A [n*m]
 * @param B [n*m]
 */
void opus_mat_sub(opus_real *A, opus_real *B, uint16_t n, uint16_t m)
{
	uint16_t i;
	for (i = 0; i < n * m; i++) A[i] -= B[i];
}

/**
 * @brief calculate kA, and store the result in A
 * @param A [n*m]
 */
void opus_mat_scale(opus_real *A, opus_real k, uint16_t n, uint16_t m)
{
	uint16_t i;
	for (i = 0; i < n * m; i++) A[i] *= k;
}

/**
 * @brief perform LU-decomposition with partial pivoting in a square matrix \n\n

 * Not all matrices can be LU decomposed.\n
 * The matrices that can be LU decomposed need to meet the following three conditions:\n
 * 		1` The matrix is square matrix (LU decomposition is mainly for square matrix);\n
 * 		2` The matrix is invertible, that is, the matrix is a full rank matrix, and each row is an\n
 *			independent vector;\n
 * 		3` There is no zero principal element in the elimination process, \n
 * 			that is, there is no elementary transformation of row exchange in the elimination\n
 *			process. However, you can left multiply a P matrix to A, PA = LU.
 * @param A [n*n]
 * @param LU [n*n]
 * @param P [n]
 * @param n
 * @return 1 if successful
 * @reference https://zhuanlan.zhihu.com/p/602486157 elementary transformation of matrix
 * 			  https://zhuanlan.zhihu.com/p/363948873 LU-decomposition
 */
uint8_t opus_mat_lup(opus_real *A, opus_real *LU, uint8_t *P, uint16_t n)
{
	uint16_t ind_max, tmp_int;
	uint16_t i, j, k;

	/* if not the same */
	if (A != LU) memcpy(LU, A, n * n * sizeof(opus_real));

	/* create the pivot vector */
	for (i = 0; i < n; ++i) P[i] = i;

	for (i = 0; i < n - 1; ++i) {
		ind_max = i;
		for (j = i + 1; j < n; ++j)
			if (opus_abs(LU[n * P[j] + i]) > opus_abs(LU[n * P[ind_max] + i]))
				//			if (r_abs(ele__(LU, n, P[j], i)) > r_abs(ele__(LU, n, P[ind_max], i)))
				ind_max = j;

		tmp_int    = P[i];
		P[i]       = P[ind_max];
		P[ind_max] = tmp_int;

		if (opus_equal(LU[n * P[i] + i], 0)) return 0; /* matrix is singular (up to tolerance) */

		for (j = i + 1; j < n; ++j) {
			LU[n * P[j] + i] = LU[n * P[j] + i] / LU[n * P[i] + i];

			for (k = i + 1; k < n; ++k)
				LU[n * P[j] + k] = LU[n * P[j] + k] - LU[n * P[i] + k] * LU[n * P[j] + i];
		}
	}

	return 1; /* solved */
}

/**
 * @brief compute the determinant of square matrix
 * @param A [n*n]
 * @param n
 * @return determinant value, or 0 for singular matrix
 */
opus_real opus_mat_det(opus_real *A, uint16_t n)
{
	opus_real determinant = 1.f;
	opus_real
	         LU[OPUS_MAX_STATIC_MATRIX_DIMENSION * OPUS_MAX_STATIC_MATRIX_DIMENSION]; /* row * row */
	uint8_t  P[OPUS_MAX_STATIC_MATRIX_DIMENSION];                                     /* row */
	uint8_t  status = opus_mat_lup(A, LU, P, n);
	uint16_t i, j;

	if (status == 0) return 0; /* matrix is singular */

	for (i = 0; i < n; ++i) determinant *= LU[n * P[i] + i];

	j = 0;
	for (i = 0; i < n; ++i)
		if (P[i] != i) ++j;

	if (j && (j - 1) % 2 == 1) determinant = -determinant;

	return determinant;
}

/**
 * @brief transpose matrix inplace
 * @param A [n * m]
 * @param n
 * @param m
 */
void opus_mat_transpose(opus_real *A, uint16_t n, uint16_t m)
{
	opus_real  B[OPUS_MAX_STATIC_MATRIX_DIMENSION *
                OPUS_MAX_STATIC_MATRIX_DIMENSION]; /* row * column */
	opus_real *transpose;
	opus_real *e = A;

	uint16_t i, j;

	for (i = 0; i < n; i++) {
		transpose = &B[i];
		for (j = 0; j < m; j++) {
			*transpose = *e;
			e++;
			transpose += n;
		}
	}

	memcpy(A, B, n * m * sizeof(opus_real));
}

static uint8_t solve_inv(opus_real *A, opus_real *x, const opus_real *b, const uint8_t *P,
                         opus_real *LU, uint16_t row)
{
	/* forward substitution with pivoting */
	int32_t i, j;
	for (i = 0; i < row; ++i) {
		x[i] = b[P[i]];

		for (j = 0; j < i; ++j) x[i] = x[i] - LU[row * P[i] + j] * x[j];
	}

	/* backward substitution with pivoting */
	for (i = row - 1; i >= 0; --i) {
		for (j = i + 1; j < row; ++j) x[i] = x[i] - LU[row * P[i] + j] * x[j];

		/* Just in case if we divide with zero */
		if (opus_abs(LU[row * P[i] + i]) > OPUS_REAL_EPSILON) x[i] = x[i] / LU[row * P[i] + i];
		else
			return 0;
	}

	return 1; /* No problems */
}

/**
 * @brief invert a matrix, a very expensive operation. It is better to unfold the loop if you
 * 		already know the row of the matrix
 * @param A should be square matrix
 * @param row
 * @return 1 if succeeds
 */
uint8_t opus_mat_inv(opus_real *A, uint16_t row)
{
	uint16_t i;

	/* Create iA matrix */
	opus_real
	        iA[OPUS_MAX_STATIC_MATRIX_DIMENSION * OPUS_MAX_STATIC_MATRIX_DIMENSION]; /* row * row */

	opus_real tmp_vec[OPUS_MAX_STATIC_MATRIX_DIMENSION]; /* row */
	uint8_t   status = 0;

	opus_real
	        LU[OPUS_MAX_STATIC_MATRIX_DIMENSION * OPUS_MAX_STATIC_MATRIX_DIMENSION]; /* row * row */
	uint8_t P[OPUS_MAX_STATIC_MATRIX_DIMENSION];                                     /* row */

	/* Create temporary matrix and status variable */
	memset(tmp_vec, 0, row * sizeof(opus_real));

	/* Check if the determinant is 0 */
	status = opus_mat_lup(A, LU, P, row);
	if (status == 0) return 0; /* matrix is singular. Determinant 0 */
	/* Create the inverse */
	for (i = 0; i < row; i++) {
		tmp_vec[i] = 1.0f;
		if (!solve_inv(A, &iA[row * i], tmp_vec, P, LU, row)) return 0; /* We divided with zero */
		tmp_vec[i] = 0.0f;
	}

	/* Transpose of iA */
	opus_mat_transpose(iA, row, row);

	/* Copy over iA -> A */
	memcpy(A, iA, row * row * sizeof(opus_real));

	return status;
}

/**
 * @brief multiply two matrix, C can be output into A or B while not messing up the result
 * @param A [n*r]
 * @param B [r*m]
 * @param C [n*m], equals to "A * B"
 * @param n
 * @param r
 * @param m
 */
void opus_mat_mul(opus_real *A, opus_real *B, opus_real *C, uint16_t n, uint16_t r, uint16_t m)
{
	opus_real *ea;
	opus_real *eb;
	opus_real  temp[OPUS_MAX_STATIC_MATRIX_DIMENSION * OPUS_MAX_STATIC_MATRIX_DIMENSION];
	opus_real *p_temp = temp;

	uint16_t i, j, k;

	for (i = 0; i < n; i++) {
		for (j = 0; j < m; j++) {
			ea = &A[i * r]; /* first element of each row in A */
			eb = &B[j];     /* first element of each column in B */

			*p_temp = 0;
			for (k = 0; k < r; k++) {
				*p_temp += *ea * *eb;
				ea++;
				eb += m;
			}
			p_temp++;
		}
	}

	memcpy(C, temp, sizeof(opus_real) * n * m);
}

/**
 * @brief Householder QR-decomposition
 * @param A [row_a * column_a]
 * @param Q [row_a * row_a]
 * @param R [row_a * column_a]
 * @param r
 * @param c
 * @param r_only
 * @return 1 if succeeds
 */
uint8_t opus_mat_qr(opus_real *A, opus_real *Q, opus_real *R, uint16_t r, uint16_t c,
                    int r_only)
{
	/* Declare */
	uint16_t  row_a_row_a = r * r;
	uint16_t  l           = r - 1 < c ? r - 1 : c;
	uint16_t  i, j, k;
	uint8_t   status;
	opus_real s, Rk, rr;

	opus_real W[OPUS_MAX_STATIC_MATRIX_DIMENSION]; /* row_a */
	opus_real WW[OPUS_MAX_STATIC_MATRIX_DIMENSION *
	             OPUS_MAX_STATIC_MATRIX_DIMENSION]; /* row_a_row_a */
	opus_real Hi[OPUS_MAX_STATIC_MATRIX_DIMENSION *
	             OPUS_MAX_STATIC_MATRIX_DIMENSION]; /* row_a_row_a */
	opus_real H[OPUS_MAX_STATIC_MATRIX_DIMENSION *
	            OPUS_MAX_STATIC_MATRIX_DIMENSION]; /* row_a_row_a */
	opus_real HiH[OPUS_MAX_STATIC_MATRIX_DIMENSION *
	              OPUS_MAX_STATIC_MATRIX_DIMENSION]; /*row_a_row_a */
	opus_real HiR[OPUS_MAX_STATIC_MATRIX_DIMENSION *
	              OPUS_MAX_STATIC_MATRIX_DIMENSION]; /*row_a * column_a */

	/* Give A to R */
	memcpy(R, A, r * c * sizeof(opus_real));

	/* Turn H into identity matrix */
	memset(H, 0, row_a_row_a * sizeof(opus_real));
	for (i = 0; i < r; i++) H[r * i + i] = 1.0f;

	/* Do house holder transformations */
	for (k = 0; k < l; k++) {
		/* Do L2 norm */
		s = 0;
		for (i = k; i < r; i++) s += R[i * c + k] * R[i * c + k];
		s = opus_sqrt(s);

		/* Find Rk */
		Rk = R[k * c + k];

		/* Do sign */
		if (Rk < 0.0f) s = -s;

		/* Compute r */
		rr = opus_sqrt(2 * s * (Rk + s)); /* FIXME */

		/* Fill W */
		memset(W, 0, r * sizeof(opus_real));
		W[k] = (Rk + s) / rr;
		for (i = k + 1; i < r; i++) W[i] = R[i * c + k] / rr;

		/* WW = W*W' */
		opus_mat_mul(W, W, WW, r, 1, r);

		/* Fill Hi matrix */
		for (i = 0; i < row_a_row_a; i++) Hi[i] = -2.0f * WW[i];

		/* Use identity matrix on Hi */
		for (i = 0; i < r; i++) Hi[i * r + i] += 1.0f;

		/* HiH = Hi * H -> HiH = H */
		if (!r_only) {
			opus_mat_mul(Hi, H, HiH, r, r, r);
			memcpy(H, HiH, row_a_row_a * sizeof(opus_real));
		}

		/* HiR = Hi * R -> HiR = R */
		opus_mat_mul(Hi, R, HiR, r, r, c);
		memcpy(R, HiR, r * c * sizeof(opus_real));
	}

	/* Do inverse on H and give it to Q */
	status = 1;
	if (!r_only) {
		status = opus_mat_inv(H, r);
		memcpy(Q, H, row_a_row_a * sizeof(opus_real));
	}
	return status;
}

/**
 * @brief solve Ax=b
 * @param A an upper triangular matrix
 * @param x
 * @param b
 * @param row
 */
void opus_linear_solve_ut(const opus_real *A, opus_real *x, opus_real *b, uint16_t row)
{
	opus_real sum;
	uint16_t  i, j;
	memset(x, 0, row * sizeof(opus_real));
	for (i = row - 1; i >= 0; i--) { /* Column */
		sum = 0;                     /* This is our sum */
		for (j = i; j < row; j++) {  /* Row */
			sum += A[i * row + j] * x[j];
		}
		x[i] = (b[i] - sum) / A[i * row + i];

		/* For backwards unsigned for-loops, important because uint16 i = -1 is actually 65535 */
		if (i == 0) break;
	}
}

/**
 * @brief solve with forward substitution. This can be used with Cholesky decomposition
 * @param A need to be lower triangular and square
 * @param x
 * @param b
 * @param row
 */
void opus_linear_solve_lt(const opus_real *A, opus_real *x, opus_real *b, uint16_t row)
{
	/* Time to solve x from Ax = b. */
	opus_real sum;
	uint16_t  i, j;
	memset(x, 0, row * sizeof(opus_real));
	for (i = 0; i < row; i++) {
		sum = 0;
		for (j = 0; j < i; j++) sum = sum + A[row * i + j] * x[j];

		x[i] = (b[i] - sum) / A[row * i + i];
	}
}

/**
 * @brief solve AX=b using QR-decomposition
 * @param A [row*column]
 * @param x [column]
 * @param b [row]
 * @param row
 * @param column
 */
void opus_linear_solve_qr(opus_real *A, opus_real *x, opus_real *b, uint16_t row, uint16_t column)
{
	/* QR-decomposition */
	opus_real Q[10 * 10]; /* row * row */
	opus_real R[10 * 10]; /* row * column */
	opus_real QTb[10];    /* row */
	opus_mat_qr(A, Q, R, row, column, 0);
	opus_mat_transpose(Q, row, row);      /* Do transpose Q -> Q^T */
	opus_mat_mul(Q, b, QTb, row, row, 1); /* Q^Tb = Q^T*b */
	opus_linear_solve_ut(R, x, QTb, column);
}

void opus_mat2d_identity(opus_mat2d mat)
{
	mat[0] = 1.0f;
	mat[1] = 0.0f;
	mat[2] = 0.0f;
	mat[3] = 1.0f;
	mat[4] = 0.0f;
	mat[5] = 0.0f;
}

/**
 * @brief construct a translation matrix
 * @param mat
 * @param tx
 * @param ty
 */
void opus_mat2d_translate(opus_mat2d mat, float tx, float ty)
{
	mat[0] = 1.0f;
	mat[1] = 0.0f;
	mat[2] = 0.0f;
	mat[3] = 1.0f;
	mat[4] = tx;
	mat[5] = ty;
}

/**
 * @brief construct a scaling matrix
 * @param mat
 * @param sx
 * @param sy
 */
void opus_mat2d_scale(opus_mat2d mat, float sx, float sy)
{
	mat[0] = sx;
	mat[1] = 0.0f;
	mat[2] = 0.0f;
	mat[3] = sy;
	mat[4] = 0.0f;
	mat[5] = 0.0f;
}

/**
 * @brief construct a rotation matrix
 * @param mat
 * @param a
 */
void opus_mat2d_rotate(opus_mat2d mat, float a)
{
	float cs = cosf(a), sn = sinf(a);
	mat[0] = cs;
	mat[1] = sn;
	mat[2] = -sn;
	mat[3] = cs;
	mat[4] = 0.0f;
	mat[5] = 0.0f;
}

void opus_mat2d_rotate_about(opus_mat2d mat, float a, opus_vec2 p)
{
	float cs = cosf(a), sn = sinf(a);
	mat[0] = cs;
	mat[1] = sn;
	mat[2] = -sn;
	mat[3] = cs;
	mat[4] = (float) p.x;
	mat[5] = (float) p.y;
}

/**
 * @brief construct a X-skewed matrix
 * @param mat
 * @param a
 */
void opus_mat2d_skew_x(opus_mat2d mat, float a)
{
	mat[0] = 1.0f;
	mat[1] = 0.0f;
	mat[2] = tanf(a);
	mat[3] = 1.0f;
	mat[4] = 0.0f;
	mat[5] = 0.0f;
}

/**
 * @brief construct a Y-skewed matrix
 * @param mat
 * @param a
 */
void opus_mat2d_skew_y(opus_mat2d mat, float a)
{
	mat[0] = 1.0f;
	mat[1] = tanf(a);
	mat[2] = 0.0f;
	mat[3] = 1.0f;
	mat[4] = 0.0f;
	mat[5] = 0.0f;
}

void opus_mat2d_mul(opus_mat2d mat, const opus_mat2d to_mult)
{
	float t0, t2, t4;

	t0 = mat[0] * to_mult[0] + mat[1] * to_mult[2];
	t2 = mat[2] * to_mult[0] + mat[3] * to_mult[2];
	t4 = mat[4] * to_mult[0] + mat[5] * to_mult[2] + to_mult[4];

	mat[1] = mat[0] * to_mult[1] + mat[1] * to_mult[3];
	mat[3] = mat[2] * to_mult[1] + mat[3] * to_mult[3];
	mat[5] = mat[4] * to_mult[1] + mat[5] * to_mult[3] + to_mult[5];
	mat[0] = t0;
	mat[2] = t2;
	mat[4] = t4;
}

void opus_mat2d_pre_mul(opus_mat2d mat, opus_mat2d to_mult)
{
	opus_mat2d s2;
	memcpy(s2, to_mult, sizeof(float) * 6);
	opus_mat2d_mul(s2, mat);
	memcpy(mat, s2, sizeof(float) * 6);
}

/* invert matrix "t" and store the result in "inv". return 0 if successful */
int opus_mat2d_inv(opus_mat2d inv, const opus_mat2d t)
{
	double inv_det, det = t[0] * t[3] - t[2] * t[1];
	if (opus_equal(det, 0)) {
		opus_mat2d_identity(inv);
		return 1;
	}
	inv_det = 1.0 / det;
	inv[0]  = (float) (t[3] * inv_det);
	inv[2]  = (float) (-t[2] * inv_det);
	inv[4]  = (float) ((t[2] * t[5] - t[3] * t[4]) * inv_det);
	inv[1]  = (float) (-t[1] * inv_det);
	inv[3]  = (float) (t[0] * inv_det);
	inv[5]  = (float) ((t[1] * t[4] - t[0] * t[5]) * inv_det);
	return 0;
}

/**
 * @brief https://blog.csdn.net/qq_31473097/article/details/121412135
 * @param dx
 * @param dy
 * @param mat
 * @param sx
 * @param sy
 */
void opus_mar2d_pre_mul_xy(opus_real *dx, opus_real *dy, const opus_mat2d mat, opus_real sx,
                           opus_real sy)
{
	*dx = sx * mat[0] + sy * mat[2] + mat[4];
	*dy = sx * mat[1] + sy * mat[3] + mat[5];
}

opus_vec2 opus_mat2d_pre_mul_vec(opus_mat2d mat, opus_vec2 src)
{
	opus_vec2 p;
	opus_mar2d_pre_mul_xy(&p.x, &p.y, mat, src.x, src.y);
	return p;
}

void opus_mat2d_copy(opus_mat2d dst, opus_mat2d src)
{
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
	dst[3] = src[3];
	dst[4] = src[4];
	dst[5] = src[5];
}
