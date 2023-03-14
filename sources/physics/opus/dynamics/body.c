/**
 * @file body.c
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
#include "data_structure/array.h"
#include "math/polygon/polygon.h"
#include "physics/opus/physics.h"
#include "physics/opus/physics_private.h"
#include "utils/utils.h"

opus_body *opus_body_init(opus_body *body)
{
	if (body) {
		body->type        = OPUS_BODY_DYNAMIC;
		body->id          = opus_get_physics_id();
		body->bitmask     = 0x0001;
		body->density     = 0.002;
		body->inv_mass    = OPUS_REAL_MAX;
		body->inertia     = OPUS_REAL_MAX;
		body->friction    = 0.01;
		body->restitution = 0.01;
		opus_arr_create(body->parts, sizeof(opus_body *));
		opus_arr_push(body->parts, &body);
	}
	return body;
}

opus_body *opus_body_create(void) { return opus_body_init(OPUS_CALLOC(1, sizeof(opus_body))); }

void opus_body_done(opus_body *body)
{
	opus_recycle_physics_id(body->id);
	opus_arr_destroy(body->parts);
	opus_shape_destroy(body->shape);
}

void opus_body_destroy(opus_body *body)
{
	opus_body_done(body);
	OPUS_FREE(body);
}

void opus_body_apply_impulse(opus_body *body, opus_vec2 impulse, opus_vec2 r)
{
	OPUS_RETURN_IF(, body->type == OPUS_BODY_STATIC);
	body->velocity.x += impulse.x * body->inv_mass;
	body->velocity.y += impulse.y * body->inv_mass;
	body->angular_velocity += body->inv_inertia * opus_vec2_cross(r, impulse);
}

void opus_body_apply_force(opus_body *body, opus_vec2 force, opus_vec2 r)
{
	body->force.x += force.x;
	body->force.y += force.y;
	body->torque += opus_vec2_cross(r, force);
}

void opus_body_set_position(opus_body *body, opus_vec2 position)
{
	body->position    = position;
	body->is_sleeping = 0;
}

void opus_body_clear_force(opus_body *body)
{
	body->force.x = 0;
	body->force.y = 0;
	body->torque  = 0;
}

void opus_body_integrate_forces(opus_body *body, opus_real dt)
{
	opus_real friction_air;

	OPUS_RETURN_IF(, body->type == OPUS_BODY_STATIC || body->type == OPUS_BODY_KINEMATIC || body->is_sleeping);

	friction_air = body->friction;

	/* FIXME */
	body->velocity.x *= 0.9;
	body->velocity.y *= 0.9;
	body->angular_velocity *= 0.9;

	body->velocity.x += body->force.x * body->inv_mass * dt;
	body->velocity.y += body->force.y * body->inv_mass * dt;
	body->angular_velocity += body->torque * body->inv_inertia * dt;
}

void opus_body_integrate_velocity(opus_body *body, opus_real dt)
{
	opus_real dx, dy, dr;
	opus_real speed, angular_speed;

	OPUS_RETURN_IF(, body->type == OPUS_BODY_STATIC || body->is_sleeping);

	body->velocity.x *= 0.9;
	body->velocity.y *= 0.9;

	dx = body->velocity.x * dt;
	dy = body->velocity.y * dt;
	dr = body->angular_velocity * dt;

	body->position.x += dx;
	body->position.y += dy;
	body->rotation = opus_mod(body->rotation + dr, 2 * OPUS_PI);

	speed         = opus_hypot(dx, dy);
	angular_speed = opus_abs(dr);

	body->motion = speed * speed + angular_speed * angular_speed;
}

void opus_body_set_mass(opus_body *body, opus_real mass)
{
	body->inv_mass = opus_equal(mass, 0) ? OPUS_REAL_MAX : 1 / mass;
	body->mass     = mass;
}

void opus_body_set_inertia(opus_body *body, opus_real inertia)
{
	body->inv_inertia = opus_equal(inertia, 0) ? OPUS_REAL_MAX : 1 / inertia;
	body->inertia     = inertia;
}

/**
 * @brief transform a world point into local point, mind the numerical inaccuracy
 * @param body
 * @param point
 * @return
 */
opus_vec2 opus_body_w2l(opus_body *body, opus_vec2 point)
{
	opus_mat2d t, inv;
	opus_mat2d_rotate_about(t, (float) body->rotation, body->position);
	opus_mat2d_inv(inv, t);
	return opus_mat2d_pre_mul_vec(inv, point);
}

/**
 * @brief transform a local point into world point
 * @param body
 * @param point
 * @return
 */
opus_vec2 opus_body_l2w(opus_body *body, opus_vec2 point)
{
	opus_mat2d t;
	opus_mat2d_rotate_about(t, (float) body->rotation, body->position);
	return opus_mat2d_pre_mul_vec(t, point);
}

void opus_body_set_density(opus_body *body, opus_real density)
{
	body->density = density;
	opus_body_set_mass(body, body->area * body->density);
	opus_body_set_inertia(body, body->shape->get_inertia(body->shape, body->mass));
}

opus_real opus_body_get_area(opus_body *body)
{
	if (!body->shape) return 0;
	return body->shape->get_area(body->shape);
}

void opus_body_set_shape(opus_body *body, opus_shape *shape)
{
	body->shape = shape;
	body->area  = opus_body_get_area(body);
	opus_body_set_mass(body, body->area * body->density);
	opus_body_set_inertia(body, shape->get_inertia(shape, body->mass));
	shape->update_bound(shape, body->rotation, body->position);
}

void opus_body_step_position(opus_body *body, opus_real dt)
{
	switch (body->type) {
		case OPUS_BODY_STATIC:
			break;
		case OPUS_BODY_DYNAMIC:
		case OPUS_BODY_KINEMATIC:
		case OPUS_BODY_BULLET:
			body->position = opus_vec2_add(body->position, opus_vec2_scale(body->velocity, dt));
			body->rotation += body->angular_velocity * dt;

			body->force.x = 0;
			body->force.y = 0;
			body->torque  = 0;
			break;
		default:
			OPUS_ERROR("opus_body_step_position::no such body type or unsupported -- id:%d\n",
			           body->id);
			break;
	}
}

void opus_body_step_velocity(opus_body *body, opus_vec2 gravity, opus_real dt)
{
	opus_real lvd = 0.9f;
	opus_real avd = 0.9f;
	opus_vec2 at;

	switch (body->type) {
		case OPUS_BODY_STATIC:
			opus_vec2_set(&body->velocity, 0, 0);
			body->angular_velocity = 0;
			break;
		case OPUS_BODY_DYNAMIC:
			body->force = opus_vec2_add(body->force, opus_vec2_scale(gravity, body->mass));

			at = opus_vec2_scale(body->force, body->inv_mass * dt);

			body->velocity = opus_vec2_add(body->velocity, at);
			body->angular_velocity += body->inv_inertia * body->torque * dt;

			/* damping, reduce numerical instability generated */
			body->velocity = opus_vec2_scale(body->velocity, lvd);
			body->angular_velocity *= avd;

			break;
		case OPUS_BODY_KINEMATIC:
		default:
			OPUS_ERROR("opus_body_step_velocity::no such body type or unsupported -- id:%d\n",
			           body->id);
			break;
	}
}
