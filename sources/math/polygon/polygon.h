/**
 * @file polygon.h
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/12/29
 *
 * @example
 *
 * @development_log
 *
 */
#ifndef POLYGON_H
#define POLYGON_H

#include <stdio.h>
#include <stdint.h>
#include "math/math.h"

/* Increase EPSILON to encourage merging of near coincident edges */
#define OPUS_POLY_EPSILON (DBL_EPSILON)

enum {
	OPUS_POLY_DIFF,
	OPUS_POLY_INT,
	OPUS_POLY_XOR,
	OPUS_POLY_UNION
};

typedef struct opus_poly       opus_poly;
typedef struct opus_poly_verts opus_poly_verts;
typedef struct opus_poly_strip opus_poly_strip;

struct opus_poly_verts {
	int        num_vertices;
	opus_vec2 *vertex;
};

struct opus_poly {
	int  num_contours;
	int *hole; /* Hole / external contour flags */

	opus_poly_verts *contour; /* Contour array pointer */
};

struct opus_poly_strip {
	int              num_strips;
	opus_poly_verts *strip;
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

size_t *opus_tessellate(opus_vec2 **coords_ptr, opus_vec2 **holes);

void       opus_center(opus_vec2 *coords, size_t n, opus_vec2 *out_center);
void       opus_mean(opus_vec2 *coords, size_t n, opus_vec2 *out_mean);
opus_real  opus_area(opus_vec2 *coords, size_t n, int is_signed);
opus_real  opus_inertia(opus_vec2 *coords, size_t n, opus_real mass);
void       opus_translate(opus_vec2 *coords, size_t n, opus_vec2 t, opus_vec2 scalar);
void       opus_rotate(opus_vec2 *coords, size_t n, opus_vec2 point, opus_real angle);
void       opus_scale(opus_vec2 *coords, size_t n, opus_vec2 origin, opus_vec2 scalar);
int        opus_contains(opus_vec2 *coords, size_t n, opus_vec2 point);
void       opus_make_cw(opus_vec2 *coords, size_t n);
void       opus_make_ccw(opus_vec2 *coords, size_t n);
int        opus_is_convex(opus_vec2 *coords, size_t n);
opus_vec2 *opus_contour(opus_vec2 *coords, size_t n);
opus_vec2 *opus_chamfer(opus_vec2 *coords, size_t n, const opus_real *radius, size_t n_radius,
                        opus_real quality, opus_real quality_min, opus_real quality_max);

void       opus_set_polygon_offset(ptrdiff_t coord_offset, ptrdiff_t ele_offset);
void       opus_center_(void *vertices, size_t n, void *out_center);
opus_real  opus_area_(void *vertices, size_t n, int is_signed);
void       opus_mean_(void *vertices, size_t n, void *out_mean);
opus_real  opus_inertia_(void *vertices, size_t n, opus_real mass);
void       opus_translate_(void *vertices, size_t n, opus_vec2 vec, opus_vec2 scalar);
void       opus_rotate_(void *vertices, size_t n, opus_vec2 point, opus_real angle);
void       opus_scale_(void *vertices, size_t n, opus_vec2 origin, opus_vec2 scalar);
int        opus_contains_(void *vertices, size_t n, opus_vec2 point);
int        opus_is_convex_(void *vertices, size_t n);
opus_vec2 *opus_contour_(void *vertices, size_t n);
opus_vec2 *opus_chamfer_(void *vertices, size_t n, const opus_real *radius, size_t n_radius,
                         opus_real quality, opus_real quality_min, opus_real quality_max);
void      *opus_make_cw_(void *vertices, size_t n);
void      *opus_make_ccw_(void *vertices, size_t n);

void opus_poly_read(FILE *infile_ptr, int read_hole_flags, opus_poly *polygon);
void opus_poly_write(FILE *outfile_ptr, int write_hole_flags, opus_poly *polygon);
void opus_poly_add_contour(opus_poly *polygon, opus_poly_verts *contour, int hole);
void opus_poly_clip(int set_operation, opus_poly *subject, opus_poly *clip, opus_poly *result_polygon);
void opus_poly_strip_clip(int set_operation, opus_poly *subject, opus_poly *clip, opus_poly_strip *result_strip);
void opus_poly_to_strip(opus_poly *polygon, opus_poly_strip *strip);
void opus_poly_free(opus_poly *polygon);
void opus_poly_strip_free(opus_poly_strip *strip);


#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* POLYGON_H */
