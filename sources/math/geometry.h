/**
 * @file geo.h
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/8/7
 *
 * @brief In fact, there already exists a perfect version of such geometry calculating,
 * 		that is https://www.cs.cmu.edu/~quake/robust.html (which can be found at "sources/external/predicates.c").
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

enum {
	left_voronoi_region,
	middle_voronoi_region,
	right_voronoi_region
};

typedef struct aabb aabb_t;
struct aabb {
	real min_x, min_y, max_x, max_y;
};

void    aabb_init(aabb_t *aabb, real min_x, real min_y, real max_x, real max_y);
void    aabb_init_with_range(aabb_t *aabb, real x, real y, real w, real h);
aabb_t *aabb_create(real min_x, real min_y, real max_x, real max_y);
void    aabb_destroy(aabb_t *aabb);
void    aabb_copy(aabb_t *dest, aabb_t *src);

real aabb_get_width(aabb_t *aabb);
real aabb_get_height(aabb_t *aabb);

int aabb_is_ray_intersect(vec2 origin, vec2 direction, aabb_t *aabb, real *t);
int aabb_is_overlap_with(aabb_t *a, aabb_t *b);
int aabb_is_contain_point(aabb_t *aabb, real x, real y);

void aabb_translate(aabb_t *aabb, real dx, real dy);
void aabb_shift(aabb_t *aabb, real x, real y);
real aabb_get_perimeter(aabb_t *aabb);
void aabb_combine(aabb_t *a, aabb_t *b, aabb_t *result);

void      vertices_center(void *vertices, size_t n, void *out_center, ptrdiff_t offset);
real      vertices_area(void *vertices, size_t n, int is_signed, ptrdiff_t offset);
void      vertices_mean(void *vertices, size_t n, void *out_mean, ptrdiff_t offset);
real      vertices_inertia(void *vertices, size_t n, real mass, ptrdiff_t offset);
void     *vertices_translate(void *vertices, size_t n, vec2 vec, real scalar, ptrdiff_t offset);
void     *vertices_rotate(void *vertices, size_t n, vec2 point, real angle, ptrdiff_t offset);
void     *vertices_scale(void *vertices, size_t n, vec2 origin, real scalar_x, real scalar_y, ptrdiff_t offset);
int vertices_contains(void *vertices, size_t n, vec2 point, ptrdiff_t offset);
int vertices_is_convex(void *vertices, size_t n, ptrdiff_t offset);
vec2     *vertices_hull(void *vertices, size_t n, ptrdiff_t offset);
vec2     *vertices_chamfer(void *vertices, size_t n, real *radius, size_t n_radius, real quality, real quality_min, real quality_max, ptrdiff_t offset);
void     *vertices_sort_clockwise(void *vertices, size_t n, ptrdiff_t offset);

void      get_nearest_point_on_line(vec2 line_a, vec2 line_b, vec2 point, vec2 *res);
int       get_point_voronoi_region(vec2 line, vec2 point);
int       get_point_voronoi_region2(vec2 line_s, vec2 line_e, vec2 point);
int get_line_line_intersection(real line_ax1, real line_ay1, real line_ax2, real line_ay2,
                                     real line_bx1, real line_by1, real line_bx2, real line_by2,
                                     real *x, real *y, real *t1, real *t2);
void      get_circum_center_of_triangle(real ax, real ay, real bx, real by, real cx, real cy, real *xc, real *yc, real *r_sq);
int is_ray_intersect_rect(vec2 ray_ori, vec2 ray_dir, vec2 rect_pos, real width, real height, real *t);
int is_point_in_triangle(real x, real y, real x1, real y1, real x2, real y2, real x3, real y3);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* GEOMETRY_H */
