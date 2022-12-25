/**
 * @file composite.c
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

#include <stdlib.h>

#include "data_structure/array.h"
#include "physics/physics.h"

composite_t *composite_alloc()
{
	return (composite_t *) malloc(sizeof(composite_t));
}

composite_t *composite_init(composite_t *composite)
{
	composite->id                     = common_next_id();
	composite->parent                 = NULL;
	composite->is_modified            = 0;
	composite->cache_.all_bodies      = NULL;
	composite->cache_.all_constraints = NULL;
	composite->cache_.all_composites  = NULL;
	array_create(composite->bodies, sizeof(body_t *));
	array_create(composite->composites, sizeof(composite_t *));
	array_create(composite->constraints, sizeof(constraint_t *));

	return composite;
}

composite_t *composite_create()
{
	composite_t *composite = composite_alloc();
	composite              = composite_init(composite);
	return composite;
}

void composite_done(composite_t *composite)
{
	array_destroy(composite->bodies);
	array_destroy(composite->composites);
	array_destroy(composite->constraints);
	if (composite->cache_.all_bodies) array_destroy(composite->cache_.all_bodies);
	if (composite->cache_.all_constraints) array_destroy(composite->cache_.all_constraints);
	if (composite->cache_.all_composites) array_destroy(composite->cache_.all_composites);
}

void composite_destroy(composite_t *composite)
{
	composite_done(composite);
	free(composite);
}

void composite_set_modified(composite_t *composite, int is_modified, int update_parents, int update_children)
{
	composite->is_modified = is_modified;

	if (is_modified) {
		if (composite->cache_.all_bodies) array_destroy(composite->cache_.all_bodies);
		if (composite->cache_.all_constraints) array_destroy(composite->cache_.all_constraints);
		if (composite->cache_.all_composites) array_destroy(composite->cache_.all_composites);
		composite->cache_.all_bodies      = NULL;
		composite->cache_.all_constraints = NULL;
		composite->cache_.all_composites  = NULL;
	}

	if (update_parents && composite->parent) {
		composite_set_modified(composite->parent, is_modified, update_parents, update_children);
	}

	if (update_children) {
		count_t i;
		for (i = 0; i < array_len(composite->composites); i++)
			composite_set_modified(composite->composites[i], is_modified, update_parents, update_children);
	}
}

void composite_add_composite(composite_t *composite, composite_t *add)
{
	array_push(composite->composites, &add);
	add->parent = composite;
	composite_set_modified(composite, 1, 1, 0);
}

void composite_remove_composite_at(composite_t *composite, count_t position)
{
	array_remove(composite->composites, position);
	composite_set_modified(composite, 1, 1, 0);
}

void composite_remove_composite(composite_t *composite, composite_t *re, int deep)
{
	count_t i;
	for (i = 0; i < array_len(composite->composites); i++) {
		if (composite->composites[i] == re) {
			composite_remove_composite_at(composite, i);
			break;
		}
	}

	if (deep) {
		for (i = 0; i < array_len_(composite->composites); i++) {
			composite_remove_composite(composite->composites[i], re, 1);
		}
	}
}

void composite_add_body(composite_t *composite, body_t *add)
{
	array_push(composite->bodies, &add);
	composite_set_modified(composite, 1, 1, 0);
}

void composite_remove_body_at(composite_t *composite, count_t position)
{
	array_remove(composite->bodies, position);
	composite_set_modified(composite, 1, 1, 0);
}

void composite_remove_body(composite_t *composite, body_t *re, int deep)
{
	count_t i;
	for (i = 0; i < array_len(composite->bodies); i++) {
		if (composite->bodies[i] == re) {
			composite_remove_body_at(composite, i);
			break;
		}
	}

	if (deep) {
		for (i = 0; i < array_len(composite->composites); i++) {
			composite_remove_body(composite->composites[i], re, 1);
		}
	}
}

void composite_add_constraint(composite_t *composite, constraint_t *add)
{
	array_push(composite->constraints, &add);
	composite_set_modified(composite, 1, 1, 0);
}

void composite_remove_constraint_at(composite_t *composite, count_t position)
{
	array_remove(composite->constraints, position);
	composite_set_modified(composite, 1, 1, 0);
}

void composite_remove_constraint(composite_t *composite, constraint_t *re, int deep)
{
	count_t i;
	for (i = 0; i < array_len(composite->constraints); i++) {
		if (composite->constraints[i] == re) {
			composite_remove_constraint_at(composite, i);
			break;
		}
	}

	if (deep) {
		for (i = 0; i < array_len(composite->composites); i++) {
			composite_remove_constraint(composite->composites[i], re, 1);
		}
	}
}

body_t **composite_all_bodies(composite_t *composite)
{
	count_t  i;
	body_t **bodies;
	if (composite->cache_.all_bodies) return composite->cache_.all_bodies;

	array_create(bodies, sizeof(body_t *));
	array_concat(bodies, composite->bodies);

	for (i = 0; i < array_len(composite->composites); i++)
		array_concat(bodies, composite_all_bodies(composite->composites[i]));

	composite->cache_.all_bodies = bodies;

	return bodies;
}

composite_t **composite_all_composites(composite_t *composite)
{
	count_t       i;
	composite_t **composites;
	if (composite->cache_.all_composites) return composite->cache_.all_composites;

	array_create(composites, sizeof(composite_t *));
	array_concat(composites, composite->composites);

	for (i = 0; i < array_len(composite->composites); i++)
		array_concat(composites, composite_all_composites(composite->composites[i]));

	composite->cache_.all_composites = composites;

	return composites;
}

constraint_t **composite_all_constraints(composite_t *composite)
{
	count_t i;
	constraint_t **constraints;
	if (composite->cache_.all_constraints) return composite->cache_.all_constraints;

	array_create(constraints, sizeof(constraint_t *));
	array_concat(constraints, composite->constraints);

	for (i = 0; i < array_len(composite->composites); i++)
		array_concat(constraints, composite_all_constraints(composite->composites[i]));

	composite->cache_.all_constraints = constraints;

	return constraints;
}
