/**
 * @file geo.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/8/7
 *
 * @example
 *
 * @development_log
 *		for running speed:
 *			1` use less condition judgements
 *			2` use pointer
 */


#include <float.h>
#include <stdio.h>
#include <stdlib.h>

#include "math/geometry.h"
#include "math/polygon/polygon.h"
#include "data_structure/array.h"


void opus_aabb_init_with_range(opus_aabb *aabb, opus_real x, opus_real y, opus_real w, opus_real h)
{
	aabb->min.x = x;
	aabb->min.y = y;
	aabb->max.x = x + w;
	aabb->max.y = y + h;
}

opus_real opus_aabb_width(opus_aabb *aabb) { return aabb->max.x - aabb->min.x; }

opus_real opus_aabb_height(opus_aabb *aabb) { return aabb->max.y - aabb->min.y; }

void opus_aabb_copy(opus_aabb *dest, opus_aabb *src)
{
	dest->min.x = src->min.x;
	dest->min.y = src->min.y;
	dest->max.x = src->max.x;
	dest->max.y = src->max.y;
}

void opus_aabb_combine(opus_aabb *a, opus_aabb *b, opus_aabb *result)
{
	result->min.x = a->min.x < b->min.x ? a->min.x : b->min.x;
	result->max.x = a->max.x > b->max.x ? a->max.x : b->max.x;
	result->min.y = a->min.y < b->min.y ? a->min.y : b->min.y;
	result->max.y = a->max.y > b->max.y ? a->max.y : b->max.y;
}

opus_real opus_aabb_perimeter(opus_aabb *aabb)
{
	opus_real wx = aabb->max.x - aabb->min.x;
	opus_real wy = aabb->max.y - aabb->min.y;
	return 2.0 * (wx + wy);
}

int opus_aabb_is_overlap(opus_aabb *a, opus_aabb *b)
{
	return (a->min.x <= b->max.x && a->min.y <= b->max.y && a->max.x >= b->min.x &&
	        a->max.y >= b->min.y);
}

int opus_aabb_contain(opus_aabb *aabb, opus_real x, opus_real y)
{
	return (x <= aabb->max.x && x >= aabb->min.x && y <= aabb->max.y && y >= aabb->min.y);
}

int opus_aabb_ray_intersect(opus_vec2 origin, opus_vec2 direction, opus_aabb *aabb, opus_real *t)
{
	return opus_ray_rect_intersect(origin, direction, opus_vec2_(aabb->min.x, aabb->min.y),
	                               aabb->max.x - aabb->min.x, aabb->max.y - aabb->min.y, t);
}

void opus_aabb_init(opus_aabb *aabb, opus_real min_x, opus_real min_y, opus_real max_x, opus_real max_y)
{
	aabb->min.x = min_x;
	aabb->min.y = min_y;
	aabb->max.x = max_x;
	aabb->max.y = max_y;
}

opus_aabb *opus_aabb_create(opus_real min_x, opus_real min_y, opus_real max_x, opus_real max_y)
{
	opus_aabb *aabb = (opus_aabb *) OPUS_MALLOC(sizeof(opus_aabb));
	opus_aabb_init(aabb, min_x, min_y, max_x, max_y);

	return aabb;
}

void opus_aabb_destroy(opus_aabb *aabb) { OPUS_FREE(aabb); }

/**
 * translate the AABB box
 */
void opus_aabb_translate(opus_aabb *aabb, opus_real dx, opus_real dy)
{
	aabb->min.x += dx;
	aabb->max.x += dx;
	aabb->min.y += dy;
	aabb->max.y += dy;
}

/**
 * shift the aabb bounding box to a designated position(bottom left).
 */
void opus_aabb_shift(opus_aabb *aabb, opus_real x, opus_real y)
{
	opus_real width  = aabb->max.x - aabb->min.x;
	opus_real height = aabb->max.y - aabb->min.y;

	aabb->min.x = x;
	aabb->min.y = y;
	aabb->max.x = x + width;
	aabb->max.y = y + height;
}

/*              |     (middle)   |
 *     (left)  [S]--------------[E]  (right)
 *             |     (middle)   |
 */
/**
 * @brief determine which voronoi region a point is on a line segment.
 * 	Assume that both the line and the point are relative to `(0,0)`
 * @return r_geo_left_voronoi_region if in left voronoi region
 * 		   r_geo_middle_voronoi_region if in middle voronoi region
 * 		   r_geo_right_voronoi_region if in right voronoi region
 */
int opus_voronoi_region(opus_vec2 line, opus_vec2 point)
{
	opus_real len2 = opus_vec2_len2(line);
	opus_real dp   = opus_vec2_dot(point, line);
	if (dp < 0) {
		return OPUS_LEFT_VORONOI;
	} else if (dp > len2) {
		return OPUS_RIGHT_VORONOI;
	} else {
		return OPUS_MIDDLE_VORONOI;
	}
}

int opus_voronoi_region_(opus_vec2 line_s, opus_vec2 line_e, opus_vec2 point)
{
	opus_vec2 line, rel_point;
	line      = opus_vec2_sub(line_e, line_s);
	rel_point = opus_vec2_sub(point, line_s);

	return opus_voronoi_region(line, rel_point);
}

opus_real opus_triangle_area(opus_vec2 a, opus_vec2 b, opus_vec2 c)
{
	return opus_abs(opus_vec2_cross(opus_vec2_sub(a, b), opus_vec2_sub(a, c))) / 2;
}

opus_vec2 opus_nearest_point_on_line(opus_vec2 a, opus_vec2 b, opus_vec2 p)
{
	opus_vec2 res;
	opus_vec2 ap, ab_n, ap_proj, op_proj;
	if (opus_vec2_equal(a, b))
		return a;

	if (opus_is_collinear(a, b, p))
		return p;

	ap      = opus_vec2_to(a, p);
	ab_n    = opus_vec2_norm(opus_vec2_to(a, b));
	ap_proj = opus_vec2_scale(ab_n, opus_vec2_dot(ab_n, ap));
	op_proj = opus_vec2_add(a, ap_proj);

	if (opus_fuzzy_is_collinear(a, b, op_proj))
		return op_proj;

	return opus_vec2_len2(ap) > opus_vec2_len2(opus_vec2_to(b, p)) ? b : a;
}

OPUS_INLINE int geo_ll_intersect(opus_real line_ax1, opus_real line_ay1, opus_real line_ax2,
                            opus_real line_ay2, opus_real line_bx1, opus_real line_by1,
                            opus_real line_bx2, opus_real line_by2, opus_real *x, opus_real *y,
                            opus_real *t1, opus_real *t2)
{
	opus_real line_adx, line_ady, line_bdx, line_bdy;

	/* Line_a: (line_ax1, line_ay1) + T1 * (line_adx, line_ady) */
	line_adx = line_ax2 - line_ax1;
	line_ady = line_ay2 - line_ay1;

	/* Line_b: (line_bx1, line_by1) + T2 * (line_bdx, line_bdy) */
	line_bdx = line_bx2 - line_bx1;
	line_bdy = line_by2 - line_by1;

	/* if parallel, no intersect */
	if ((line_adx == 0 && line_bdx == 0) || (line_ady * line_bdx == line_bdy * line_adx)) return 0;

	/* solve for parametric T1 and T2 */
	/* r_px+r_dx*T1 = s_px+s_dx*T2 && r_py+r_dy*T1 =s_py+s_dy*T2 */
	*t2 = (line_adx * (line_by1 - line_ay1) + line_ady * (line_ax1 - line_bx1)) /
	      (line_bdx * line_ady - line_bdy * line_adx);

	/* line_adx or line_ady might be zero */
	*t1 = line_adx != 0 ? (line_bx1 + line_bdx * (*t2) - line_ax1) / line_adx
	                    : (line_by1 + line_bdy * (*t2) - line_ay1) / line_ady;
	*x  = line_ax1 + line_adx * (*t1);
	*y  = line_ay1 + line_ady * (*t1);

	return 1;
}

OPUS_INLINE int opus_ray_rect_intersect(opus_vec2 ray_ori, opus_vec2 ray_dir, opus_vec2 rect_pos,
                                  opus_real width, opus_real height, opus_real *t)
{
	opus_real dir_frac_x, dir_frac_y, t1, t2, t3, t4, temp, t_min, t_max;

	/*  TODO: this could be reused when testing a ray is intersect with multiple AABB boxes */
	dir_frac_x = 1.0f / ray_dir.x;
	dir_frac_y = 1.0f / ray_dir.y;

	t1 = (rect_pos.x - ray_ori.x) * dir_frac_x;
	t2 = (rect_pos.x + width - ray_ori.x) * dir_frac_x;
	t3 = (rect_pos.y - ray_ori.y) * dir_frac_y;
	t4 = (rect_pos.y + height - ray_ori.y) * dir_frac_y;

	if (t1 < t2) {
		temp = t1;
		t1   = t2;
		t2   = temp;
	}
	if (t3 < t4) {
		temp = t3;
		t3   = t4;
		t4   = temp;
	}

	t_min = t2 > t4 ? t2 : t4;
	t_max = t1 < t3 ? t1 : t3;

	/* if t_max < 0, ray (line) is intersecting AABB, but the whole AABB is behind us */
	if (t_max < 0) {
		*t = t_max;
		return 0;
	}

	/* if t_min > t_max, ray doesn't intersect AABB */
	if (t_min > t_max) {
		*t = t_max;
		return 0;
	}

	*t = t_min;
	return 1;
}

/*
 * One simple way to detect if point D lies in the circum-circle of points A, B, C (triangle)
 *  is to evaluate the determinant:
 *    |  Ax  Ay  (Ax * Ax + Ay * Ay)  1  |
 *    |  Bx  By  (Bx * Bx + By * By)  1  |
 *    |  Cx  Cy  (Cx * Cx + Cy * Cy)  1  |
 *    |  Dx  Dy  (Dx * Dx + Dy * Dy)  1  |
 *
 * Which equals to:
 *    |  (Ax - Dx)  (Ay - Dy)  (Ax * Ax - Dx * Dx)  |
 *    |  (Bx - Dx)  (By - Dy)  (Bx * Bx - Dx * Dx)  |
 *    |  (Cx - Dx)  (Cy - Dy)  (Cx * Cx - Dx * Dx)  |
 *
 * When A, B and C are sorted in a counter-clockwise order, this determinant is positive
 *  only if D lies in the circum-circle.
 *
 */
OPUS_INLINE int opus_is_point_in_triangle(opus_real x, opus_real y, opus_real x1, opus_real y1,
                                    opus_real x2, opus_real y2, opus_real x3, opus_real y3)
{
	/* another version, ARGS: vec2 p, vec2 t0, vec2 t1, vec2 t2 */
	/*real a, b, c;
	vec2 ab, bc, ca, ap, bp, cp;
	vec2_sub(ab, t1, t0);
	vec2_sub(bc, t2, t1);
	vec2_sub(ca, t0, t2);
	vec2_sub(ap, p, t0);
	vec2_sub(bp, p, t1);
	vec2_sub(cp, p, t2);
	a = vec2_cross(ab, ap);
	b = vec2_cross(bc, bp);
	c = vec2_cross(ca, cp);
	if (a < 0 && b < 0 && c < 0) {
	    return 1;
	}
	return a > 0 && b > 0 && c > 0;*/
	opus_real e1_x = x1 - x;
	opus_real e1_y = y1 - y;
	opus_real e2_x = x2 - x;
	opus_real e2_y = y2 - y;
	opus_real e3_x = x3 - x;
	opus_real e3_y = y3 - y;
	opus_real t1   = e1_x * e2_y - e2_x * e1_y;
	opus_real t2   = e2_x * e3_y - e3_x * e2_y;
	opus_real t3   = e3_x * e1_y - e1_x * e3_y;

	/* when lies in the bound of the triangle, we also think it is "in" triangle */
	return t1 * t2 >= 0 && t1 * t3 >= 0;
}

OPUS_INLINE void opus_triangle_cricumcenter(opus_real ax, opus_real ay, opus_real bx, opus_real by,
                                      opus_real cx, opus_real cy, opus_real *xc, opus_real *yc,
                                      opus_real *r_sq)
{
	opus_real dx, dy, ex, ey;
	opus_real bl, cl, d, r;

	dx = bx - ax, dy = by - ay;
	ex = cx - ax, ey = cy - ay;
	cl = ex * ex + ey * ey;
	bl = dx * dx + dy * dy;
	d  = (opus_real) 0.5 / (dx * ey - dy * ex);

	*xc   = (ey * bl - dy * cl) * d;
	*yc   = (dx * cl - ex * bl) * d;
	*r_sq = *xc * *xc + *yc * *yc;
	*xc += ax, *yc += ay;
}

int opus_is_triangle_contains_origin(opus_vec2 a, opus_vec2 b, opus_vec2 c)
{
	opus_real ra = opus_vec2_cross(opus_vec2_sub(b, a), opus_vec2_neg(a));
	opus_real rb = opus_vec2_cross(opus_vec2_sub(c, b), opus_vec2_neg(b));
	opus_real rc = opus_vec2_cross(opus_vec2_sub(a, c), opus_vec2_neg(c));
	return (ra >= 0 && rb >= 0 && rc >= 0) || (ra <= 0 && rb <= 0 && rc <= 0);
}

opus_vec2 opus_triangle_inner_center(opus_vec2 a, opus_vec2 b, opus_vec2 c)
{
	opus_vec2 res;
	opus_real ab = opus_vec2_len(opus_vec2_to(a, b));
	opus_real bc = opus_vec2_len(opus_vec2_to(b, c));
	opus_real ca = opus_vec2_len(opus_vec2_to(c, a));
	res     = opus_vec2_add(opus_vec2_scale(c, ab), opus_vec2_scale(a, bc));
	res     = opus_vec2_add(res, opus_vec2_scale(b, ca));
	return opus_vec2_scale(res, 1. / (ab + bc + ca));
}

int opus_is_point_on_same_side(opus_vec2 s, opus_vec2 e, opus_vec2 ref, opus_vec2 point)
{
	opus_vec2 u  = opus_vec2_sub(e, s);
	opus_vec2 v  = opus_vec2_sub(ref, s);
	opus_vec2 w  = opus_vec2_sub(point, s);
	opus_real d1 = opus_vec2_cross(u, v);
	opus_real d2 = opus_vec2_cross(u, w);
	/* same side or on the edge */
	return opus_sign(d1) == opus_sign(d2);
}

int opus_is_collinear(opus_vec2 a, opus_vec2 b, opus_vec2 c)
{
	opus_real v = opus_vec2_cross(opus_vec2_sub(a, b), opus_vec2_sub(a, c));
	return opus_equal(opus_abs(v), 0);
}

int opus_fuzzy_is_collinear(opus_vec2 a, opus_vec2 b, opus_vec2 c)
{
	return c.x <= opus_max(a.x, b.x) && c.x >= opus_min(a.x, b.x) && c.y <= opus_max(a.y, b.y) &&
	       c.y >= opus_min(a.y, b.y);
}

int opus_is_point_on_segment(opus_vec2 a, opus_vec2 b, opus_vec2 p)
{
	return !opus_is_collinear(a, b, p) ? 0 : opus_fuzzy_is_collinear(a, b, p);
}

opus_vec2 opus_point_to_segment(opus_vec2 a, opus_vec2 b, opus_vec2 p)
{
	opus_vec2 ap, ab_normal, ap_proj, op_proj;

	if (opus_vec2_equal(a, b)) return opus_vec2_(0, 0);
	if (opus_is_collinear(a, b, p)) return p;

	ap        = opus_vec2_sub(p, a);
	ab_normal = opus_vec2_norm(opus_vec2_sub(b, a));
	ap_proj   = opus_vec2_scale(ab_normal, opus_vec2_dot(ab_normal, ap));
	op_proj   = opus_vec2_add(a, ap_proj);

	if (opus_fuzzy_is_collinear(a, b, op_proj)) return op_proj;

	return opus_vec2_len2(opus_vec2_sub(p, a)) > opus_vec2_len2(opus_vec2_sub(p, b)) ? b : a;
}
