/**
 * @file pair.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/8/26
 *
 * @example
 *
 * @development_log
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "physics/physics.h"
#include "data_structure/array.h"
#include "data_structure/hashmap.h"

void collision_pair_get_id(char *id_out, body_t *a, body_t *b)
{
	if (a->id < b->id) {
		sprintf(id_out, "A%"PRIu64"B%"PRIu64, a->id, b->id);
	} else {
		sprintf(id_out, "A%"PRIu64"B%"PRIu64, b->id, a->id);
	}
}

collision_pair_t *collision_pair_alloc()
{
	return (collision_pair_t *) malloc(sizeof(collision_pair_t));
}

collision_pair_t *collision_pair_init(collision_pair_t *pair, collision_t *collision, real time_stamp)
{
	collision_pair_get_id(pair->id, collision->body_a, collision->body_b);
	pair->body_a    = collision->body_a;
	pair->body_b    = collision->body_b;
	pair->collision = NULL;
	collision_set_reference((void **) &pair->collision, collision);
	array_create(pair->contacts, sizeof(collision_contact_t));
	array_create(pair->active_contacts, sizeof(collision_contact_t));

	pair->separation      = 0;
	pair->inv_mass        = 0;
	pair->friction        = 0;
	pair->friction_static = 0;
	pair->restitution     = 0;
	pair->slop            = 0;

	pair->is_active        = 1;
	pair->confirmed_active = 1;
	pair->is_sensor        = collision->body_a->is_sensor || collision->body_b->is_sensor;
	pair->time_created     = time_stamp;
	pair->time_updated     = time_stamp;

	collision_pair_update(pair, collision, time_stamp);

	return pair;
}

collision_pair_t *collision_pair_create(collision_t *collision, real time_stamp)
{
	collision_pair_t *pair = collision_pair_alloc();
	pair                   = collision_pair_init(pair, collision, time_stamp);
	return pair;
}

void collision_pair_done(collision_pair_t *pair)
{
	array_destroy(pair->contacts);
	array_destroy(pair->active_contacts);
	collision_destroy(pair->collision);
	pair->collision = NULL;
}

void collision_pair_destroy(collision_pair_t *pair)
{
	collision_pair_done(pair);
	free(pair);
}

void collision_pair_update(collision_pair_t *pair, collision_t *collision, real time_stamp)
{
	count_t i;

	pair->is_active       = 1;
	pair->time_updated    = time_stamp;
	pair->separation      = collision->depth;
	pair->inv_mass        = collision->parent_a->inv_mass + collision->parent_b->inv_mass;
	pair->friction        = r_min(collision->parent_a->friction, collision->parent_b->friction);
	pair->friction_static = r_max(collision->parent_a->friction_static, collision->parent_b->friction_static);
	pair->restitution     = r_max(collision->parent_a->restitution, collision->parent_b->restitution);
	pair->slop            = r_max(collision->parent_a->slop, collision->parent_b->slop);

	collision_set_reference((void **) &pair->collision, collision);

	collision->pair = pair;
	array_clear(pair->active_contacts);
	array_resize(pair->contacts, 2 * r_max(array_len_(collision->parent_a->vertices), array_len_(collision->parent_b->vertices)));
	memset(pair->contacts, 0, sizeof(collision_contact_t) * array_len(pair->contacts));

	for (i = 0; i < collision->n_supports; i++) {
		count_t id = collision->supports[i].index + collision->supports[i].body == collision->parent_a ? 0 : array_len(collision->parent_a->vertices);

		ASSERT(id < array_len_(pair->contacts));
		if (pair->contacts[id].vertex.body != NULL) {
			array_push(pair->active_contacts, &pair->contacts[id]);
		} else {
			pair->contacts[id].vertex          = collision->supports[i];
			pair->contacts[id].normal_impulse  = 0;
			pair->contacts[id].tangent_impulse = 0;
			array_push(pair->active_contacts, &pair->contacts[id]);
		}
	}
}

void collision_pair_set_active(collision_pair_t *pair, int is_active, real time_stamp)
{
	if (is_active) {
		pair->is_active    = 1;
		pair->time_updated = time_stamp;
	} else {
		pair->is_active = 0;
		array_clear(pair->active_contacts);
	}
}

collision_pairs_t *collision_pairs_alloc()
{
	return (collision_pairs_t *) malloc(sizeof(collision_pairs_t));
}

static size_t pairs_hash_str(const void *str)
{
	return hashmap_murmur((void*) str, strlen((char *) str), 0, 7);
}

static int pairs_compare_str(const void *a, const void *b)
{
	return strcmp((char *) a, (char *) b);
}

collision_pairs_t *collision_pairs_init(collision_pairs_t *pairs)
{
	avl_map_init(&pairs->table, pairs_hash_str, pairs_compare_str);
	array_create(pairs->list, sizeof(collision_pair_t *));
	array_create(pairs->collision_start, sizeof(collision_pair_t *));
	array_create(pairs->collision_active, sizeof(collision_pair_t *));
	array_create(pairs->collision_end, sizeof(collision_pair_t *));
	return pairs;
}

collision_pairs_t *collision_pairs_create()
{
	collision_pairs_t *pairs = collision_pairs_alloc();
	pairs                    = collision_pairs_init(pairs);
	return pairs;
}

void collision_pairs_done(collision_pairs_t *pairs)
{
	count_t i;
	avl_map_done(&pairs->table);
	for (i = 0; i < array_len_(pairs->list); i++) collision_pair_destroy(pairs->list[i]);
	array_destroy(pairs->list);
	array_destroy(pairs->collision_start);
	array_destroy(pairs->collision_active);
	array_destroy(pairs->collision_end);
}

void collision_pairs_destroy(collision_pairs_t *pairs)
{
	collision_pairs_done(pairs);
	free(pairs);
}

void collision_pairs_update(collision_pairs_t *pairs, collision_t **collisions, count_t n, real time_stamp)
{
	int      ret; /* returned code of avl_map_add */
	count_t  i;
	count_t *removed_index;
	uint64_t pairs_list_len;

	array_clear(pairs->collision_start);
	array_clear(pairs->collision_active);
	array_clear(pairs->collision_end);

	pairs_list_len = array_len(pairs->list);
	for (i = 0; i < pairs_list_len; i++)
		pairs->list[i]->confirmed_active = 0;

	for (i = 0; i < n; i++) {
		collision_t      *collision = collisions[i];
		collision_pair_t *pair      = collision->pair;

		if (pair) {
			/* pair already exists (but may or may not be active) */
			if (pair->is_active) {
				/* pair exists and is active */
				array_push(pairs->collision_active, &pair);
			} else {
				/* pair exists but was inactive, so a collision has just started again */
				array_push(pairs->collision_start, &pair);
			}

			/* update the pair */
			collision_pair_update(pair, collision, time_stamp);
			pair->confirmed_active = 1;
		} else {
			/* pair did not exist, create a new pair */
			pair = collision_pair_create(collision, time_stamp);
			avl_map_add(&pairs->table, pair->id, pair, &ret);

			/* push the new pair */
			array_push(pairs->collision_start, &pair);
			array_push(pairs->list, &pair);
		}
	}

	/* find pairs that are no longer active */
	array_create(removed_index, sizeof(count_t));
	pairs_list_len = array_len(pairs->list);

	for (i = 0; i < pairs_list_len; i++) {
		collision_pair_t *pair = pairs->list[i];

		if (!pair->confirmed_active) {
			collision_pair_set_active(pair, 0, time_stamp);
			array_push(pairs->collision_end, &pair);

			if (!pair->collision->body_a->is_sleeping && !pair->collision->body_b->is_sleeping) {
				array_push(removed_index, &i);
			}
		}
	}

	/* remove inactive pairs */
	for (i = 0; i < array_len(removed_index); i++) {
		count_t           pair_idx = removed_index[i] - i;
		collision_pair_t *pair     = pairs->list[pair_idx];
		array_remove(pairs->list, pair_idx);
		/*avl_map_set(&pairs->table, pair->id, NULL);*/
		avl_map_remove(&pairs->table, pair->id);
		ASSERT(avl_map_get_v(&pairs->table, pair->id)  == NULL);
		collision_pair_destroy(pair);
	}

	array_destroy(removed_index);
}

void collision_pairs_clear(collision_pairs_t *pairs)
{
	count_t i;
	avl_map_done(&pairs->table);
	for (i = 0; i < array_len_(pairs->list); i++) collision_pair_destroy(pairs->list[i]);
	avl_map_init(&pairs->table, pairs_hash_str, pairs_compare_str);
	array_clear(pairs->collision_start);
	array_clear(pairs->collision_active);
	array_clear(pairs->collision_end);
}
