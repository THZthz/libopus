/**
* @file circle.c
*     Author:    _              _
*               / \   _ __ ___ (_)  __ _ ___
*              / _ \ | '_ ` _ \| |/ _` / __|
*             / ___ \| | | | | | | (_| \__ \
*            /_/   \_\_| |_| |_|_|\__,_|___/  2023/2/28
*
* @example
*
* @development_log
*
*/
#include <stdlib.h>
#include "physics/opus/physics.h"
#include "physics/opus/physics_private.h"


opus_circle *opus_shape_circle_init(opus_circle *circle, opus_real radius)
{
	if (circle) {
		circle->_.type_ = PHYSICS_SHAPE_CIRCLE;
		circle->_.get_support = opus_shape_circle_get_support;
		circle->_.get_inertia = opus_shape_circle_get_inertia;
		circle->_.update_bound = opus_shape_circle_update_bound;
		circle->radius = radius;

		opus_shape_circle_update_bound((void *) circle, 0, opus_vec2_(0, 0));
	}
	return circle;
}

opus_circle *opus_shape_circle_create(opus_real radius)
{
	return opus_shape_circle_init(malloc(sizeof(opus_circle)), radius);
}

void opus_shape_circle_done(opus_circle *circle)
{

}

void opus_shape_circle_destroy(opus_circle *circle)
{
	free(circle);
}

opus_vec2 opus_shape_circle_get_support(opus_shape *shape, opus_mat2d transform, opus_vec2 dir,
                                      size_t *index)
{
	opus_circle *circle = (void *) shape;

	*index = 0;

	opus_vec2_set_length(&dir, circle->radius);

	return dir;
}

opus_real opus_shape_circle_get_inertia(opus_shape *shape, opus_real mass)
{
	opus_circle *circle = (void *) shape;
	return mass * circle->radius * circle->radius / 2;
}

void opus_shape_circle_update_bound(opus_shape *shape, opus_real rotation, opus_vec2 position)
{
	opus_circle *circle = (void *) shape;
	shape->bound.min.x = -circle->radius + position.x;
	shape->bound.min.y = -circle->radius + position.y;
	shape->bound.max.x = circle->radius + position.x;
	shape->bound.max.y = circle->radius + position.y;
}
