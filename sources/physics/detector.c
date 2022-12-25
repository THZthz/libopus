/**
 * @file detector.c
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
#include "physics/physics.h"

detector_t *detector_alloc()
{
	return (detector_t *) malloc(sizeof(detector_t));
}

detector_t *detector_init(detector_t *detector)
{
	detector->pairs  = NULL;
	array_create(detector->collisions, sizeof(collision_t *));
	array_create(detector->bodies, sizeof(body_t *));
	return detector;
}

detector_t *detector_create()
{
	detector_t *detector = detector_alloc();
	detector             = detector_init(detector);
	return detector;
}

void detector_done(detector_t *detector)
{
	array_destroy(detector->bodies);
	 array_destroy(detector->collisions);
	/* no need to destroy pairs */
}

void detector_destroy(detector_t *detector)
{
	detector_done(detector);
	free(detector);
}

void detector_set_bodies(detector_t *detector, body_t **bodies, count_t n)
{
	array_resize(detector->bodies, n);
	memcpy(detector->bodies, bodies, n * sizeof(body_t *));
}

void detector_clear(detector_t *detector)
{
	array_clear(detector->bodies);
}

static int detector_sort_x(const void *a, const void *b)
{
	body_t *ba = *(body_t **) a;
	body_t *bb = *(body_t **) b;
	return r_sign(ba->bound.min.x - bb->bound.min.x);
}

static int detector_can_collide(uint64_t group_a, uint64_t group_b, uint64_t mask_a, uint64_t mask_b, uint64_t category_a, uint64_t category_b)
{
	if (group_a == group_b && group_a != 0) return group_a > 0;
	return (mask_a & category_b) != 0 && (mask_b & category_a) != 0;
}

collision_t **detector_collisions(detector_t *detector)
{
	count_t       i, j;
	collision_t *collision;

	array_clear(detector->collisions);

	if (detector->bodies == NULL) return NULL;
	qsort(detector->bodies, array_len(detector->bodies), sizeof(body_t *), detector_sort_x);
	for (i = 0; i < array_len(detector->bodies); i++) {
		body_t   *body_a         = detector->bodies[i];
		int body_a_static  = body_a->is_sleeping || body_a->is_static;
		count_t   parts_a_len    = array_len(body_a->parts);
		int parts_a_single = parts_a_len == 1;

		for (j = i + 1; j < array_len(detector->bodies); j++) {
			body_t *body_b      = detector->bodies[j];
			count_t parts_b_len = array_len(body_b->parts);

			if (body_b->bound.min.x > body_a->bound.max.x)
			    break;

			if (body_a->bound.max.y < body_b->bound.min.y || body_a->bound.min.y > body_b->bound.max.y)
			    continue;

			if (body_a_static && (body_b->is_sleeping || body_b->is_static))  continue;

			if (!detector_can_collide(body_a->filter_group, body_b->filter_group,
			                          body_a->filter_mask, body_b->filter_mask,
			                          body_a->filter_category, body_b->filter_category))
			    continue;

			if (parts_a_single && parts_b_len == 1) {
				collision = collision_collides(body_a, body_b, detector->pairs);
				if (collision) {
					array_push(detector->collisions, &collision);
				}
			} else {
				count_t k, z;
				count_t pas = parts_a_len > 1 ? 1 : 0,
				        pbs = parts_b_len > 1 ? 1 : 0;

				for (k = pas; k < parts_a_len; k++) {
					body_t   *part_a   = body_a->parts[k];
					bounds_t *bounds_a = &part_a->bound;

					for (z = pbs; z < parts_b_len; z++) {
						body_t      *part_b   = body_b->parts[z];
						bounds_t    *bounds_b = &part_b->bound;

						if (bounds_a->min.x > bounds_b->max.x || bounds_a->max.x < bounds_b->min.x ||
						    bounds_a->max.y < bounds_b->min.y || bounds_a->min.y > bounds_b->max.y) {
						    continue;
						}

						if ((collision = collision_collides(part_a, part_b, detector->pairs)))
							array_push(detector->collisions, &collision);
					}
				}
			}
		}
	}

	return detector->collisions;
}
