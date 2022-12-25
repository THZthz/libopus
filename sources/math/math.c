
#include "math/math.h"
#include "utils/utils.h"
#include <string.h>

#define SIGMOID_LOOKUP_SIZE (4096)
#define SIGMOID_RANGE_MIN (-15.0)
#define SIGMOID_RANGE_MAX (15.0)

static int sigmoid_is_inited_   = 0;
static double sigmoid_interval_ = 0.12;
static double sigmoid_lookup_table_[SIGMOID_LOOKUP_SIZE];
static unsigned int r_seed = 0x11223344;

unsigned int r_xrand(void)
{
	return (((r_seed = r_seed * 214013L + 2531011L) >> 16) & 0x7fffffff);
}

real r_round_n(real number, unsigned int bits)
{
	int64_t int_part = (int64_t) number;

	number -= (real) int_part;
	number *= r_pow(10, bits);
	number = (real) ((int64_t) (number + 0.5));
	number *= r_pow(1.0 / 10.0, bits);
	return (real) int_part + number;
}

real r_sigmoid(real a)
{
	if (a < -45.0) return 0;
	if (a > 45.0) return 1;
	return 1.0 / (1 + exp(-a));
}

void r_init_sigmoid_lookup(void)
{
	if (!sigmoid_is_inited_)
	{
		double f = (SIGMOID_RANGE_MAX - SIGMOID_RANGE_MIN) / SIGMOID_LOOKUP_SIZE;
		int i;

		for (i = 0; i < SIGMOID_LOOKUP_SIZE; ++i)
		{
			sigmoid_lookup_table_[i] = r_sigmoid(SIGMOID_RANGE_MIN + f * i);
		}

		sigmoid_interval_  = SIGMOID_LOOKUP_SIZE / (SIGMOID_RANGE_MAX - SIGMOID_RANGE_MIN);
		sigmoid_is_inited_ = 1;
	}
}

real r_sigmoid_cached(real a)
{
	uint64_t j;

	if (a < SIGMOID_RANGE_MIN) return sigmoid_lookup_table_[0];
	if (a >= SIGMOID_RANGE_MAX) return sigmoid_lookup_table_[SIGMOID_LOOKUP_SIZE - 1];

	j = (uint64_t) ((a - SIGMOID_RANGE_MIN) * sigmoid_interval_ + 0.5);

	if (UNLIKELY(j >= SIGMOID_LOOKUP_SIZE)) return sigmoid_lookup_table_[SIGMOID_LOOKUP_SIZE - 1];

	return sigmoid_lookup_table_[j];
}

vec2 vec2_(real x, real y)
{
	vec2 v;
	v.x = x;
	v.y = y;
	return v;
}

vec3 vec3_(real x, real y, real z)
{
	vec3 v;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

void vec3_mul_cross(vec3 r, vec3 a, vec3 b)
{
	r.x = a.y * b.z - a.z * b.y;
	r.y = a.z * b.x - a.x * b.z;
	r.z = a.x * b.y - a.y * b.x;
}

void mat_identity(real *mat, int y, int x)
{
	int iy, ix;
	for (iy = 0; iy < y; iy++)
		for (ix = 0; ix < x; ix++)
			mat[iy * x + ix] = ix == iy ? 1 : 0;
}

void mat_print(real *mat, int y, int x)
{
	int iy, ix;
	for (iy = 0; iy < y; iy++) {
		for (ix = 0; ix < x; ix++)
			printf("%lf\t", mat[iy * x + ix]);
		printf("\n");
	}
	printf("\n");
}

/**
 * Do LU-decomposition with partial pivoting
 * A [m*n]
 * LU [m*n]
 * P [n]
 * n == m
 * Returns 1 == Success
 * Returns 0 == Fail
 */
uint8_t linear_lup(real A[], real LU[], uint8_t P[], uint16_t row)
{
	/* Variables */
	uint16_t ind_max, tmp_int;
	uint16_t i, j, k;

	/* If not the same */
	if (A != LU) memcpy(LU, A, row * row * sizeof(real));

	/* Create the pivot vector */
	for (i = 0; i < row; ++i)
		P[i] = i;

	for (i = 0; i < row - 1; ++i) {
		ind_max = i;
		for (j = i + 1; j < row; ++j)
			if (r_abs(LU[row * P[j] + i]) > r_abs(LU[row * P[ind_max] + i]))
				ind_max = j;

		tmp_int    = P[i];
		P[i]       = P[ind_max];
		P[ind_max] = tmp_int;

		if (r_abs(LU[row * P[i] + i]) < REAL_EPSILON)
			return 0; /* matrix is singular (up to tolerance) */

		for (j = i + 1; j < row; ++j) {
			LU[row * P[j] + i] = LU[row * P[j] + i] / LU[row * P[i] + i];

			for (k = i + 1; k < row; ++k)
				LU[row * P[j] + k] = LU[row * P[j] + k] - LU[row * P[i] + k] * LU[row * P[j] + i];
		}
	}

	return 1; /* Solved */
}

/**
 * Computes the determinant of square matrix A
 * A [m*n]
 * n == m
 * Return: Determinant value, or 0 for singular matrix
 */
real det(real A[], uint16_t row)
{
	real     determinant = 1.f;
	real     LU[10 * 10]; /* row * row */
	uint8_t  P[10];       /* row */
	uint8_t  status = linear_lup(A, LU, P, row);
	uint16_t i, j;
	if (status == 0)
		return 0; /* matrix is singular */

	for (i = 0; i < row; ++i)
		determinant *= LU[row * P[i] + i];

	j = 0;
	for (i = 0; i < row; ++i)
		if (P[i] != i)
			++j;

	if (j && (j - 1) % 2 == 1)
		determinant = -determinant;

	return determinant;
}

/*
 * Turn A into transpose A^T
 */
void mat_transpose(real A[], uint16_t row, uint16_t column)
{

	real  B[10 * 10]; /* row * column */
	real *transpose;
	real *ptr_A = A;

	uint16_t i, j;
	for (i = 0; i < row; i++) {
		transpose = &B[i];
		for (j = 0; j < column; j++) {
			*transpose = *ptr_A;
			ptr_A++;
			transpose += row;
		}
	}

	/* Copy! */
	memcpy(A, B, row * column * sizeof(real));
}

static uint8_t solve_inv(real A[], real x[], const real b[], const uint8_t P[], real LU[], uint16_t row)
{
	/* forward substitution with pivoting */
	int32_t i, j;
	for (i = 0; i < row; ++i) {
		x[i] = b[P[i]];

		for (j = 0; j < i; ++j)
			x[i] = x[i] - LU[row * P[i] + j] * x[j];
	}

	/* backward substitution with pivoting */
	for (i = row - 1; i >= 0; --i) {
		for (j = i + 1; j < row; ++j)
			x[i] = x[i] - LU[row * P[i] + j] * x[j];

		/* Just in case if we divide with zero */
		if (r_abs(LU[row * P[i] + i]) > FLT_EPSILON)
			x[i] = x[i] / LU[row * P[i] + i];
		else
			return 0;
	}

	return 1; /* No problems */
}

/*
 * A to A^(-1)
 * Notice that only square matrices are only allowed.
 * Finding inverse should only be used for last solution.
 * Finding inverse is very cost expensive. Better to solve Ax=b instead
 * A[m*n]
 * n == m
 * Returns 1 == Success
 * Returns 0 == Fail
 */
uint8_t mat_inv(real A[], uint16_t row)
{
	uint16_t i;

	/* Create iA matrix */
	real iA[10 * 10]; /* row * row */

	real    tmp_vec[10]; /* row */
	uint8_t status = 0;

	real    LU[10 * 10]; /* row * row */
	uint8_t P[10];       /* row */

	/* Create temporary matrix and status variable */
	memset(tmp_vec, 0, row * sizeof(real));

	/* Check if the determinant is 0 */
	status = linear_lup(A, LU, P, row);
	if (status == 0)
		return 0; /* matrix is singular. Determinant 0 */
	/* Create the inverse */
	for (i = 0; i < row; i++) {
		tmp_vec[i] = 1.0f;
		if (!solve_inv(A, &iA[row * i], tmp_vec, P, LU, row))
			return 0; /* We divided with zero */
		tmp_vec[i] = 0.0f;
	}

	/* Transpose of iA */
	mat_transpose(iA, row, row);

	/* Copy over iA -> A */
	memcpy(A, iA, row * row * sizeof(real));

	return status;
}

/*
 * C = A*B
 * A [row_a*column_a]
 * B [column_a*column_b]
 * C [row_a*column_b]
 */
void mat_mul(real A[], real B[], real C[], uint16_t row_a, uint16_t column_a, uint16_t column_b)
{
	/* Data matrix */
	real    *data_a;
	real    *data_b;
	uint16_t i, j, k;

	for (i = 0; i < row_a; i++) {
		/* Then we go through every column of b */
		for (j = 0; j < column_b; j++) {
			data_a = &A[i * column_a];
			data_b = &B[j];

			*C = 0; /* Reset */
			/* And we multiply rows from a with columns of b */
			for (k = 0; k < column_a; k++) {
				*C += *data_a * *data_b;
				data_a++;
				data_b += column_b;
			}
			C++; /* ;) */
		}
	}
}

/*
 * Householder QR-decomposition
 * A [m*n]
 * Q [m*m]
 * R [m*n]
 *
 * Returns 1 == Success
 * Returns 0 == Fail
 */
uint8_t mat_qr(real *A, real *Q, real *R, uint16_t row_a, uint16_t column_a, int only_compute_R)
{
	/* Declare */
	uint16_t row_a_row_a = row_a * row_a;
	uint16_t l           = row_a - 1 < column_a ? row_a - 1 : column_a;
	uint16_t i, j, k;
	uint8_t  status;
	real     s, Rk, r,
	        W[10], /* row_a */
	        WW[10 * 10] /* row_a_row_a */,
	        Hi[10 * 10] /* row_a_row_a */,
	        H[10 * 10] /* row_a_row_a */,
	        HiH[10 * 10] /*row_a_row_a */,
	        HiR[10 * 10] /*row_a * column_a */;

	/* Give A to R */
	memcpy(R, A, row_a * column_a * sizeof(real));

	/* Turn H into identity matrix */
	memset(H, 0, row_a_row_a * sizeof(real));
	for (i = 0; i < row_a; i++)
		H[row_a * i + i] = 1.0f;

	/* Do house holder transformations */
	for (k = 0; k < l; k++) {
		/* Do L2 norm */
		s = 0;
		for (i = k; i < row_a; i++)
			s += R[i * column_a + k] * R[i * column_a + k];
		s = r_sqrt(s);

		/* Find Rk */
		Rk = R[k * column_a + k];

		/* Do sign */
		if (Rk < 0.0f)
			s = -s;

		/* Compute r */
		r = r_sqrt(2 * s * (Rk + s));

		/* Fill W */
		memset(W, 0, row_a * sizeof(real));
		W[k] = (Rk + s) / r;
		for (i = k + 1; i < row_a; i++)
			W[i] = R[i * column_a + k] / r;

		/* WW = W*W' */
		mat_mul(W, W, WW, row_a, 1, row_a);

		/* Fill Hi matrix */
		for (i = 0; i < row_a_row_a; i++)
			Hi[i] = -2.0f * WW[i];

		/* Use identity matrix on Hi */
		for (i = 0; i < row_a; i++)
			Hi[i * row_a + i] += 1.0f;

		/* HiH = Hi * H -> HiH = H */
		if (!only_compute_R) {
			mat_mul(Hi, H, HiH, row_a, row_a, row_a);
			memcpy(H, HiH, row_a_row_a * sizeof(real));
		}

		/* HiR = Hi * R -> HiR = R */
		mat_mul(Hi, R, HiR, row_a, row_a, column_a);
		memcpy(R, HiR, row_a * column_a * sizeof(real));
	}

	/* Do inverse on H and give it to Q */
	status = 1;
	if (!only_compute_R) {
		status = mat_inv(H, row_a);
		memcpy(Q, H, row_a_row_a * sizeof(real));
	}
	return status;
}

/*
 * This solves Ax = b.
 * A need to be square and upper triangular.
 * A [m*n]
 * b [m]
 * m == n
 */
void linear_solve_upper_triangular(const real A[], real x[], real b[], uint16_t column)
{

	/* Time to solve x from Ax = b. */
	real     sum;
	uint16_t i, j;
	memset(x, 0, column * sizeof(real));
	for (i = column - 1; i >= 0; i--) { /* Column */
		sum = 0;                        /* This is our sum */
		for (j = i; j < column; j++) {  /* Row */
			sum += A[i * column + j] * x[j];
		}
		x[i] = (b[i] - sum) / A[i * column + i];

		/* For backwards unsigned for-loops, important because uint16 i = -1 is actually 65535 */
		if (i == 0)
			break;
	}
}

/*
 * Solve with forward substitution. This can be used with Cholesky decomposition
 * A [m*n] need to be lower triangular and square
 * b [m]
 * x [n]
 * n == m
 */
void linear_solve_lower_triangular(const real A[], real x[], real b[], uint16_t row)
{
	/* Time to solve x from Ax = b. */
	real     sum;
	uint16_t i, j;
	memset(x, 0, row * sizeof(real));
	for (i = 0; i < row; i++) {
		sum = 0;
		for (j = 0; j < i; j++)
			sum = sum + A[row * i + j] * x[j];

		x[i] = (b[i] - sum) / A[row * i + i];
	}
}

/*
 * Solve Ax=b with QR decomposition
 * A[m*n]
 * b[m]
 * x[n]
 */
void linear_solve_qr(real A[], real x[], real b[], uint16_t row, uint16_t column)
{
	/* QR-decomposition */
	real Q[10 * 10]; /* row * row */
	real R[10 * 10]; /* row * column */
	real QTb[10];    /* row */
	mat_qr(A, Q, R, row, column, 0);
	mat_transpose(Q, row, row);      /* Do transpose Q -> Q^T */
	mat_mul(Q, b, QTb, row, row, 1); /* Q^Tb = Q^T*b */
	linear_solve_upper_triangular(R, x, QTb, column);
}

