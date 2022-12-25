/**
 * @file constraint.c
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

#include <malloc.h>

#include "utils/utils.h"
#include "physics/physics.h"

real CONSTRAINT_warming       = 0.5;
real CONSTRAINT_torque_dampen = 1;
real CONSTRAINT_min_length    = 0.000001;

constraint_t constraint_(body_t *ba, body_t *bb, vec2 pa, vec2 pb, real len_ratio)
{
	constraint_t constraint;
	constraint.body_a     = ba;
	constraint.body_b     = bb;
	constraint.point_a    = pa;
	constraint.point_b    = pb;
	constraint.len_ratio_ = len_ratio;
	return constraint;
}

constraint_t *constraint_alloc()
{
	return (constraint_t *) malloc(sizeof(constraint_t));
}

constraint_t *constraint_init(constraint_t *constraint, constraint_t op)
{
	/*  calculate static length using initial world space points */
	vec2 pa, pb, diff;
	real length;
	if (op.body_a) vec2_add(pa, op.point_a, op.body_a->position);
	else
		vec2_dup(pa, op.point_a);
	if (op.body_b) vec2_add(op.point_b, op.point_b, op.body_b->position);
	else
		vec2_dup(pb, op.point_b);
	vec2_sub(diff, pa, pb);
	length = vec2_len(diff);

	constraint->length = length * op.len_ratio_;

	constraint->id                = common_next_id();
	constraint->stiffness         = constraint->length > 0 ? 1 : 0.7;
	constraint->damping           = 0;
	constraint->angular_stiffness = 0;
	constraint->body_a            = op.body_a;
	constraint->body_b            = op.body_b;
	vec2_dup(constraint->point_a, op.point_a);
	vec2_dup(constraint->point_b, op.point_b);
	constraint->angle_a = constraint->body_a ? constraint->body_a->angle : constraint->angle_a;
	constraint->angle_b = constraint->body_b ? constraint->body_b->angle : constraint->angle_b;

	return constraint;
}

/* FIXME */
constraint_t *constraint_create(constraint_t op)
{
	constraint_t *constraint = constraint_alloc();
	constraint               = constraint_init(constraint, op);
	return constraint;
}

void constraint_done(constraint_t *constraint)
{
	DO_NOTHING();
}

void constraint_destroy(constraint_t *constraint)
{
	constraint_done(constraint);
	free(constraint);
}

/* FIXME: should be checked again */
void constraint_solve(constraint_t *constraint, real time_scale)
{
	body_t *body_a = constraint->body_a,
	       *body_b = constraint->body_b;
	vec2 *point_a  = &constraint->point_a,
	     *point_b  = &constraint->point_b;
	vec2 point_a_world, pointB_world;
	vec2 delta;
	real current_length;

	real difference, stiffness;
	vec2 force;
	real mass_total, inertia_total, resistance_total, torque, share;
	vec2 normal;
	real n_vel;   /* normal velocity */
	vec2 rel_vel; /* relative velocity */

	if (!body_a && !body_b)
		return;

	/* update reference angle */
	if (body_a && !body_a->is_static) {
		vec2_rotate(*point_a, *point_a, body_a->angle - constraint->angle_a);
		constraint->angle_a = body_a->angle;
	}

	if (body_b && !body_b->is_static) {
		vec2_rotate(*point_b, *point_b, body_b->angle - constraint->angle_b);
		constraint->angle_b = body_b->angle;
	}

	vec2_dup(point_a_world, constraint->point_a);
	vec2_dup(pointB_world, constraint->point_b);
	if (body_a)
		vec2_add(point_a_world, body_a->position, *point_a);
	if (body_b)
		vec2_add(pointB_world, body_b->position, *point_b);

	if ((point_a_world.x == REAL_MAX && point_a_world.y == REAL_MAX) ||
	    (pointB_world.x == REAL_MAX && pointB_world.y == REAL_MAX))
		return;

	vec2_sub(delta, point_a_world, pointB_world);
	current_length = vec2_len(delta);

	/* prevent singularity */
	if (current_length < CONSTRAINT_min_length) {
		current_length = CONSTRAINT_min_length;
	}

	/* solve distance constraint with Gauss-Siedel method */
	difference = (current_length - constraint->length) / current_length;
	stiffness  = constraint->stiffness < 1 ? constraint->stiffness * time_scale : constraint->stiffness;
	vec2_scale(force, delta, difference * stiffness);
	mass_total       = (body_a ? body_a->inv_mass : 0) + (body_b ? body_b->inv_mass : 0);
	inertia_total    = (body_a ? body_a->inv_inertia : 0) + (body_b ? body_b->inv_inertia : 0);
	resistance_total = mass_total + inertia_total;

	if (constraint->damping) {
		vec2 a = {0, 0}, b = {0, 0};
		vec2_scale(normal, delta, 1.0 / current_length);
		if (body_a) vec2_sub(a, body_a->position, body_a->position_prev);
		if (body_b) vec2_sub(b, body_b->position, body_b->position_prev);

		vec2_sub(rel_vel, b, a);

		n_vel = vec2_dot(normal, rel_vel);
	}

	if (body_a && !body_a->is_static) {
		share = body_a->inv_mass / mass_total;

		/* keep track of applied impulses for post solving */
		body_a->constraint_impulse.x -= force.x * share;
		body_a->constraint_impulse.y -= force.y * share;

		/* apply forces */
		body_a->position.x -= force.x * share;
		body_a->position.y -= force.y * share;

		/* apply damping */
		if (constraint->damping) {
			body_a->position_prev.x -= constraint->damping * normal.x * n_vel * share;
			body_a->position_prev.y -= constraint->damping * normal.y * n_vel * share;
		}

		/* apply torque */
		torque = (vec2_cross(*point_a, force) / resistance_total) * CONSTRAINT_torque_dampen * body_a->inv_inertia * (1 - constraint->angular_stiffness);
		body_a->constraint_impulse_angle -= torque;
		body_a->angle -= torque;
	}

	if (body_b && !body_b->is_static) {
		share = body_b->inv_mass / mass_total;

		body_b->constraint_impulse.x += force.x * share;
		body_b->constraint_impulse.y += force.y * share;

		body_b->position.x += force.x * share;
		body_b->position.y += force.y * share;

		if (constraint->damping) {
			body_b->position_prev.x += constraint->damping * normal.x * n_vel * share;
			body_b->position_prev.y += constraint->damping * normal.y * n_vel * share;
		}

		torque = (vec2_cross(*point_b, force) / resistance_total) * CONSTRAINT_torque_dampen * body_b->inv_inertia * (1 - constraint->angular_stiffness);
		body_b->constraint_impulse_angle += torque;
		body_b->angle += torque;
	}
}
