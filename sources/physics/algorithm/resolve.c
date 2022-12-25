/**
 * @file resolver.c
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

#include <stdio.h>

#include "data_structure/array.h"
#include "physics/physics.h"

real RESOLVER_resting_thresh             = 4;
real RESOLVER_resting_thresh_tangent     = 6;
real RESOLVER_position_dampen            = 0.9;
real RESOLVER_position_warming           = 0.8;
real RESOLVER_friction_normal_multiplier = 5;


void resolver_pre_solve_position(collision_pair_t **pairs, count_t n)
{
	count_t i;
	count_t active_count;

	/* find total contacts on each body */
	for (i = 0; i < n; i++) {
		collision_pair_t *pair = pairs[i];

		if (!pair->is_active)
			continue;

		active_count = array_len(pair->active_contacts);
		pair->collision->parent_a->total_contacts += active_count;
		pair->collision->parent_b->total_contacts += active_count;
	}
}

void resolver_solve_position(collision_pair_t **pairs, count_t n, real time_scale)
{
	count_t           i;
	collision_pair_t *pair;
	collision_t      *collision;
	body_t	       *bodyA, *bodyB;
	vec2              normal;
	real              position_impulse;
	real              contact_share;

	/* find impulses required to resolve penetration */
	for (i = 0; i < n; i++) {
		pair = pairs[i];

		if (!pair->is_active || pair->is_sensor)
			continue;

		collision = pair->collision;
		bodyA     = collision->parent_a;
		bodyB     = collision->parent_b;
		normal    = collision->normal;

		/* get current separation between body edges involved in collision */
		pair->separation =
		        normal.x * (bodyB->position_impulse.x + collision->penetration.x - bodyA->position_impulse.x) +
		        normal.y * (bodyB->position_impulse.y + collision->penetration.y - bodyA->position_impulse.y);
	}

	for (i = 0; i < n; i++) {
		pair = pairs[i];

		if (!pair->is_active || pair->is_sensor)
			continue;

		collision        = pair->collision;
		bodyA            = collision->parent_a;
		bodyB            = collision->parent_b;
		normal           = collision->normal;
		position_impulse = (pair->separation - pair->slop) * time_scale;

		if (bodyA->is_static || bodyB->is_static)
			position_impulse *= 2;

		if (!(bodyA->is_static || bodyA->is_sleeping)) {
			contact_share = RESOLVER_position_dampen / (double) bodyA->total_contacts;
			bodyA->position_impulse.x += normal.x * position_impulse * contact_share;
			bodyA->position_impulse.y += normal.y * position_impulse * contact_share;
		}

		if (!(bodyB->is_static || bodyB->is_sleeping)) {
			contact_share = RESOLVER_position_dampen / (double) bodyB->total_contacts;
			bodyB->position_impulse.x -= normal.x * position_impulse * contact_share;
			bodyB->position_impulse.y -= normal.y * position_impulse * contact_share;
		}
	}
}

void resolver_post_solve_position(body_t **bodies, count_t n)
{
	count_t i;
	for (i = 0; i < n; i++) {
		body_t *body               = bodies[i];
		vec2   *position_impulse   = &body->position_impulse;
		real    position_impulse_x = position_impulse->x;
		real    position_impulse_y = position_impulse->y;
		vec2    velocity           = body->velocity;

		/* reset contact count */
		body->total_contacts = 0;

		if (position_impulse_x != 0 || position_impulse_y != 0) {
			/* update body geometry */
			count_t j;
			for (j = 0; j < array_len(body->parts); j++) {
				body_t *part = body->parts[j];
				vertices_translate(part->vertices, array_len(part->vertices), *position_impulse, 1, sizeof(vertex_t));
				vertices_update_aabb(part->vertices, (aabb_t *) &part->bound, velocity, 1);
				part->position.x += position_impulse_x;
				part->position.y += position_impulse_y;
			}

			/* move the body without changing velocity */
			body->position_prev.x += position_impulse_x;
			body->position_prev.y += position_impulse_y;

			if (position_impulse_x * velocity.x + position_impulse_y * velocity.y < 0) {
				/* reset cached impulse if the body has velocity along it */
				position_impulse->x = 0;
				position_impulse->y = 0;
			} else {
				/* warm the next iteration */
				position_impulse->x *= RESOLVER_position_warming;
				position_impulse->y *= RESOLVER_position_warming;
			}
		}
	}
}

void resolver_pre_solve_velocity(collision_pair_t **pairs, count_t n)
{
	count_t              i, j;
	collision_contact_t *contacts;
	count_t              contacts_length;
	collision_t         *collision;
	body_t              *body_a, *body_b;
	vec2                 normal, tangent;
	for (i = 0; i < n; i++) {
		collision_pair_t *pair = pairs[i];

		if (!pair->is_active || pair->is_sensor)
			continue;

		contacts        = pair->active_contacts;
		contacts_length = array_len(contacts);
		collision       = pair->collision;
		body_a          = collision->parent_a;
		body_b          = collision->parent_b;
		normal          = collision->normal,
		tangent         = collision->tangent;

		/*  resolve each contact*/
		for (j = 0; j < contacts_length; j++) {
			collision_contact_t *contact = &contacts[j];

			vertex_t contact_vertex  = contact->vertex;
			real     normal_impulse  = contact->normal_impulse;
			real     tangent_impulse = contact->tangent_impulse;

			if (normal_impulse != 0 || tangent_impulse != 0) {
				/* total impulse from contact */
				real impulse_x = normal.x * normal_impulse + tangent.x * tangent_impulse,
				     impulse_y = normal.y * normal_impulse + tangent.y * tangent_impulse;

				/* apply impulse from contact */
				if (!(body_a->is_static || body_a->is_sleeping)) {
					body_a->position_prev.x += impulse_x * body_a->inv_mass;
					body_a->position_prev.y += impulse_y * body_a->inv_mass;
					body_a->angle_prev += body_a->inv_inertia * ((contact_vertex.x - body_a->position.x) * impulse_y -
					                                             (contact_vertex.y - body_a->position.y) * impulse_x);
				}

				if (!(body_b->is_static || body_b->is_sleeping)) {
					body_b->position_prev.x -= impulse_x * body_b->inv_mass;
					body_b->position_prev.y -= impulse_y * body_b->inv_mass;
					body_b->angle_prev -= body_b->inv_inertia * ((contact_vertex.x - body_b->position.x) * impulse_y -
					                                             (contact_vertex.y - body_b->position.y) * impulse_x);
				}
			}
		}
	}
}

void resolver_solve_velocity(collision_pair_t **pairs, count_t n, real time_scale)
{
	count_t i, j;
	real    time_scale_squared      = time_scale * time_scale,
	     resting_thresh             = RESOLVER_resting_thresh * time_scale_squared,
	     resting_thresh_tangent     = RESOLVER_resting_thresh_tangent * time_scale_squared,
	     friction_normal_multiplier = RESOLVER_friction_normal_multiplier,
	     number_max_value           = REAL_MAX,
	     tangent_impulse,
	     max_friction;

	for (i = 0; i < n; i++) {
		collision_pair_t    *pair = pairs[i];
		collision_t         *collision;
		body_t              *body_a, *body_b;
		vec2                *ba_vel, *bb_vel;
		real                 nx, ny, tx, ty; /* normal and tangent */
		collision_contact_t *contacts;
		count_t              contacts_len;
		real                 contact_share, inv_mass_total, friction;

		if (!pair->is_active || pair->is_sensor)
			continue;

		collision      = pair->collision,
		body_a         = collision->parent_a;
		body_b         = collision->parent_b;
		ba_vel         = &body_a->velocity,
		bb_vel         = &body_b->velocity,
		nx             = collision->normal.x,
		ny             = collision->normal.y,
		tx             = collision->tangent.x,
		ty             = collision->tangent.y,
		contacts       = pair->active_contacts,
		contacts_len   = array_len(contacts),
		contact_share  = 1.0 / (real) contacts_len,
		inv_mass_total = body_a->inv_mass + body_b->inv_mass,
		friction       = pair->friction * pair->friction_static * friction_normal_multiplier * time_scale_squared;

		/* update body velocities */
		ba_vel->x = body_a->position.x - body_a->position_prev.x;
		ba_vel->y = body_a->position.y - body_a->position_prev.y;
		bb_vel->x = body_b->position.x - body_b->position_prev.x;
		bb_vel->y = body_b->position.y - body_b->position_prev.y;

		body_a->angular_velocity = body_a->angle - body_a->angle_prev;
		body_b->angular_velocity = body_b->angle - body_b->angle_prev;

		/* resolve each contact */
		for (j = 0; j < contacts_len; j++) {
			collision_contact_t *contact        = &contacts[j];
			vertex_t            *contact_vertex = &contact->vertex;

			real offset_ax = contact_vertex->x - body_a->position.x,
			     offset_ay = contact_vertex->y - body_a->position.y,
			     offset_bx = contact_vertex->x - body_b->position.x,
			     offset_by = contact_vertex->y - body_b->position.y;

			/* velocity point */
			real vp_ax = ba_vel->x - offset_ay * body_a->angular_velocity,
			     vp_ay = ba_vel->y + offset_ax * body_a->angular_velocity,
			     vp_bx = bb_vel->x - offset_by * body_b->angular_velocity,
			     vp_by = bb_vel->y + offset_bx * body_b->angular_velocity;

			real rel_vel_x = vp_ax - vp_bx,
			     rel_vel_y = vp_ay - vp_by;

			/* normal and tangent velocity */
			real nv = nx * rel_vel_x + ny * rel_vel_y,
			     tv = tx * rel_vel_x + ty * rel_vel_y;

			/* coulomb friction */
			real normal_overlap = pair->separation + nv;
			real normal_force   = normal_overlap < 0 ? 0 : r_min(normal_overlap, 1);

			real friction_limit = normal_force * friction;
			real oAcN, oBcN, share;
			real normal_impulse;
			real impulse_x, impulse_y;

			if (tv > friction_limit || -tv > friction_limit) {
				max_friction    = tv > 0 ? tv : -tv;
				tangent_impulse = pair->friction * (tv > 0 ? 1 : -1) * time_scale_squared;

				if (tangent_impulse < -max_friction) {
					tangent_impulse = -max_friction;
				} else if (tangent_impulse > max_friction) {
					tangent_impulse = max_friction;
				}
			} else {
				tangent_impulse = tv;
				max_friction    = number_max_value;
			}

			/* account for mass, inertia and contact offset */
			oAcN  = offset_ax * ny - offset_ay * nx;
			oBcN  = offset_bx * ny - offset_by * nx;
			share = contact_share / (inv_mass_total + body_a->inv_inertia * oAcN * oAcN + body_b->inv_inertia * oBcN * oBcN);

			/* raw impulses */
			normal_impulse = (1 + pair->restitution) * nv * share;
			tangent_impulse *= share;

			/* handle high velocity and resting collisions separately */
			if (nv * nv > resting_thresh && nv < 0) {
				/* high normal velocity so clear cached contact normal impulse */
				contact->normal_impulse = 0;
			} else {
				/* solve resting collision constraints using Erin Catto's method (GDC08) */
				/* impulse constraint tends to 0 */
				real contact_normal_impulse = contact->normal_impulse;
				contact->normal_impulse += normal_impulse;
				contact->normal_impulse = r_min(contact->normal_impulse, 0);
				normal_impulse          = contact->normal_impulse - contact_normal_impulse;
			}

			/* handle high velocity and resting collisions separately */
			if (tv * tv > resting_thresh_tangent) {
				/* high tangent velocity so clear cached contact tangent impulse */
				contact->tangent_impulse = 0;
			} else {
				/* solve resting collision constraints using Erin Catto's method (GDC08) */
				/* tangent impulse tends to -tangentSpeed or +tangentSpeed */
				real contact_tangent_impulse = contact->tangent_impulse;
				contact->tangent_impulse += tangent_impulse;
				if (contact->tangent_impulse < -max_friction) contact->tangent_impulse = -max_friction;
				if (contact->tangent_impulse > max_friction) contact->tangent_impulse = max_friction;
				tangent_impulse = contact->tangent_impulse - contact_tangent_impulse;
			}

			/* total impulse from contact */
			impulse_x = nx * normal_impulse + tx * tangent_impulse;
			impulse_y = ny * normal_impulse + ty * tangent_impulse;

			/* apply impulse from contact */
			if (!(body_a->is_static || body_a->is_sleeping)) {
				body_a->position_prev.x += impulse_x * body_a->inv_mass;
				body_a->position_prev.y += impulse_y * body_a->inv_mass;
				body_a->angle_prev += (offset_ax * impulse_y - offset_ay * impulse_x) * body_a->inv_inertia;
			}

			if (!(body_b->is_static || body_b->is_sleeping)) {
				body_b->position_prev.x -= impulse_x * body_b->inv_mass;
				body_b->position_prev.y -= impulse_y * body_b->inv_mass;
				body_b->angle_prev -= (offset_bx * impulse_y - offset_by * impulse_x) * body_b->inv_inertia;
			}
		}
	}
}

void resolver_pre_solve_all_constraints(body_t **bodies, count_t n)
{
	count_t i;
	for (i = 0; i < n; i += 1) {
		body_t *body = bodies[i];

		if (body->is_static || (body->constraint_impulse.x == 0 && body->constraint_impulse.y == 0 && body->constraint_impulse_angle == 0)) {
			continue;
		}

		body->position.x += body->constraint_impulse.x;
		body->position.y += body->constraint_impulse.y;
		body->angle += body->constraint_impulse_angle;
	}
}

void resolver_solve_all_constraints(constraint_t **constraints, count_t n, real time_scale)
{
	count_t       i;
	constraint_t *constraint;
	int     fixed_a, fixed_b;

	/* solve fixed constraints */
	for (i = 0; i < n; i += 1) {
		constraint = constraints[i];
		fixed_a    = !constraint->body_a || (constraint->body_a && constraint->body_a->is_static),
		fixed_b    = !constraint->body_b || (constraint->body_b && constraint->body_b->is_static);

		if (fixed_a || fixed_b) {
			constraint_solve(constraints[i], time_scale);
		}
	}

	/* solve free constraints*/
	for (i = 0; i < n; i += 1) {
		constraint = constraints[i];
		fixed_a    = !constraint->body_a || (constraint->body_a && constraint->body_a->is_static);
		fixed_b    = !constraint->body_b || (constraint->body_b && constraint->body_b->is_static);

		if (!fixed_a && !fixed_b) {
			constraint_solve(constraints[i], time_scale);
		}
	}
}

void resolver_post_solve_all_constraints(body_t **bodies, count_t n)
{
	count_t i, j;
	for (i = 0; i < n; i++) {
		body_t *body    = bodies[i];
		vec2   *impulse = &body->constraint_impulse;

		if (body->is_static || (impulse->x == 0 && impulse->y == 0 && body->constraint_impulse_angle == 0)) {
			continue;
		}

		sleeping_set(body, 0);

		/* update geometry and reset */
		for (j = 0; j < array_len(body->parts); j++) {
			body_t *part = body->parts[j];

			vertices_translate(part->vertices, array_len(part->vertices), *impulse, 1, sizeof(vertex_t));

			if (j > 0) {
				part->position.x += impulse->x;
				part->position.y += impulse->y;
			}

			if (body->constraint_impulse_angle != 0) {
				vertices_rotate(part->vertices, array_len(part->vertices), body->position, body->constraint_impulse_angle, sizeof(vertex_t));
				axes_rotate(part->axes, array_len(part->axes), body->constraint_impulse_angle);
				if (j > 0) {
					vec2_rotate_about(part->position, part->position, body->position, body->constraint_impulse_angle);
				}
			}

			vertices_update_aabb(part->vertices, (aabb_t *) &part->bound, body->velocity, 1);
		}

		/* dampen the cached impulse for warming next step */
		body->constraint_impulse_angle *= CONSTRAINT_warming;
		impulse->x *= CONSTRAINT_warming;
		impulse->y *= CONSTRAINT_warming;
	}
}
