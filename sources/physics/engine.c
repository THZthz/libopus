/**
 * @file engine.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/8/27
 *
 * @example
 *
 * @development_log
 *
 */

#include <stdlib.h>

#include "data_structure/array.h"
#include "external/sokol_time.h"
#include "physics/physics.h"

physics_engine_t *physics_engine_alloc()
{
	return (physics_engine_t *) malloc(sizeof(physics_engine_t));
}

physics_engine_t *physics_engine_init(physics_engine_t *engine)
{
	engine->position_iterations   = 16;
	engine->velocity_iterations   = 4;
	engine->constraint_iterations = 2;
	engine->gravity.x             = 0;
	engine->gravity.y             = 1;
	engine->gravity_scale         = 0.001;
	engine->timing.time_stamp     = 0;
	engine->timing.time_scale     = 1;
	engine->timing.last_delta     = 0;
	engine->timing.last_elapsed   = 0;

	engine->world    = composite_create();
	engine->pairs    = collision_pairs_create();
	engine->detector = detector_create();

	engine->fps       = 60;
	engine->delta     = 1000.0 / engine->fps;
	engine->delta_min = 1000.0 / engine->fps;
	engine->delta_max = 1000.0 / (engine->fps * 0.5);

	engine->correction         = 1;
	engine->delta_sample_size  = 60;
	engine->counter_time_stamp = 0;
	engine->frame_counter      = 0;
	engine->time_prev          = 0;
	engine->time_scale_prev    = 0;
	engine->is_fixed           = 1;
	engine->enabled            = 1;
	engine->enable_sleeping    = 1;

	stm_setup();

	return engine;
}

physics_engine_t *physics_engine_create()
{
	physics_engine_t *engine = physics_engine_alloc();
	engine                   = physics_engine_init(engine);
	return engine;
}

void physics_engine_done(physics_engine_t *engine)
{
	composite_destroy(engine->world);
	collision_pairs_destroy(engine->pairs);
	detector_destroy(engine->detector);
}

void physics_engine_destroy(physics_engine_t *engine)
{
	physics_engine_done(engine);
	free(engine);
}

static void physics_engine_apply_gravity(body_t **bodies, count_t n, vec2 gravity, real gravity_scale)
{
	count_t i;
	if ((gravity.x == 0 && gravity.y == 0) || gravity_scale == 0) {
		return;
	}

	for (i = 0; i < n; i++) {
		body_t *body = bodies[i];

		if (body->is_static || body->is_sleeping)
			continue;

		/* apply gravity */
		body->force.x += body->mass * gravity.x * gravity_scale;
		body->force.y += body->mass * gravity.y * gravity_scale;
	}
}

static void physics_engine_update_bodies(body_t **bodies, count_t n, real delta, real time_scale, real correction)
{
	count_t i;
	for (i = 0; i < n; i++) {
		body_t *body = bodies[i];

		if (body->is_static || body->is_sleeping)
			continue;

		body_update(body, delta, time_scale, correction);
	}
}

static void physics_engine_clear_forces(body_t **bodies, count_t n)
{
	count_t i;
	for (i = 0; i < n; i++) {
		body_t *body = bodies[i];

		body->force.x = 0;
		body->force.y = 0;
		body->torque  = 0;
	}
}

void physics_engine_update(physics_engine_t *engine, real delta, real correction)
{
	real start_time = stm_ms(stm_now());

	physics_timing_t *timing = &engine->timing;

	composite_t       *world      = engine->world;
	detector_t        *detector   = engine->detector;
	collision_pairs_t *pairs      = engine->pairs;
	real               time_stamp = timing->time_stamp;

	collision_t  **collisions;
	body_t       **all_bodies;
	constraint_t **all_constraints;
	count_t        i;

	/* increment timestamp */
	timing->time_stamp += delta * timing->time_scale;
	timing->last_delta = delta * timing->time_scale;

	/* get all bodies and all constraints in the world */
	all_bodies      = composite_all_bodies(world);
	all_constraints = composite_all_constraints(world);

	/* update the detector bodies if they have changed */
	if (world->is_modified) {
		detector_set_bodies(detector, all_bodies, array_len(all_bodies));
	}

	/* reset all composite modified flags */
	if (world->is_modified) {
		composite_set_modified(world, 0, 0, 1);
	}

	/* update sleeping if enabled */
	if (engine->enable_sleeping)
		sleeping_update(all_bodies, array_len(all_bodies), timing->time_scale);

	/* apply gravity to all bodies */
	physics_engine_apply_gravity(all_bodies, array_len(all_bodies), engine->gravity, engine->gravity_scale);

	/* update all body position and rotation by integration */
	physics_engine_update_bodies(all_bodies, array_len(all_bodies), delta, timing->time_scale, correction);

	/* update all constraints (first pass) */
	resolver_pre_solve_all_constraints(all_bodies, array_len(all_bodies));
	for (i = 0; i < engine->constraint_iterations; i++) {
		resolver_solve_all_constraints(all_constraints, array_len(all_constraints), timing->time_scale);
	}
	resolver_post_solve_all_constraints(all_bodies, array_len(all_bodies));

	/* find all collisions */
	detector->pairs = engine->pairs;
	collisions      = detector_collisions(detector);

	/* update collision pairs */
	collision_pairs_update(pairs, collisions, array_len(collisions), time_stamp);

	/* wake up bodies involved in collisions */
	if (engine->enable_sleeping)
		sleeping_after_collisions(pairs->list, array_len(pairs->list), timing->time_scale);

	/* iteratively resolve position between collisions */
	resolver_pre_solve_position(pairs->list, array_len(pairs->list));
	for (i = 0; i < engine->position_iterations; i++) {
		resolver_solve_position(pairs->list, array_len(pairs->list), timing->time_scale);
	}
	resolver_post_solve_position(all_bodies, array_len(all_bodies));

	/* update all constraints (second pass) */
	resolver_pre_solve_all_constraints(all_bodies, array_len(all_bodies));
	for (i = 0; i < engine->constraint_iterations; i++) {
		resolver_solve_all_constraints(all_constraints, array_len(all_constraints), timing->time_scale);
	}
	resolver_post_solve_all_constraints(all_bodies, array_len(all_bodies));

	/* iteratively resolve velocity between collisions */
	resolver_pre_solve_velocity(pairs->list, array_len(pairs->list));
	for (i = 0; i < engine->velocity_iterations; i++) {
		resolver_solve_velocity(pairs->list, array_len(pairs->list), timing->time_scale);
	}

	/* clear force buffers */
	physics_engine_clear_forces(all_bodies, array_len(all_bodies));

	/* log the time elapsed computing this update */
	timing->last_elapsed = stm_ms(stm_now()) - start_time;
}

void physics_engine_tick(physics_engine_t *engine)
{
	physics_timing_t        *timing = &engine->timing;
	real                     time   = stm_ms(stm_now());

	real correction = 1, delta;

	if (engine->is_fixed) {
		delta = engine->delta;
	} else {
		static real     *delta_history = NULL;
		static count_t   idx           = 0;
		static int flag          = 1; /* if true previous data is old data, if false afterwards data is old data */
		static real      min           = 1000.0 / 60;
		count_t          i;

		if ((UNLIKELY(delta_history == NULL)))
			delta_history = (real *) malloc(sizeof(real) * engine->delta_sample_size);

		/* dynamic time step based on wall clock between calls */
		delta                   = time - engine->time_prev;
		engine->time_scale_prev = time;

		/* optimistically filter delta over a few frames, to improve stability */
		if (idx == engine->delta_sample_size) {
			idx  = 0;
			flag = !flag;
		}
		delta_history[idx++] = delta;
		for (i = flag ? 0 : idx; i < (flag ? idx : engine->delta_sample_size); i++)
			if (delta_history[i] < min) min = delta_history[i];
		delta = min;

		delta = delta < engine->delta_min ? engine->delta_min : delta;
		delta = delta > engine->delta_max ? engine->delta_max : delta;

		correction = delta / engine->delta;

		engine->delta = delta;
	}

	/* time correction for time scaling */
	if (engine->time_scale_prev != 0)
		correction *= timing->time_scale / engine->time_scale_prev;

	if (timing->time_scale == 0)
		correction = 0;

	engine->time_scale_prev = timing->time_scale;
	engine->correction      = correction;

	/* fps counter */
	engine->frame_counter += 1;
	if (time - engine->counter_time_stamp >= 1000) {
		engine->fps                = (real) engine->frame_counter * ((time - engine->counter_time_stamp) / 1000);
		engine->counter_time_stamp = time;
		engine->frame_counter      = 0;
	}

	physics_engine_update(engine, delta, correction);
}
