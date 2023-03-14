/**
 * @file sleeping.c
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

#include "physics/opus/physics.h"
#include "physics/opus/physics_private.h"
#include "data_structure/array.h"

void opus_sleeping_wake_up(opus_body *body)
{
	if (body->type != OPUS_BODY_STATIC) {
		body->is_sleeping   = 0;
		body->sleep_counter = 0;
	}
}

void opus_sleeping_fall_asleep(opus_physics_world *world, opus_body *body)
{
	if (body->type != OPUS_BODY_KINEMATIC && body->joint_count == 0) {
		body->is_sleeping   = 1;
		body->sleep_counter = world->body_sleep_delay_counter;
		opus_vec2_set(&body->velocity, 0, 0);
		body->angular_velocity = 0;
		body->motion           = 0;
	}
}

void opus_sleeping_update(opus_physics_world *world, opus_real dt)
{
	uint64_t i;

	opus_body *body;
	opus_real  min_motion, max_motion;
	opus_real  time_factor;

	if (!world->enable_sleeping) return;

	time_factor = dt * dt * dt;
	for (i = 0; i < opus_arr_len(world->bodies); i++) {
		body = world->bodies[i];

		/* an external force is applied */
		if (!opus_vec2_equal(body->force, opus_vec2_(0, 0))) {
			opus_sleeping_wake_up(body);
			continue;
		}

		/* biased average motion estimation between frames */
		min_motion        = opus_min(body->motion, body->prev_motion);
		max_motion        = opus_max(body->motion, body->prev_motion);
		body->prev_motion = body->motion;
		body->motion      = world->body_min_motion_bias * min_motion +
		               (1 - world->body_min_motion_bias) * max_motion;

		if (body->is_sleeping) continue;

		if (world->body_sleep_counter_threshold && body->motion < time_factor * world->body_sleep_motion_threshold) {
			body->sleep_counter++;

			if (body->sleep_counter >= world->body_sleep_counter_threshold)
				opus_sleeping_fall_asleep(world, body);
		} else {
			if (body->sleep_counter > 0)
				body->sleep_counter--;
		}
	}
}

void opus_sleeping_before_resolution(opus_physics_world *world)
{
	uint64_t i;

	opus_contacts *contacts;
	opus_body     *A, *B;

	if (!world->enable_sleeping) return;

	//	opus_hashmap_foreach_start(&world->contacts, contacts, i)
	//	{
	//		contacts = *(opus_contacts **) contacts;
	//
	//		A = contacts->A;
	//		B = contacts->B;
	//
	//		opus_sleeping_wake_up(A);
	//		opus_sleeping_wake_up(B);
	//	}
	//	opus_hashmap_foreach_end();
}

void opus_sleeping_after_collision(opus_physics_world *world, opus_real dt)
{
	int      has_active_contact;
	uint64_t i, j, n;

	opus_contacts *contacts;
	opus_contact  *contact;
	opus_body     *A, *B, *sleeping_body, *moving_body;
	opus_real      wake_threshold, time_factor;

	if (!world->enable_sleeping) return;

	time_factor    = dt * dt * dt;
	wake_threshold = world->body_wake_motion_threshold * time_factor;
	opus_hashmap_foreach_start(&world->contacts, contacts, i)
	{
		contacts = *(opus_contacts **) contacts;

		n = opus_arr_len(contacts->contacts);
		if (n == 0) continue;
		has_active_contact = 0;
		for (j = 0; j < n; j++) {
			contact = contacts->contacts[j];
			if (contact->is_active) {
				has_active_contact       = 1;
				contact->is_active       = 0;
				contact->normal_impulse  = 0;
				contact->tangent_impulse = 0;
			}
		}
		if (!has_active_contact) continue;

		A = contacts->A;
		B = contacts->B;

		if (A->is_sleeping && B->is_sleeping) continue;
		if (A->type == OPUS_BODY_STATIC || B->type == OPUS_BODY_STATIC) continue;

		if (A->is_sleeping || B->is_sleeping) {
			sleeping_body = (A->is_sleeping && A->type != OPUS_BODY_STATIC) ? A : B,
			moving_body   = sleeping_body == A ? B : A;

			if (sleeping_body->type != OPUS_BODY_STATIC && moving_body->motion > wake_threshold) {
				opus_sleeping_wake_up(sleeping_body);
			}
		}
	}
	opus_hashmap_foreach_end();
}
