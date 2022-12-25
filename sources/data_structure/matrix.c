/**
 * @file matrix.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/12/24
 *
 * @example
 *
 * @development_log
 *
 */

#include "data_structure/matrix.h"

#include <stdlib.h>
#include <string.h>

/**
 * @brief allocate space for matrix and update matrix's information, return NULL if failed
 */
matrix_t *matrix_alloc_space(matrix_t *matrix, unsigned rows, unsigned cols, unsigned cell_size)
{
	matrix->rows      = rows;
	matrix->cols      = cols;
	matrix->cell_size = cell_size;
	matrix->data      = malloc(rows * cols * cell_size);

	if (!matrix->data) return NULL;

	return matrix;
}

/**
 * @brief namely call "memset" to initialize matrix
 */
matrix_t *matrix_init_zero(matrix_t *matrix)
{
	if (matrix->data) memset(matrix->data, 0, matrix->rows * matrix->cols * matrix->cell_size);
	return matrix;
}

/**
 * @brief get the pointer of the data on coordinate (x, y)
 */
void *matrix_cell(matrix_t *matrix, unsigned x, unsigned y)
{
	return (char *) matrix->data + (y * matrix->cols + x) * matrix->cell_size;
}

/**
 * @brief copy the data of (src_x, src_y) to (dst_x, dst_y)
 */
void matrix_copy_cell(matrix_t *matrix, unsigned src_x, unsigned src_y, unsigned dst_x, unsigned dst_y)
{
	memcpy(matrix_cell(matrix, dst_x, dst_y), matrix_cell(matrix, src_x, src_y), matrix->cell_size);
}

/**
 * @brief copy the data of (src_x, src_y) on "source" matrix to (dst_x, dst_y) on "destination" matrix
 */
void matrix_copy_cell_from(matrix_t *src, unsigned src_x, unsigned src_y, matrix_t *dst, unsigned dst_x, unsigned dst_y)
{
	/* assume the cell_size of the two matrix is identical */
	memcpy(matrix_cell(dst, dst_x, dst_y), matrix_cell(src, src_x, src_y), dst->cell_size);
}

matrix_t *matrix_pad(matrix_t *matrix, unsigned pad_x, unsigned pad_y, unsigned new_cols, unsigned new_rows)
{
	unsigned x, y;
	matrix_t new_matrix;

	matrix_alloc_space(&new_matrix, new_rows, new_cols, matrix->cell_size);
	if (!new_matrix.data) return NULL;

	for (y = pad_y; y < new_rows; y++)
		for (x = pad_x; x < new_cols; x++)
			matrix_copy_cell_from(matrix, x - pad_x, y - pad_y, &new_matrix, x, y);

	free(matrix->data);
	memcpy(matrix, &new_matrix, sizeof(matrix_t));

	return matrix;
}
