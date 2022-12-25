/**
 * @file collision->c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/8/25
 *
 * @example
 *
 * @development_log
 *
 */

#include <stdlib.h>

#include "data_structure/array.h"
#include "math/geometry.h"
#include "physics/physics.h"

collision_t *collision_alloc()
{
	return (collision_t *) malloc(sizeof(collision_t));
}

collision_t *collision_init(collision_t *collision, body_t *body_a, body_t *body_b)
{
	collision->pair       = NULL;
	collision->collided   = 0;
	collision->body_a     = body_a;
	collision->body_b     = body_b;
	collision->parent_a   = body_a->parent;
	collision->parent_b   = body_b->parent;
	collision->depth      = 0;
	collision->n_supports = 0;
	collision->ref_count_ = 0;
	vec2_set(collision->normal, 0, 0);
	vec2_set(collision->tangent, 0, 0);
	vec2_set(collision->penetration, 0, 0);
	return collision;
}

collision_t *collision_set_reference(void **ref_obj, collision_t *collision)
{
	if (*ref_obj != collision) collision->ref_count_++;
	*ref_obj = collision;
}

collision_t *collision_create(body_t *body_a, body_t *body_b)
{
	collision_t *collision = collision_alloc();
	collision              = collision_init(collision, body_a, body_b);
	return collision;
}

void collision_done(collision_t *collision)
{
	DO_NOTHING();
}

void collision_destroy(collision_t *collision)
{
	collision_done(collision);
	collision->ref_count_--;
	if (collision->ref_count_ == 0) {
		free(collision);
	}
}

/* get old record at "collision_pairs_t" for more information, otherwise create a new one */
collision_t *collision_get_pair_record(body_t *body_a, body_t *body_b, collision_pairs_t *pairs)
{
	collision_pair_t *pair;
	collision_t      *collision;

	char id[MAX_ID_LEN];
	collision_pair_get_id(id, body_a, body_b);
	pair = pairs ? avl_map_get(collision_pair_t *, &pairs->table, id) : NULL;

	if (!pair) {
		collision           = collision_create(body_a, body_b);
		collision->collided = 1;
		collision->body_a   = body_a->id < body_b->id ? body_a : body_b;
		collision->body_b   = body_a->id < body_b->id ? body_b : body_a;
		collision->parent_a = collision->body_a->parent;
		collision->parent_b = collision->body_b->parent;
	} else {
		collision = pair->collision;
	}
	return collision;
}


collision_t *collision_collides(body_t *body_a, body_t *body_b, collision_pairs_t *pairs)
{
	return collision_sat(body_a, body_b, pairs);
}
