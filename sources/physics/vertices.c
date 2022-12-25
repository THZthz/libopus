/**
 * @file vertices.c
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
#include "physics/physics.h"

vertices_t vertices_alloc()
{
	return NULL; /* do not use this to allocate memory for vertices_t */
}

vertices_t vertices_init(vertices_t vertices, points_t points, count_t n, body_t *body)
{
	count_t i;
	for (i = 0; i < n; i++) {
		vertex_t *vertex    = &vertices[i];
		vertex->x           = points[i].x;
		vertex->y           = points[i].y;
		vertex->is_internal = 0;
		vertex->body        = body;
		vertex->index       = i;
	}
	return vertices;
}

void vertices_done(vertices_t vertices)
{
	DO_NOTHING();
}

vertices_t vertices_create(points_t points, count_t n, body_t *body)
{
	vertices_t vertices = vertices_alloc();
	array_create(vertices, sizeof(vertex_t));
	array_resize(vertices, n);
	vertices = vertices_init(vertices, points, n, body);
	return vertices;
}

void vertices_destroy(vertices_t vertices)
{
	array_destroy(vertices);
}

void vertices_update_aabb(vertices_t vertices, aabb_t *aabb, vec2 velocity, int update_velocity)
{
	count_t i;
	bounds_t *bounds = cast_bounds(aabb);
	bounds->min.x = REAL_MAX;
	bounds->max.x = -REAL_MAX;
	bounds->min.y = REAL_MAX;
	bounds->max.y = -REAL_MAX;
	
	for ( i = 0; i < array_len(vertices); i++) {
		vertex_t *vertex = &vertices[i];
		if (vertex->x > bounds->max.x) bounds->max.x = vertex->x;
		if (vertex->x < bounds->min.x) bounds->min.x = vertex->x;
		if (vertex->y > bounds->max.y) bounds->max.y = vertex->y;
		if (vertex->y < bounds->min.y) bounds->min.y = vertex->y;
	}

	if (update_velocity) {
		if (velocity.x > 0) {
			bounds->max.x += velocity.x;
		} else {
			bounds->min.x += velocity.x;
		}

		if (velocity.y > 0) {
			bounds->max.y += velocity.y;
		} else {
			bounds->min.y += velocity.y;
		}
	}
}



