/**
 * @file joint.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2023/3/13
 *
 * @example
 *
 * @development_log
 *
 */

#include "physics/opus/physics.h"
#include "physics/opus/physics_private.h"

void opus_joint_destroy(opus_joint *joint)
{
	switch (joint->type) {
		case OPUS_JOINT_DISTANCE:
			opus_joint_distance_destroy((void *) joint);
			break;
		case OPUS_JOINT_REVOLUTE:
			opus_joint_revolute_destroy((void *) joint);
			break;
		default:
			OPUS_WARNING("opus_joint_destroy::no matching destroying function\n");
			break;
	}
}

opus_joint_distance *opus_joint_distance_create(opus_body *body, opus_vec2 offset, opus_vec2 anchor, opus_real min_distance, opus_real max_distance)
{
	opus_joint_distance *joint;
	joint = OPUS_CALLOC(1, sizeof(opus_joint_distance));
	if (joint) {
		joint->_.type = OPUS_JOINT_DISTANCE;

		joint->body        = body;
		joint->offset      = offset;
		joint->anchor      = anchor;
		joint->bias_factor = 0.3;

		joint->min_distance = min_distance;
		joint->max_distance = max_distance;
		OPUS_ASSERT(min_distance <= max_distance);

		joint->_.solve_velocity = opus_joint_distance_solve_velocity;
		joint->_.prepare        = opus_joint_distance_prepare;

		body->joint_count++;
	}
	return joint;
}

void opus_joint_distance_destroy(opus_joint_distance *joint)
{
	joint->body->joint_count--;
	OPUS_FREE(joint);
}

void opus_joint_distance_prepare(opus_joint *joint, opus_real dt)
{
	opus_joint_distance *dis = (void *) joint;

	opus_body *A;
	opus_vec2  pa, pb, ra, rb, dp;
	opus_real  len_dp, c;
	opus_real  ima, iia, rna;

	A      = dis->body;
	pa     = opus_body_l2w(A, dis->offset);
	pb     = dis->anchor;
	dp     = opus_vec2_sub(pb, pa);
	len_dp = opus_vec2_len(dp);

	ima = A->inv_mass;
	iia = A->inv_inertia;

	ra = opus_vec2_to(A->position, pa);

	dis->normal_ = opus_vec2_norm(dp);
	if (len_dp < dis->min_distance) {
		c            = dis->min_distance - len_dp;
		dis->normal_ = opus_vec2_neg(dis->normal_);
	} else if (len_dp > dis->max_distance) {
		c = len_dp - dis->max_distance;
	} else {
		dis->accumulated_impulse_ = 0;
		dis->normal_              = opus_vec2_(0, 0);
		dis->bias_                = 0;
		return;
	}
	if (opus_vec2_dot(A->velocity, dis->normal_) > 0) {
		dis->accumulated_impulse_ = 0;
		dis->normal_              = opus_vec2_(0, 0);
		dis->bias_                = 0;
		return;
	}

	rna = opus_vec2_dot(dis->normal_, ra);

	dis->effective_mass_ = 1.0 / (ima + iia * rna * rna);
	dis->bias_           = dis->bias_factor * c / dt;
}

static opus_vec2 cross_rv(opus_real r, opus_vec2 v)
{
	return opus_vec2_(-r * v.y, r * v.x);
}

void opus_joint_distance_solve_velocity(opus_joint *joint, opus_real dt)
{
	opus_joint_distance *dis = (void *) joint;

	opus_vec2 ra, va, dv;
	opus_real jv, jvb, lambda_n, old_impulse;
	opus_vec2 impulse;

	OPUS_RETURN_IF(, dis->bias_ == 0);

	ra = opus_body_l2w(dis->body, dis->offset);
	ra = opus_vec2_sub(ra, dis->body->position);
	va = opus_vec2_add(dis->body->velocity, cross_rv(dis->body->angular_velocity, ra));

	dv = va;

	jv       = opus_vec2_dot(dis->normal_, dv);
	jvb      = -jv + dis->bias_;
	lambda_n = jvb * dis->effective_mass_;

	old_impulse               = dis->accumulated_impulse_;
	dis->accumulated_impulse_ = opus_max(old_impulse + lambda_n, 0);
	lambda_n                  = dis->accumulated_impulse_ - old_impulse;

	impulse = opus_vec2_scale(dis->normal_, lambda_n);
	opus_body_apply_impulse(dis->body, impulse, ra);
}

opus_joint_revolute *opus_joint_revolute_create(opus_body *A, opus_body *B, opus_vec2 offset_a, opus_vec2 offset_b)
{
	opus_joint_revolute *joint;
	joint = OPUS_CALLOC(1, sizeof(opus_joint_revolute));
	if (joint) {
		joint->_.type = OPUS_JOINT_REVOLUTE;

		A->joint_count++;
		B->joint_count ++;

		joint->A       = A;
		joint->B       = B;
		joint->local_a = offset_a;
		joint->local_b = offset_b;

		joint->frequency     = 8.0;
		joint->max_force     = 5000.0;
		joint->damping_ratio = 0.2;
		joint->gamma         = 0.0;

		joint->_.prepare        = opus_joint_revolute_prepare;
		joint->_.solve_velocity = opus_joint_revolute_solve_velocity;
	}
	return joint;
}

void opus_joint_revolute_destroy(opus_joint_revolute *joint)
{
	joint->A->joint_count--;
	joint->B->joint_count--;
	OPUS_FREE(joint);
}

static opus_real constraint_impulse_mixing_(opus_real dt, opus_real stiffness, opus_real damping)
{
	opus_real cim = dt * (dt * stiffness + damping);
	return opus_equal(cim, 0.0) ? 0.0 : 1.0 / cim;
}

static opus_real error_reduction_parameter_(opus_real dt, opus_real stiffness, opus_real damping)
{
	opus_real erp = dt * stiffness + damping;
	return opus_equal(erp, 0.0) ? 0.0 : stiffness / erp;
}

void opus_joint_revolute_prepare(opus_joint *joint, opus_real dt)
{
	opus_joint_revolute *rev = (void *) joint;

	opus_body *A, *B;
	opus_real  ma, ima, iia, mb, imb, iib;
	opus_real  nf, erp;
	opus_vec2  pa, ra, pb, rb;
	opus_real  inv_det, e11, e12, e21, e22;

	A = rev->A;
	B = rev->B;

	ma  = A->mass;
	ima = A->inv_mass;
	iia = A->inv_inertia;
	mb  = B->mass;
	imb = B->inv_mass;
	iib = B->inv_inertia;

	if (rev->frequency > 0.0) {
		nf             = 2 * OPUS_PI * rev->frequency;
		rev->stiffness = (ma + mb) * nf * nf;
		rev->damping   = 2.0 * rev->damping_ratio * (ma + mb) * nf;
	} else {
		rev->stiffness = 0.0;
		rev->damping   = 0.0;
	}

	rev->gamma = constraint_impulse_mixing_(dt, rev->stiffness, rev->damping);

	erp = error_reduction_parameter_(dt, rev->stiffness, rev->damping);

	pa = opus_body_l2w(A, rev->local_a);
	ra = opus_vec2_to(A->position, pa);
	pb = opus_body_l2w(B, rev->local_b);
	rb = opus_vec2_to(B->position, pb);

	rev->bias = opus_vec2_scale(opus_vec2_sub(pa, pb), erp);

	e11 = ima + ra.y * ra.y * iia + imb + rb.y * rb.y * iib + rev->gamma;
	e12 = -ra.x * ra.y * iia - rb.x * rb.y * iib;
	e21 = e12;
	e22 = ima + ra.x * ra.x * iia + imb + rb.x * rb.x * iib + rev->gamma;

	inv_det = e11 * e22 - e12 * e21;
	inv_det = opus_equal(inv_det, 0) ? 0 : 1.0 / inv_det;

	rev->effective_mass[0] = e22 * inv_det;
	rev->effective_mass[1] = -e12 * inv_det;
	rev->effective_mass[2] = -e21 * inv_det;
	rev->effective_mass[3] = e11 * inv_det;

	opus_body_apply_impulse(A, rev->impulse, ra);
	opus_body_apply_impulse(B, opus_vec2_neg(rev->impulse), rb);
}

void opus_joint_revolute_solve_velocity(opus_joint *joint, opus_real dt)
{
	opus_joint_revolute *rev = (void *) joint;

	opus_vec2 ra, rb, va, vb;
	opus_vec2 jvb, J;
	opus_vec2 oldImpulse;
	opus_real maxImpulse;

	ra = opus_vec2_to(rev->A->position, opus_body_l2w(rev->A, rev->local_a));
	rb = opus_vec2_to(rev->B->position, opus_body_l2w(rev->B, rev->local_b));
	va = opus_vec2_add(rev->A->velocity, cross_rv(rev->A->angular_velocity, ra));
	vb = opus_vec2_add(rev->B->velocity, cross_rv(rev->B->angular_velocity, rb));

	jvb = opus_vec2_sub(va, vb);
	jvb = opus_vec2_add(jvb, rev->bias);
	jvb = opus_vec2_add(jvb, opus_vec2_scale(rev->impulse, rev->gamma));
	jvb = opus_vec2_neg(jvb);

	J.x          = rev->effective_mass[0] * jvb.x + rev->effective_mass[1] * jvb.y;
	J.y          = rev->effective_mass[2] * jvb.x + rev->effective_mass[3] * jvb.y;
	oldImpulse   = rev->impulse;
	rev->impulse = opus_vec2_add(rev->impulse, J);
	maxImpulse   = dt * rev->max_force;
	if (opus_vec2_len2(rev->impulse) > maxImpulse * maxImpulse) {
		rev->impulse = opus_vec2_scale(opus_vec2_norm(rev->impulse), maxImpulse);
	}
	J = opus_vec2_sub(rev->impulse, oldImpulse);
	opus_body_apply_impulse(rev->A, J, ra);
	opus_body_apply_impulse(rev->B, opus_vec2_neg(J), rb);
}
