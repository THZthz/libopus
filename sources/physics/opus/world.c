/**
 * @file engine.c
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

#define MAX_RECYCLED_ID_SIZE (128)

size_t id_start         = 1;
size_t recycled_ids_len = 0;
size_t recycled_ids[MAX_RECYCLED_ID_SIZE];

static opus_vec2 cross_vr(opus_vec2 v, opus_real r)
{
	return opus_vec2_(r * v.y, -r * v.x);
}

static opus_vec2 cross_rv(opus_real r, opus_vec2 v)
{
	return opus_vec2_(-r * v.y, r * v.x);
}

size_t opus_get_physics_id(void)
{
	if (recycled_ids_len > 0) { return recycled_ids[--recycled_ids_len]; }
	return id_start++;
}

void opus_recycle_physics_id(size_t id)
{
	/* recycle if the cache is not full */
	if (recycled_ids_len < MAX_RECYCLED_ID_SIZE) { recycled_ids[recycled_ids_len++] = id; }
}

uint64_t hash_(opus_hashmap *map, const void *x, uint64_t seed0, uint64_t seed1, void *data)
{
	char                *key;
	const opus_contacts *contacts;
	contacts = *(opus_contacts **) x;
	key      = opus_contacts_id(contacts->A, contacts->B);
	return opus_hashmap_murmur(key, strlen(key), 0, 0);
}

int compare_(opus_hashmap *map, const void *a, const void *b, void *data)
{
	const opus_contacts *ca = *(opus_contacts **) a;
	const opus_contacts *cb = *(opus_contacts **) b;
	return !(ca->A->id == cb->A->id && ca->B->id == cb->B->id);
}

opus_physics_world *opus_physics_world_create(void)
{
	opus_physics_world *world = OPUS_CALLOC(1, sizeof(opus_physics_world));
	if (world) {
		opus_hashmap_init(&world->contacts, sizeof(opus_contacts *), 2, 0, 0, compare_, hash_, NULL);
		opus_arr_create(world->bodies, sizeof(opus_body *));
		opus_arr_create(world->joints, sizeof(opus_joint *));
		opus_arr_create(world->constraints, sizeof(opus_constraint *));
		opus_arr_create(world->delta_history, sizeof(opus_real));

		world->velocity_iteration = 6;
		world->joint_iteration    = 6;
		world->position_iteration = 20;
		world->position_bias      = 0.012; /* 4 - 0.1 */
		world->velocity_bias      = 0.33;
		world->constraint_bias    = 0.2;
		world->position_slop      = 0.01;
		world->rest_factor        = 0.01;
		opus_vec2_set(&world->gravity, 0, 0);

		world->time_scale   = 1;
		world->elapsed_time = 0.05;
		world->delta_min    = 0.7 * world->elapsed_time;
		world->delta_max    = 2 * world->elapsed_time;

		world->delta_history_max_size = 100;
	}
	return world;
}

static void destroy_contacts_(opus_physics_world *world)
{
	uint64_t i, j;

	opus_contacts *contacts;
	opus_hashmap_foreach_start(&world->contacts, contacts, i)
	{
		contacts = *(opus_contacts **) contacts;
		for (j = 0; j < opus_arr_len(contacts->contacts); j++)
			opus_contact_destroy(contacts->contacts[j]);
		opus_arr_destroy(contacts->contacts);
	}
	opus_hashmap_foreach_end();
}

static void destroy_bodies_(opus_physics_world *world)
{
	size_t i;
	for (i = 0; i < opus_arr_len(world->bodies); i++)
		opus_body_destroy(world->bodies[i]);
}

static void destroy_joints_(opus_physics_world *world)
{
	size_t i;
	for (i = 0; i < opus_arr_len(world->joints); i++)
		opus_joint_destroy(world->joints[i]);
}

static void destroy_constraints_(opus_physics_world *world)
{
	size_t i;
	for (i = 0; i < opus_arr_len(world->constraints); i++)
		opus_constraint_destroy(world->constraints[i]);
}

void opus_physics_world_destroy(opus_physics_world *world)
{
	destroy_contacts_(world);
	opus_hashmap_done(&world->contacts);
	destroy_bodies_(world);
	opus_arr_destroy(world->bodies);
	destroy_joints_(world);
	opus_arr_destroy(world->joints);
	destroy_constraints_(world);
	opus_arr_destroy(world->constraints);
	opus_arr_destroy(world->delta_history);
	OPUS_FREE(world);
}

static void apply_gravity_(opus_physics_world *world, opus_real dt)
{
	opus_body **bodies, *body;
	opus_vec2   force;
	size_t      i, n;

	bodies = world->bodies;
	n      = opus_arr_len(world->bodies);
	for (i = 0; i < n; i++) {
		body  = bodies[i];
		force = opus_vec2_scale(world->gravity, body->mass);
		opus_body_apply_force(body, force, opus_vec2_(0, 0));
		opus_body_integrate_forces(body, dt);
	}
}

static void prepare_resolution_(opus_physics_world *world, opus_contacts *contacts, opus_contact *contact)
{
	opus_body *A, *B;
	opus_vec2  impulse, pa, pb, ra, rb, n, t, va, vb, dv;
	opus_real  emn, emt;
	opus_real  ra_n, rb_n, ra_t, rb_t;
	opus_vec2  bias;
	opus_real  restitution;

	contact->is_active = 1;

	n  = contact->normal;
	t  = contact->tangent;
	A  = contact->A;
	B  = contact->B;
	pa = contact->pa;
	pb = contact->pb;

	contact->ra = opus_vec2_to(A->position, pa);
	contact->rb = opus_vec2_to(B->position, pb);

	ra = contact->ra;
	rb = contact->rb;

	ra_n = opus_vec2_cross(ra, n);
	rb_n = opus_vec2_cross(rb, n);
	ra_t = opus_vec2_cross(ra, t);
	rb_t = opus_vec2_cross(rb, t);

	/* J(M^-1)(J^T) */
	emn = A->inv_mass + B->inv_mass + A->inv_inertia * ra_n * ra_n + B->inv_inertia * rb_n * rb_n;
	emt = A->inv_mass + B->inv_mass + A->inv_inertia * ra_t * ra_t + B->inv_inertia * rb_t * rb_t;
	/* effective mass, namely [J(M^-1)(J^T)]^-1 */
	contact->effective_mass_normal  = opus_equal(emn, 0.f) ? OPUS_REAL_MAX : 1.0f / emn;
	contact->effective_mass_tangent = opus_equal(emt, 0.f) ? OPUS_REAL_MAX : 1.0f / emt;

	va = opus_vec2_add(A->velocity, cross_rv(A->angular_velocity, ra));
	vb = opus_vec2_add(B->velocity, cross_rv(B->angular_velocity, rb));

	dv   = opus_vec2_sub(va, vb);
	bias = opus_vec2_scale(dv, -contacts->restitution);

	contact->restitution_bias = bias;

	/* warm start */
	impulse.x = n.x * contact->normal_impulse + t.x * contact->tangent_impulse;
	impulse.y = n.y * contact->normal_impulse + t.y * contact->tangent_impulse;
	opus_body_apply_impulse(A, opus_vec2_neg(impulse), ra);
	opus_body_apply_impulse(B, (impulse), rb);
}

static void check_potential_collision_pair_(opus_body *A, opus_body *B, opus_mat2d ta, opus_mat2d tb, void *data)
{
	size_t i, j;

	opus_body *T;
	opus_overlap_result or ;
	opus_clip_result    cr;
	opus_contact       *contact;
	opus_contacts      *contacts, key, *key_ptr;
	opus_physics_world *world;

	opus_vec2 pa, pb;

	world = data;

	key_ptr = &key;

	/* check overlapping */
	or = opus_SAT(A->shape, B->shape, ta, tb);

	/* check if the contacts exists */
	key.A    = A->id < B->id ? A : B;
	key.B    = A->id > B->id ? A : B;
	contacts = opus_hashmap_retrieve(&world->contacts, &key_ptr);

	if (!contacts) {
		contacts = opus_contacts_create(A, B);
		opus_hashmap_insert(&world->contacts, &contacts);
	} else {
		contacts = *(opus_contacts **) contacts; /* resolve the pointer to get element */
	}

	if (or.is_overlap) {
		cr = opus_VCLIP(or);

		if (or.A == B->shape) {
			T = A;
			A = B;
			B = T;
		}

		/* FIXME */
		if (world->draw_contacts) {
			extern plutovg_t *pl;
			for (i = 0; i < cr.n_support; i++) {
				plutovg_set_source_rgba(pl, COLOR_RED, 1);
				plutovg_circle(pl, cr.supports[i][0].x, cr.supports[i][0].y, 1);
				plutovg_fill(pl);
			}
		}

		for (i = 0; i < cr.n_support; i++) {
			pa = cr.supports[i][0];
			pb = cr.supports[i][1];

			for (j = 0; j < opus_arr_len(contacts->contacts); j++) {
				contact = contacts->contacts[j];
				if (opus_vec2_equal_(contact->pa, pa, 0.01) ||
				    opus_vec2_equal_(contact->pb, pa, 0.01)) {
					prepare_resolution_(world, contacts, contact);
					break;
				}
			}

			/* create a new contact if no older contact */
			if (j == opus_arr_len(contacts->contacts)) {
				contact = opus_contact_create(A, B, pa, pb, or.normal, or.separation);
				opus_arr_push(contacts->contacts, &contact);
			}

			prepare_resolution_(world, contacts, contact);
		}
	}
}

static void apply_normal_impulse_(opus_physics_world *world, opus_contacts *contacts, opus_contact *contact, opus_real dt)
{
	opus_body *A, *B;

	opus_vec2 va, vb, dv, impulse;
	opus_real dv_n, lambda_n;
	opus_real old_impulse_n;
	opus_real bias, dp;

	A = contact->A;
	B = contact->B;

	va = opus_vec2_add(A->velocity, cross_rv(A->angular_velocity, contact->ra));
	vb = opus_vec2_add(B->velocity, cross_rv(B->angular_velocity, contact->rb));
	dv = opus_vec2_to(va, vb);

	dp       = opus_vec2_len(opus_vec2_sub(contact->pa, contact->pb));
	bias     = world->velocity_bias / dt * opus_max(0, dp - world->position_slop);
	dv_n     = opus_vec2_dot(contact->normal, opus_vec2_sub(dv, contact->restitution_bias));
	lambda_n = (-dv_n + bias) * contact->effective_mass_normal;

	/* clamp normal impulse */
	old_impulse_n = contact->normal_impulse;
	if (dv_n > 0 && dv_n * dv_n < world->rest_factor)
		contact->normal_impulse = 0;
	contact->normal_impulse = opus_max(old_impulse_n + lambda_n, 0);
	lambda_n                = contact->normal_impulse - old_impulse_n;

	if (opus_abs(lambda_n) > OPUS_REAL_EPSILON) {
		impulse = opus_vec2_scale(contact->normal, lambda_n);
		opus_body_apply_impulse(A, opus_vec2_neg(impulse), contact->ra);
		opus_body_apply_impulse(B, (impulse), contact->rb);
	}
}

static void apply_tangent_impulse_(opus_physics_world *world, opus_contacts *contacts, opus_contact *contact, opus_real dt)
{
	opus_body *A, *B;

	opus_vec2 va, vb, dv, impulse;
	opus_real dv_t, lambda_t;
	opus_real max_friction, old_tangent_impulse;

	A = contact->A;
	B = contact->B;

	va = opus_vec2_add(A->velocity, cross_vr(contact->ra, A->angular_velocity));
	vb = opus_vec2_add(B->velocity, cross_vr(contact->rb, B->angular_velocity));
	dv = opus_vec2_to(va, vb);

	dv_t     = opus_vec2_dot(contact->tangent, dv);
	lambda_t = dv_t * contact->effective_mass_tangent;

	max_friction = contacts->friction * contact->normal_impulse;

	old_tangent_impulse = contact->tangent_impulse;
	if (dv_t > 0 && dv_t * dv_t < world->rest_factor)
		contact->tangent_impulse = 0;
	contact->tangent_impulse = opus_clamp(old_tangent_impulse + lambda_t, -max_friction, max_friction);
	lambda_t                 = contact->tangent_impulse - old_tangent_impulse;

	impulse = opus_vec2_scale(contact->tangent, lambda_t / opus_arr_len(contacts->contacts));
	opus_body_apply_impulse(A, impulse, contact->ra);
	opus_body_apply_impulse(B, opus_vec2_neg(impulse), contact->rb);
}

static void solve_velocity_(opus_physics_world *world, opus_real dt)
{
	uint64_t i, j, k;

	opus_contacts   *contacts;
	opus_contact    *contact;
	opus_joint      *joint;
	opus_constraint *constraint;

	for (k = 0; k < world->velocity_iteration; k++) {
		/* solve velocity constraints for rigid bodies */
		opus_hashmap_foreach_start(&world->contacts, contacts, i)
		{
			contacts = *(opus_contacts **) contacts;

			/* for each contact point */
			for (j = 0; j < opus_arr_len(contacts->contacts); j++) {
				contact = contacts->contacts[j];

				if (!contact->is_active) continue;

				/* solve */
				apply_normal_impulse_(world, contacts, contact, dt);
				apply_tangent_impulse_(world, contacts, contact, dt);
			}
		}
		opus_hashmap_foreach_end();

		/* solve velocity constraints for joints */
		for (i = 0; i < opus_arr_len(world->joints); i++) {
			joint = world->joints[i];
			if (joint->solve_velocity) joint->solve_velocity(joint, dt);
		}

		/* solve velocity for constraints */
		for (i = 0; i < opus_arr_len(world->constraints); i++) {
			constraint = world->constraints[i];
			if (constraint->solve_velocity) constraint->solve_velocity(constraint, dt);
		}
	}
}

static void solve_position_(opus_physics_world *world, opus_real dt)
{
	uint64_t i, j, k;

	opus_contact *c;
	opus_body    *A, *B;
	opus_vec2     dp, p;
	opus_real     bias, lambda;

	opus_contacts   *contacts;
	opus_joint      *joint;
	opus_constraint *constraint;

	for (k = 0; k < world->position_iteration; k++) {
		/* solve position constraints for rigid bodies */
		opus_hashmap_foreach_start(&world->contacts, contacts, i)
		{
			contacts = *(opus_contacts **) contacts;

			for (j = 0; j < opus_arr_len(contacts->contacts); j++) {
				c = contacts->contacts[j];
				A = c->A;
				B = c->B;

				if (!c->is_active) continue;

				/* check if position constraint is solved already */
				c->pa = opus_vec2_add(c->A->position, c->ra);
				c->pb = opus_vec2_add(c->B->position, c->rb);
				dp    = opus_vec2_to(c->pa, c->pb);
				if (opus_vec2_dot(dp, c->normal) > 0) continue;

				bias   = world->position_bias / dt * opus_max(opus_vec2_len(dp) - world->position_slop, 0.f);
				lambda = c->effective_mass_normal * bias;
				p      = opus_vec2_scale(c->normal, lambda);

				if (A->type != OPUS_BODY_STATIC && !A->is_sleeping) {
					A->position = opus_vec2_sub(A->position, opus_vec2_scale(p, A->inv_mass));
					A->rotation -= A->inv_inertia * opus_vec2_cross(c->ra, p);
				}
				if (B->type != OPUS_BODY_STATIC && !B->is_sleeping) {
					B->position = opus_vec2_add(B->position, opus_vec2_scale(p, B->inv_mass));
					B->rotation += B->inv_inertia * opus_vec2_cross(c->rb, p);
				}
			}
		}
		opus_hashmap_foreach_end();

		/* solve position constraints for joints */
		for (i = 0; i < opus_arr_len(world->joints); i++) {
			joint = world->joints[i];
			if (joint->solve_position) joint->solve_position(joint, dt);
		}

		/* solve position for constraints */
		for (i = 0; i < opus_arr_len(world->constraints); i++) {
			constraint = world->constraints[i];
			if (constraint->solve_position) constraint->solve_position(constraint, dt);
		}
	}
}

static void integrate_velocity_(opus_physics_world *world, opus_real dt)
{
	size_t i;
	for (i = 0; i < opus_arr_len(world->bodies); i++)
		opus_body_integrate_velocity(world->bodies[i], dt);
}

static void clear_forces_(opus_physics_world *world)
{
	size_t i;
	for (i = 0; i < opus_arr_len(world->bodies); i++)
		opus_body_clear_force(world->bodies[i]);
}

static void inactivate_all_contacts_(opus_physics_world *world)
{
	uint64_t i, j;

	opus_contacts *contacts;
	opus_hashmap_foreach_start(&world->contacts, contacts, i)
	{
		contacts = *(opus_contacts **) contacts;

		for (j = 0; j < opus_arr_len(contacts->contacts); j++)
			contacts->contacts[j]->is_active = 0;
	}
	opus_hashmap_foreach_end();
}

static void clear_inactive_contacts_(opus_physics_world *world)
{
	uint64_t i, j;

	opus_contacts *contacts;
	opus_hashmap_foreach_start(&world->contacts, contacts, i)
	{
		contacts = *(opus_contacts **) contacts;

		for (j = 0; j < opus_arr_len(contacts->contacts); j++) {
			if (!contacts->contacts[j]->is_active) {
				opus_contact_destroy(contacts->contacts[j]);
				opus_arr_remove(contacts->contacts, j);
				j--;
			}
		}

		if (opus_arr_len(contacts->contacts) == 0) {
			opus_hashmap_remove(&world->contacts, &contacts);
			opus_contacts_destroy(contacts);
		}
	}
	opus_hashmap_foreach_end();
}

static void retrieve_collision_info_(opus_physics_world *world, opus_real dt)
{
	opus_SAP(world->bodies, opus_arr_len(world->bodies), check_potential_collision_pair_, world);
}

/**
 * @brief if the world supports dynamic delta time, we should confine the delta
 * 		in order not to make the physics simulation explodes
 * @param world
 * @param dt
 * @return
 */
static opus_real get_nice_dt_(opus_physics_world *world, opus_real dt)
{
	uint64_t i;

	opus_real min_delta = OPUS_REAL_MAX;
	if (!world->is_delta_fixed) {
		world->last_time = world->current_time;

		opus_arr_push(world->delta_history, &dt);
		if (opus_arr_len(world->delta_history) == world->delta_history_max_size) {
			memmove(world->delta_history,
			        world->delta_history + 1,
			        sizeof(opus_real) * (world->delta_history_max_size - 1));
		}
		for (i = 0; i < opus_arr_len(world->delta_history); i++)
			if (world->delta_history[i] < min_delta)
				min_delta = world->delta_history[i];
		dt = opus_clamp(min_delta, world->delta_min, world->delta_max) * world->time_scale;

		world->elapsed_time = dt;
	}
	return dt;
}

static void step_time_(opus_physics_world *world, opus_real dt)
{
	world->current_time += dt;
}

static void prepare_(opus_physics_world *world, opus_real dt)
{
	uint64_t i;

	opus_joint      *joint;
	opus_constraint *constraint;

	for (i = 0; i < opus_arr_len(world->joints); i++) {
		joint = world->joints[i];
		if (joint->prepare) joint->prepare(joint, dt);
	}

	for (i = 0; i < opus_arr_len(world->constraints); i++) {
		constraint = world->constraints[i];
		if (constraint->prepare) constraint->prepare(constraint, dt);
	}
}

void opus_physics_world_step(opus_physics_world *world, opus_real dt)
{
	dt = get_nice_dt_(world, dt);
	/* apply gravity force to rigid bodies and integrate forces, affecting their velocity */
	apply_gravity_(world, dt);
	/* check collision and generate contacts, plus warm start */
	retrieve_collision_info_(world, dt);
	clear_inactive_contacts_(world);
	/* prepare to resolve constraint (other type of constraints and joints) */
	prepare_(world, dt);
	/* solve velocity constraints */
	solve_velocity_(world, dt);
	/* integrate velocity, affect their position */
	integrate_velocity_(world, dt);
	/* solve position constraints, mainly to supplement the resolution of velocity */
	solve_position_(world, dt);
	clear_forces_(world);
	/* prepare for next frame */
	inactivate_all_contacts_(world);
	step_time_(world, dt);
}
