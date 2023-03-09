/**
 * @file geo.h
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/8/7
 *
 * @brief In fact, there already exists a perfect version of such geometry calculating,
 * 		that is https://www.cs.cmu.edu/~quake/robust.html (which can be found at
 * "sources/external/predicates.c").
 *
 * @development_log
 *
 */
#ifndef GEOMETRY_H
#define GEOMETRY_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stddef.h>

#include "math/math.h"

enum { OPUS_LEFT_VORONOI   = -1,
	   OPUS_MIDDLE_VORONOI = 0,
	   OPUS_RIGHT_VORONOI  = 1 };

typedef struct opus_aabb opus_aabb;

struct opus_aabb {
	opus_vec2 min, max;
};

void       opus_aabb_init(opus_aabb *aabb, opus_real min_x, opus_real min_y, opus_real max_x, opus_real max_y);
void       opus_aabb_init_with_range(opus_aabb *aabb, opus_real x, opus_real y, opus_real w, opus_real h);
opus_aabb *opus_aabb_create(opus_real min_x, opus_real min_y, opus_real max_x, opus_real max_y);
void       opus_aabb_destroy(opus_aabb *aabb);
void       opus_aabb_copy(opus_aabb *dest, opus_aabb *src);
opus_real  opus_aabb_width(opus_aabb *aabb);
opus_real  opus_aabb_height(opus_aabb *aabb);
opus_real  opus_aabb_perimeter(opus_aabb *aabb);
int        opus_aabb_ray_intersect(opus_vec2 origin, opus_vec2 direction, opus_aabb *aabb, opus_real *t);
int        opus_aabb_is_overlap(opus_aabb *a, opus_aabb *b);
int        opus_aabb_contain(opus_aabb *aabb, opus_real x, opus_real y);
void       opus_aabb_translate(opus_aabb *aabb, opus_real dx, opus_real dy);
void       opus_aabb_shift(opus_aabb *aabb, opus_real x, opus_real y);
void       opus_aabb_combine(opus_aabb *a, opus_aabb *b, opus_aabb *result);

int       opus_fuzzy_is_collinear(opus_vec2 a, opus_vec2 b, opus_vec2 c);
int       opus_is_collinear(opus_vec2 a, opus_vec2 b, opus_vec2 c);
opus_vec2 opus_point_to_segment(opus_vec2 a, opus_vec2 b, opus_vec2 p);
int       opus_is_point_on_segment(opus_vec2 a, opus_vec2 b, opus_vec2 p);
int       opus_is_point_on_same_side(opus_vec2 s, opus_vec2 e, opus_vec2 ref, opus_vec2 point);
opus_vec2 opus_nearest_point_on_line(opus_vec2 a, opus_vec2 b, opus_vec2 p);
int       opus_voronoi_region(opus_vec2 line, opus_vec2 point);
int       opus_voronoi_region_(opus_vec2 line_s, opus_vec2 line_e, opus_vec2 point);

int geo_ll_intersect(opus_real line_ax1, opus_real line_ay1, opus_real line_ax2, opus_real line_ay2,
                     opus_real line_bx1, opus_real line_by1, opus_real line_bx2, opus_real line_by2,
                     opus_real *x, opus_real *y, opus_real *t1, opus_real *t2);
int opus_ray_rect_intersect(opus_vec2 ray_ori, opus_vec2 ray_dir, opus_vec2 rect_pos,
                            opus_real width, opus_real height, opus_real *t);

int       opus_is_triangle_contains_origin(opus_vec2 a, opus_vec2 b, opus_vec2 c);
opus_real opus_triangle_area(opus_vec2 a, opus_vec2 b, opus_vec2 c);
void      opus_triangle_cricumcenter(opus_real ax, opus_real ay, opus_real bx, opus_real by, opus_real cx,
                                     opus_real cy, opus_real *xc, opus_real *yc, opus_real *r_sq);
int       opus_is_point_in_triangle(opus_real x, opus_real y, opus_real x1, opus_real y1, opus_real x2,
                                    opus_real y2, opus_real x3, opus_real y3);
opus_vec2 opus_triangle_inner_center(opus_vec2 a, opus_vec2 b, opus_vec2 c);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* GEOMETRY_H */
