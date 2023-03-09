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

size_t opus_get_physics_id()
{
	if (recycled_ids_len > 0) { return recycled_ids[--recycled_ids_len]; }
	return id_start++;
}

void opus_recycle_physics_id(size_t id)
{
	/* recycle if the cache is not full */
	if (recycled_ids_len < MAX_RECYCLED_ID_SIZE) { recycled_ids[recycled_ids_len++] = id; }
}

size_t hash_(const void *x)
{
	const char *str = x;
	return hashmap_murmur(str, strlen(str), 0, 0);
}

int compare_(const void *a, const void *b)
{
	return strcmp(a, b);
}

opus_physics_world *opus_physics_world_create()
{
	opus_physics_world *world = malloc(sizeof(opus_physics_world));
	if (world) {
		avl_map_init(&world->contacts, hash_, compare_);
		opus_arr_create(world->bodies, sizeof(opus_body *));
		world->position_iteration = 12;
		world->velocity_iteration = 14;

		world->draw_contacts = 0;

		world->position_slop = 0.04;
		world->bias_factor   = 0.4;
		world->rest_factor   = 1;
		opus_vec2_set(&world->gravity, 0, 0);
	}
	return world;
}

opus_body *opus_physics_world_add_polygon(opus_physics_world *world, opus_vec2 position, opus_vec2 *vertices, size_t n)
{
	opus_polygon *shape;
	opus_body    *body;

	shape = opus_shape_polygon_create(vertices, n, opus_vec2_(0, 0));
	body  = opus_body_create();
	opus_body_set_shape(body, (opus_shape *) shape);
	opus_body_set_position(body, position);

	opus_arr_push(world->bodies, &body);

	return body;
}

opus_body *opus_physics_world_add_rect(opus_physics_world *world, opus_vec2 position, opus_real width, opus_real height, opus_real rotation)
{
	opus_vec2  vertices[4];
	opus_body *body;

	width  = width / 2;
	height = height / 2;
	opus_vec2_set(&vertices[0], -width, -height);
	opus_vec2_set(&vertices[1], width, -height);
	opus_vec2_set(&vertices[2], width, height);
	opus_vec2_set(&vertices[3], -width, height);

	body = opus_physics_world_add_polygon(world, position, vertices, 4);

	return body;
}

void opus_physics_world_destroy(opus_physics_world *world)
{
	avl_map_done(&world->contacts);
	opus_arr_destroy(world->bodies);
	free(world);
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

static void prepare_resolution_(opus_physics_world *world, opus_contact *contact)
{
	opus_body *A, *B;
	opus_vec2  p, pa, pb, ra, rb, n, t, va, vb;
	opus_real  emn, emt;
	opus_real  ra_n, rb_n, ra_t, rb_t;
	opus_real  restitution;

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

	/* [J(M^-1)(J^T)]^-1 */
	contact->effective_mass_normal  = opus_equal(emn, 0.f) ? OPUS_REAL_MAX : 1.0f / emn;
	contact->effective_mass_tangent = opus_equal(emt, 0.f) ? OPUS_REAL_MAX : 1.0f / emt;

	restitution = opus_min(A->restitution, B->restitution);

	va = opus_vec2_add(A->velocity, cross_rv(A->angular_velocity, ra));
	vb = opus_vec2_add(B->velocity, cross_rv(B->angular_velocity, rb));

	contact->velocity_bias = opus_vec2_scale(opus_vec2_sub(va, vb), restitution);

	/* warm start */
	if (contact->normal_impulse != 0 || contact->tangent_impulse != 0) {
		p.x = n.x * contact->normal_impulse + t.x * contact->tangent_impulse;
		p.y = n.y * contact->normal_impulse + t.y * contact->tangent_impulse;

		opus_body_apply_impulse(A, opus_vec2_neg(p), ra);
		opus_body_apply_impulse(B, (p), rb);
	}
}

static void check_potential_collision_pair_(opus_body *A, opus_body *B, opus_mat2d ta, opus_mat2d tb, void *data)
{
	size_t i, j;

	opus_body *T;
	opus_overlap_result or ;
	opus_clip_result    cr;
	opus_contact       *contact;
	opus_contacts      *contacts;
	opus_physics_world *world;

	opus_vec2 pa, pb;

	world = data;

	or = opus_SAT(A->shape, B->shape, ta, tb);

	if (or.is_overlap) {
		cr = opus_VCLIP(or);

		if (or.A == B->shape) {
			T = A;
			A = B;
			B = T;
		}

		/* FIXME */
		if (world->draw_contacts || 1) {
			extern plutovg_t *pl;
			for (i = 0; i < cr.n_support; i++) {
				plutovg_set_source_rgba(pl, COLOR_RED, 1);
				plutovg_circle(pl, cr.supports[i][0].x, cr.supports[i][0].y, 3);
				plutovg_fill(pl);
				//				plutovg_set_source_rgba(pl, COLOR_BLUE, 1);
				//				plutovg_circle(pl, cr.supports[i][1].x, cr.supports[i][1].y, 3);
				//				plutovg_fill(pl);
			}
		}

		for (i = 0; i < cr.n_support; i++) {
			pa = cr.supports[i][0];
			pb = cr.supports[i][1];

			if (opus_vec2_dist2(pa, pb) <= world->rest_factor) continue;

			/* check if the contacts exists */
			contacts = avl_map_get_v(&world->contacts, opus_contacts_id(A, B));

			if (!contacts) {
				contacts = opus_contacts_create(A, B);
				avl_map_set(&world->contacts, opus_contacts_id(A, B), contacts);
			}

			for (j = 0; j < opus_arr_len(contacts->contacts); j++) {
				contact = contacts->contacts[j];
				if ((opus_vec2_equal_(contact->pa, pa, 0.01) && opus_vec2_equal_(contact->pa, pb, 0.01)) ||
				    (opus_vec2_equal_(contact->pb, pa, 0.01) && opus_vec2_equal_(contact->pb, pb, 0.01))) {
					contact->depth = or.separation;
					break;
				}
			}

			/* create a new contact if no older contact */
			if (j == opus_arr_len(contacts->contacts)) {
				contact = opus_contact_create(A, B, pa, pb, or.normal, or.separation);
				opus_arr_push(contacts->contacts, &contact);
			}

			prepare_resolution_(world, contact);
		}
	}
}

static opus_real apply_normal_impulse_(opus_physics_world *world, opus_contact *c, opus_real dt)
{
	opus_body *A, *B;

	opus_vec2 va, vb, dv, dp, impulse;
	opus_real v_bias, dv_n, lambda_n;
	opus_real friction, old_impulse_n;

	A = c->A;
	B = c->B;

	va = opus_vec2_add(A->velocity, cross_rv(A->angular_velocity, c->ra));
	vb = opus_vec2_add(B->velocity, cross_rv(B->angular_velocity, c->rb));
	dv = opus_vec2_to(va, vb);

	dp     = opus_vec2_to(c->pa, c->pb);
	v_bias = world->bias_factor / dt * opus_max(0, opus_vec2_len(dp) - world->position_slop);

	dv_n     = opus_vec2_dot(c->normal, dv);
	lambda_n = (-dv_n + v_bias) * c->effective_mass_normal;

	friction = c->normal_impulse;

	old_impulse_n     = c->normal_impulse;
	c->normal_impulse = opus_max(old_impulse_n + lambda_n, 0);
	lambda_n          = c->normal_impulse - old_impulse_n;

	friction += lambda_n;

	impulse = opus_vec2_scale(c->normal, lambda_n);
	opus_body_apply_impulse(A, opus_vec2_neg(impulse), c->ra);
	opus_body_apply_impulse(B, (impulse), c->rb);

	return friction;
}

static void apply_tangent_impulse_(opus_physics_world *world, opus_contact *c, opus_real friction_normal_impulse, opus_real dt)
{

	opus_body *A, *B;

	opus_vec2 va, vb, dv, impulse;
	opus_real dv_t, lambda_t;
	opus_real max_friction, old_tangent_impulse;

	A = c->A;
	B = c->B;

	va = opus_vec2_add(A->velocity, cross_vr(c->ra, A->angular_velocity));
	vb = opus_vec2_add(B->velocity, cross_vr(c->rb, B->angular_velocity));
	dv = opus_vec2_to(va, vb);

	dv_t     = opus_vec2_dot(c->tangent, dv);
	lambda_t = dv_t * c->effective_mass_tangent;

	max_friction = opus_sqrt(A->friction * B->friction) * friction_normal_impulse;

	old_tangent_impulse = c->tangent_impulse;
	c->tangent_impulse  = opus_clamp(old_tangent_impulse + lambda_t, -max_friction, max_friction);
	lambda_t            = c->tangent_impulse - old_tangent_impulse;

	impulse = opus_vec2_scale(c->tangent, lambda_t);
	opus_body_apply_impulse(A, impulse, c->ra);
	opus_body_apply_impulse(B, opus_vec2_neg(impulse), c->rb);
}

static void solve_velocity_(opus_physics_world *world, opus_real dt)
{
	size_t i, j;

	opus_contacts *contacts;
	opus_contact  *contact;
	opus_real      friction_normal_impulse;

	avl_hash_entry_t *entry;

	/* loop through all contacts */
	for (i = 0; i < world->velocity_iteration; i++) {
		entry = avl_map_first(&world->contacts);
		while (entry) {
			contacts = entry->value;

			/* for each contact point */
			for (j = 0; j < opus_arr_len(contacts->contacts); j++) {
				contact = contacts->contacts[j];

				if (!contact->is_active) continue;

				/* solve */
				friction_normal_impulse = apply_normal_impulse_(world, contact, dt);
				apply_tangent_impulse_(world, contact, friction_normal_impulse, dt);
			}

			entry = avl_map_next(&world->contacts, entry);
		}
	}
}

static void solve_position_(opus_physics_world *world, opus_real dt)
{
	size_t i, j;

	opus_contacts    *contacts;
	avl_hash_entry_t *entry;

	opus_contact *c;
	opus_body    *A, *B;
	opus_vec2     dp, impulse;
	opus_real     bias, lambda;
	for (i = 0; i < world->velocity_iteration; i++) {
		entry = avl_map_first(&world->contacts);
		while (entry) {
			contacts = entry->value;

			for (j = 0; j < opus_arr_len(contacts->contacts); j++) {
				c = contacts->contacts[j];
				A = c->A;
				B = c->B;

				if (!c->is_active) continue;

				/* check if position constraint is solved already */
				dp = opus_vec2_to(c->pa, c->pb);
				if (opus_vec2_dot(dp, c->normal) > 0) continue;

				bias    = world->bias_factor / dt * opus_max(opus_vec2_len(dp) - world->position_slop, 0.f);
				lambda  = c->effective_mass_normal * bias;
				impulse = opus_vec2_scale(c->normal, lambda);

				if (A->type != OPUS_BODY_STATIC && !A->is_sleeping) {
					A->position = opus_vec2_sub(A->position, opus_vec2_scale(impulse, A->inv_mass));
					A->rotation -= A->inv_inertia * opus_vec2_cross(c->ra, impulse);
				}
				if (B->type != OPUS_BODY_STATIC && !B->is_sleeping) {
					B->position = opus_vec2_add(B->position, opus_vec2_scale(impulse, B->inv_mass));
					B->rotation += B->inv_inertia * opus_vec2_cross(c->rb, impulse);
				}
			}
			entry = avl_map_next(&world->contacts, entry);
		}
	}
}

static void step_velocity_(opus_physics_world *world, opus_real dt)
{
	size_t i;
	for (i = 0; i < opus_arr_len(world->bodies); i++)
		opus_body_step_velocity(world->bodies[i], world->gravity, 1, 0.9, 0.9, dt);
}

static void integrate_velocity_(opus_physics_world *world, opus_real dt)
{
	size_t i;
	for (i = 0; i < opus_arr_len(world->bodies); i++)
		opus_body_integrate_velocity(world->bodies[i], dt);
}

static void step_position_(opus_physics_world *world, opus_real dt)
{
	size_t i;
	for (i = 0; i < opus_arr_len(world->bodies); i++)
		opus_body_step_position(world->bodies[i], dt);
}

static void clear_forces_(opus_physics_world *world)
{
	size_t i;
	for (i = 0; i < opus_arr_len(world->bodies); i++)
		opus_body_clear_force(world->bodies[i]);
}

static void inactivate_all_contacts_(opus_physics_world *world)
{
	size_t i;

	opus_contacts    *contacts;
	avl_hash_entry_t *entry;
	entry = avl_map_first(&world->contacts);
	while (entry) {
		contacts = entry->value;
		for (i = 0; i < opus_arr_len(contacts->contacts); i++) {
			contacts->contacts[i]->is_active = 0;
			//			opus_contact_destroy(contacts->contacts[i]);
		}
		//		opus_arr_clear(contacts->contacts);

		entry = avl_map_next(&world->contacts, entry);
	}
}

static void retrieve_collision_info_(opus_physics_world *world, opus_real dt)
{
	opus_SAP(world->bodies, opus_arr_len(world->bodies), check_potential_collision_pair_, world);
}

void opus_physics_world_step(opus_physics_world *world, opus_real dt)
{
	step_velocity_(world, dt);
	retrieve_collision_info_(world, dt);
	solve_velocity_(world, dt);
	step_position_(world, dt);
	solve_position_(world, dt);
	clear_forces_(world);
	inactivate_all_contacts_(world);
}
