/**
 * @file sleeping.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/8/29
 *
 * @example
 *
 * @development_log
 *
 */

#include "physics/physics.h"

real SLEEPING_motion_wake_threshold  = 0.18;
real SLEEPING_motion_sleep_threshold = 0.08;
real SLEEPING_min_bias               = 0.9;

void sleeping_set(body_t *body, int is_sleeping)
{
	if (is_sleeping) {
		body->is_sleeping   = 1;
		body->sleep_counter = body->sleep_threshold;

		body->position_impulse.x = 0;
		body->position_impulse.y = 0;

		body->position_prev.x = body->position.x;
		body->position_prev.y = body->position.y;

		body->angle_prev    = body->angle;
		body->speed         = 0;
		body->angular_speed = 0;
		body->motion        = 0;
	} else {
		body->is_sleeping   = 0;
		body->sleep_counter = 0;
	}
}

void sleeping_update(body_t **bodies, count_t bodies_len, real time_scale)
{
	count_t i;
	real    time_factor = time_scale * time_scale * time_scale;

	/* update bodies sleeping status */
	for (i = 0; i < bodies_len; i++) {
		body_t *body   = bodies[i];
		real    motion = body->speed * body->speed + body->angular_speed * body->angular_speed;
		real    min_motion, max_motion;

		/* wake up bodies if they have a force applied */
		if (body->force.x != 0 || body->force.y != 0) {
			sleeping_set(body, 0);
			continue;
		}

		min_motion = r_min(body->motion, motion),
		max_motion = r_max(body->motion, motion);

		/* biased average motion estimation between frames */
		body->motion = SLEEPING_min_bias * min_motion + (1 - SLEEPING_min_bias) * max_motion;

		if (body->sleep_threshold > 0 && body->motion < SLEEPING_motion_sleep_threshold * time_factor) {
			body->sleep_counter += 1;

			if (body->sleep_counter >= body->sleep_threshold) {
				sleeping_set(body, 1);
			}
		} else if (body->sleep_counter > 0) {
			body->sleep_counter -= 1;
		}
	}
}

void sleeping_after_collisions(collision_pair_t **pairs, count_t pairs_len, real time_scale)
{
	count_t i;
	real    time_factor = time_scale * time_scale * time_scale;

	/* wake up bodies involved in collisions */
	for (i = 0; i < pairs_len; i++) {
		collision_pair_t *pair = pairs[i];
		collision_t      *collision;
		body_t           *body_a, *body_b;

		/* don't wake inactive pairs */
		if (!pair->is_active)
			continue;

		collision = pair->collision;
		body_a    = collision->body_a->parent,
		body_b    = collision->body_b->parent;

		/* don't wake if at least one body is static */
		if ((body_a->is_sleeping && body_b->is_sleeping) || body_a->is_static || body_b->is_static)
			continue;

		if (body_a->is_sleeping || body_b->is_sleeping) {
			body_t *sleeping_body = (body_a->is_sleeping && !body_a->is_static) ? body_a : body_b,
			       *moving_body   = sleeping_body == body_a ? body_b : body_a;

			if (!sleeping_body->is_static && moving_body->motion > SLEEPING_motion_wake_threshold * time_factor) {
				sleeping_set(sleeping_body, 0);
			}
		}
	}
}
