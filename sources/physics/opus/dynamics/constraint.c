/**
 * @file constraint.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2023/3/14
 *
 * @example
 *
 * @development_log
 *
 */


#include "physics/opus/physics.h"
#include "physics/opus/physics_private.h"

void opus_constraint_destroy(opus_constraint *constraint)
{
	switch (constraint->type) {
		case OPUS_CONSTRAINT_DISTANCE:
			opus_constraint_distance_destroy((void *) constraint);
			break;
		default:
			OPUS_WARNING("opus_constraint_destroy::no matching destroying function\n");
			break;
	}
}

opus_constraint_distance *opus_constraint_distance_create(opus_body *A, opus_body *B, opus_vec2 offset_a, opus_vec2 offset_b)
{
	opus_constraint_distance *constraint;
	constraint = OPUS_CALLOC(1, sizeof(opus_constraint_distance));
	if (constraint) {
		constraint->_.type = OPUS_CONSTRAINT_DISTANCE;



		constraint->_.prepare        = opus_constraint_distance_prepare;
		constraint->_.solve_velocity = opus_constraint_distance_solve_velocity;
	}
	return constraint;
}

void opus_constraint_distance_destroy(opus_constraint_distance *constraint)
{
	OPUS_FREE(constraint);
}

void opus_constraint_distance_prepare(opus_constraint *constraint, opus_real dt)
{
	opus_constraint_distance *dis;

	opus_body *A, *B;

	dis = (void *) constraint;

	A   = dis->A;
	B   = dis->B;

}

static opus_vec2 cross_rv(opus_real r, opus_vec2 v)
{
	return opus_vec2_(-r * v.y, r * v.x);
}

void opus_constraint_distance_solve_velocity(opus_constraint *constraint, opus_real dt)
{
	opus_constraint_distance *dis;

	opus_body *A, *B;

	dis = (void *) constraint;

	A   = dis->A;
	B   = dis->B;
}
