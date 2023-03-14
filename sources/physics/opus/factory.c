/**
 * @file factory.c
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
#include "data_structure/array.h"


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

opus_body *opus_physics_world_add_n_polygon(opus_physics_world *world, opus_vec2 position, opus_real radius, int n)
{
	int i;

	opus_body *body;
	opus_vec2 *vertices;
	opus_real  angle, delta_angle = 2 * OPUS_PI / (opus_real) n;

	angle    = 0;
	vertices = OPUS_MALLOC(n * sizeof(opus_vec2));
	for (i = 0; i < n; i++) {
		vertices[i].x = radius * opus_cos(angle);
		vertices[i].y = radius * opus_sin(angle);
		angle += delta_angle;
	}
	body = opus_physics_world_add_polygon(world, position, vertices, n);
	OPUS_FREE(vertices);
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

opus_body *opus_physics_world_add_circle(opus_physics_world *world, opus_vec2 position, opus_real radius)
{
	opus_circle *circle;
	opus_body   *body;

	circle = opus_shape_circle_create(radius);
	body   = opus_body_create();
	opus_body_set_shape(body, (opus_shape *) circle);
	opus_body_set_position(body, position);

	opus_arr_push(world->bodies, &body);

	return body;
}

opus_joint *opus_physics_world_add_distance_joint(opus_physics_world *world,
                                                  opus_body *body, opus_vec2 offset, opus_vec2 anchor,
                                                  opus_real min_distance, opus_real max_distance)
{
	opus_joint_distance *joint;
	joint = opus_joint_distance_create(body, offset, anchor, min_distance, max_distance);
	opus_arr_push(world->joints, &joint);
	return (opus_joint *) joint;
}

opus_constraint *opus_physics_world_add_distance_constraint(opus_physics_world *world,
                                                            opus_body *A, opus_body *B, opus_vec2 offset_a, opus_vec2 offset_b)
{
	opus_constraint_distance *constraint;
	constraint = opus_constraint_distance_create(A, B, offset_a, offset_b);
	opus_arr_push(world->constraints, &constraint);
	return (opus_constraint *) constraint;
}

opus_joint *opus_physics_world_add_revolute_joint(opus_physics_world *world,
                                                  opus_body *A, opus_body *B, opus_vec2 offset_a, opus_vec2 offset_b)
{
	opus_joint_revolute *joint;
	joint = opus_joint_revolute_create(A, B, offset_a, offset_b);
	opus_arr_push(world->joints, &joint);
	return (opus_joint *) joint;
}

void opus_physics_world_remove_body(opus_physics_world *world, opus_body *body)
{
	uint64_t i, j;

	opus_contacts *contacts;
	opus_contact *contact;
	opus_contacts **to_removed;

	/* remove it from world */
	for (i = 0; i < opus_arr_len(world->bodies); i++) {
		if (world->bodies[i] == body) {
			opus_arr_remove(world->bodies, i);
			return;
		}
	}

	/* clear records in contacts map */
	opus_arr_create(to_removed, sizeof(opus_contacts *));
	opus_hashmap_foreach_start(&world->contacts, contacts, i)
	{
		contacts = *(opus_contacts **) contacts;
		if (contacts->A == body || contacts->B == body) opus_arr_push(to_removed, &contacts);
	}
	opus_hashmap_foreach_end();
	for (i = 0; i < opus_arr_len(to_removed); i++) {
		opus_hashmap_delete(&world->contacts, to_removed[i]);
		opus_contacts_destroy(to_removed[i]);
	}
	opus_arr_destroy(to_removed);

	/* destroy this body */
	opus_body_destroy(body);
}





