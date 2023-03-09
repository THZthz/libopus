/**
 * @file matrix.h
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
#ifndef MATRIX_H
#define MATRIX_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define matrix_cell_type(cell_type, matrix, x, y) ((cell_type *) matrix_cell((matrix), (x), (y)))

typedef struct matrix matrix_t;

struct matrix {
	void    *data;
	unsigned rows, cols, cell_size;
};

matrix_t *matrix_alloc_space(matrix_t *matrix, unsigned rows, unsigned cols, unsigned cell_size);
matrix_t *matrix_init_zero(matrix_t *matrix);
void     *matrix_cell(matrix_t *matrix, unsigned x, unsigned y);
void      matrix_copy_cell(matrix_t *matrix, unsigned src_x, unsigned src_y, unsigned dst_x, unsigned dst_y);
void      matrix_copy_cell_from(matrix_t *src, unsigned src_x, unsigned src_y, matrix_t *dst, unsigned dst_x, unsigned dst_y);
matrix_t *matrix_pad(matrix_t *matrix, unsigned pad_x, unsigned pad_y, unsigned new_cols, unsigned new_rows);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* MATRIX_H */
