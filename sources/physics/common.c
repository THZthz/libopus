/**
 * @file common.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/8/25
 *
 * @example
 *
 * @development_log
 *
 */

#include "data_structure/array.h"
#include "physics.h"
#define SOKOL_TIME_IMPL
#include "external/sokol_time.h"

#include <stdarg.h>

uint64_t common_next_id()
{
	static uint64_t id = 1;
	return id++;
}

uint64_t common_next_group(int is_non_colliding)
{
	if (is_non_colliding)
		return BODY_next_non_colliding_group_id--;

	return BODY_next_colliding_group_id++;
}

uint64_t common_next_category()
{
	BODY_next_category = BODY_next_category << 1;
	return BODY_next_category;
}

body_t *common_rectangle(vec2 pos, real length, real width)
{
	vertices_t vertices;
	point_t    points[4] = {{0, 0}};
	body_t    *rect      = body_create();

	points[1].x = length;
	points[1].y = 0;
	points[2].x = length;
	points[2].y = width;
	points[3].x = 0;
	points[3].y = width;
	vertices    = vertices_create(points, 4, rect);
	body_set_vertices(rect, vertices);
	body_set_position(rect, pos);
	return rect;
}

body_t *common_polygon(vec2 pos, count_t n, ...)
{
	va_list  va_args;
	point_t *points = (point_t *) malloc(sizeof(point_t) * n);
	body_t  *poly   = body_create();
	count_t  i;

	va_start(va_args, n);
	for (i = 0; i < n; i++) {
		vec2 v = va_arg(va_args, vec2);
		vec2_dup(points[i], v);
	}
	va_end(va_args);

	body_set_vertices(poly, vertices_create(points, n, poly));
	body_set_position(poly, pos);
	free(points);

	return poly;
}
