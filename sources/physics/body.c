/**
 * @file opus_body->c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/8/24
 *
 * @example
 *
 * @development_log
 *
 */

#include "data_structure/array.h"
#include "utils/utils.h"
#include "physics.h"

#define UNUSED_ID_STACK_SIZE (512)

real     BODY_inertia_scale               = 4;
uint64_t BODY_next_colliding_group_id     = 1;
uint64_t BODY_next_non_colliding_group_id = -1;
uint64_t BODY_next_category               = 0x0001;

static uint64_t body_id_total_count = 0;
static uint64_t unused_id[UNUSED_ID_STACK_SIZE];
static int64_t  unused_id_len = 0;

body_t *body_alloc()
{
	return (body_t *) malloc(sizeof(body_t));
}

static uint64_t get_id_()
{
	uint64_t id;
	if (unused_id_len > 0) {
		id = unused_id[unused_id_len - 1];
		unused_id_len--;
	} else {
		body_id_total_count++;
		id = body_id_total_count;
	}

	return id;
}

static void recycle_id_(uint64_t id)
{
	unused_id[unused_id_len] = id;
	unused_id_len++;
}

uint64_t body_max_id()
{
	return body_id_total_count;
}

body_t *body_init(body_t *body)
{
	body->id = get_id_();
	array_create(body->parts, sizeof(body_t *));
	array_create(body->axes, sizeof(vec2));
	body->angle                    = 0;
	body->angle_prev               = 0;
	body->constraint_impulse_angle = 0;
	body->torque                   = 0;
	body->motion                   = 0;
	body->speed                    = 0;
	body->angular_speed            = 0;
	body->angular_velocity         = 0;
	vec2_set(body->position, 0, 0);
	vec2_set(body->position_prev, 0, 0);
	vec2_set(body->velocity, 0, 0);
	vec2_set(body->force, 0, 0);
	vec2_set(body->position_impulse, 0, 0);
	vec2_set(body->constraint_impulse, 0, 0);
	body->total_contacts  = 0;
	body->is_sensor       = 0;
	body->is_sleeping     = 0;
	body->is_static       = 0;
	body->sleep_threshold = 60;
	body->density         = 0.001;
	body->friction        = 0.001;
	body->friction_air    = 0.001;
	body->friction_static = 0.05;
	body->restitution     = 0;
	body->slop            = 0.05;
	body->time_scale      = 1;
	body->filter_category = 0x0001;
	body->filter_group    = 0;
	body->filter_mask     = 0xFFFFFFFF;
	body->circle_radius   = 0;
	body->area            = 0;
	body->mass            = 0;
	body->inertia         = 0;
	body->sleep_counter   = 0;
	body->inv_mass        = REAL_MAX;
	body->inv_inertia     = REAL_MAX;
	body->original        = NULL;

	/* vertices and bounds are not set */
	body->vertices = NULL;
	array_push(body->parts, &body);
	body->parent = body;

	return body;
}

body_t *body_create()
{
	body_t *body = body_alloc();
	body         = body_init(body);
	return body;
}

void body_done(body_t *body)
{
	recycle_id_(body->id);
	array_destroy(body->axes);
	array_destroy(body->vertices);
	array_destroy(body->parts);
}

void body_destroy(body_t *body)
{
	body_done(body);
	free(body);
}

void body_set_static(body_t *body, int is_static)
{
	count_t i, n;
	for (i = 0, n = array_len(body->parts); i < n; i++) {
		body_t *part    = body->parts[i];
		part->is_static = is_static;

		if (is_static) {
			part->original                   = &part->original_data_;
			part->original_data_.restitution = part->restitution;
			part->original_data_.friction    = part->friction;
			part->original_data_.mass        = part->mass;
			part->original_data_.inertia     = part->inertia;
			part->original_data_.density     = part->density;
			part->original_data_.inv_mass    = part->inv_mass;
			part->original_data_.inv_inertia = part->inv_inertia;

			part->restitution = 0;
			part->friction    = 1;
			part->mass = part->inertia = part->density = REAL_MAX;
			part->inv_mass = part->inv_inertia = 0;

			part->position_prev.x  = part->position.x;
			part->position_prev.y  = part->position.y;
			part->angle_prev       = part->angle;
			part->angular_velocity = 0;
			part->speed            = 0;
			part->angular_speed    = 0;
			part->motion           = 0;
		} else if (part->original) {
			part->restitution = part->original->restitution;
			part->friction    = part->original->friction;
			part->mass        = part->original->mass;
			part->inertia     = part->original->inertia;
			part->density     = part->original->density;
			part->inv_mass    = part->original->inv_mass;
			part->inv_inertia = part->original->inv_inertia;

			part->original = NULL;
		}
	}
}

void body_set_mass(body_t *body, real mass)
{
	real moment       = body->inertia / (body->mass / 6);
	body->inertia     = moment * (mass / 6);
	body->inv_inertia = 1 / body->inertia;

	body->mass     = mass;
	body->inv_mass = 1 / body->mass;
	body->density  = body->mass / body->area;
}

void body_set_density(body_t *body, real density)
{
	body_set_mass(body, density * body->area);
	body->density = density;
}

void body_set_inertia(body_t *body, real inertia)
{
	body->inertia     = inertia;
	body->inv_inertia = 1 / body->inertia;
}

/**
 * notice that if the vertices belongs to the body, namely all the vertex's body property is "body",
 * it will not duplicate a new array of vertices.
 */
void body_set_vertices(body_t *body, vertices_t vertices)
{
	point_t center;
	size_t  n_axes;
	size_t  i;

	vertices_sort_clockwise(vertices, array_len(vertices), sizeof(vertex_t));
	for (i = 0; i < array_len(vertices); i++) vertices[i].index = i;

	/* change vertices */
	if (vertices[0].body == body) {
		body->vertices = vertices;
	} else {
		uint64_t i;
		array_create(body->vertices, sizeof(vertex_t));
		array_resize(body->vertices, array_len(vertices));
		for (i = 0; i < array_len_(vertices); i++) {
			body->vertices[i].x           = vertices[i].x;
			body->vertices[i].y           = vertices[i].x;
			body->vertices[i].index       = i;
			body->vertices[i].is_internal = 0;
			body->vertices[i].body        = body;
		}
	}

	/* update properties */
	array_resize(body->axes, array_len(body->vertices));
	axes_set_from_vertices(body->vertices, array_len(body->vertices), body->axes, &n_axes);
	array_set_len(body->axes, n_axes);
	body->area = vertices_area(body->vertices, array_len(body->vertices), 0, sizeof(vertex_t));
	body_set_mass(body, body->density * body->area);

	/* orient vertices around the center of mass at origin (0, 0) */
	vertices_center(body->vertices, array_len(body->vertices), &center, sizeof(vertex_t));
	vertices_translate(body->vertices, array_len(body->vertices), center, -1, sizeof(vertex_t));

	/* update inertia while vertices are at origin (0, 0) */
	body_set_inertia(body, BODY_inertia_scale * vertices_inertia(body->vertices, array_len(body->vertices), body->mass, sizeof(vertex_t)));

	/* update geometry */
	vertices_translate(body->vertices, array_len(body->vertices), body->position, 1, sizeof(vertex_t));
	vertices_update_aabb(body->vertices, (aabb_t *) &body->bound, body->velocity, 1);
}

void body_get_total_properties(body_t *body, real *mass, real *area, real *inertia, point_t *center)
{
	count_t i;
	*mass    = 0;
	*area    = 0;
	*inertia = 0;

	center->x = 0;
	center->y = 0;

	/* sum the properties of all compound parts of the parent body */
	for (i = array_len(body->parts) == 1 ? 0 : 1; i < array_len(body->parts); i++) {
		body_t *part   = body->parts[i];
		real    t_mass = part->mass != REAL_MAX ? part->mass : 1;

		*mass += t_mass;
		*area += part->area;
		*inertia += part->inertia;
		center->x = center->x + part->position.x * t_mass;
		center->y = center->y + part->position.y * t_mass;
	}

	center->x /= *mass;
	center->y /= *mass;
}

void body_set_parts(body_t *body, body_t **parts, count_t n, int auto_hull)
{
	count_t  i;
	body_t **parts_copy;
	real     t_mass, t_inertia, t_area;
	point_t  center;

	/* add all the parts, ensuring that the first part is always the parent body */
	array_clear(body->parts);
	array_push(body->parts, &body);
	body->parent = body;

	for (i = 0; i < n; i++) {
		body_t *part = parts[i];
		if (part != body) {
			part->parent = body;
			array_push(body->parts, &part);
		}
	}

	if (array_len(body->parts) == 1)
		return;

	/* find the convex hull of all parts to set on the parent body */
	if (auto_hull) {
		/* TODO: auto hull */
	}

	/* sum the properties of all compound parts of the parent body */
	body_get_total_properties(body, &t_mass, &t_area, &t_inertia, &center);
	body->area            = t_area;
	body->parent          = body;
	body->position.x      = center.x;
	body->position.y      = center.y;
	body->position_prev.x = center.x;
	body->position_prev.y = center.y;

	body_set_mass(body, t_mass);
	body_set_inertia(body, t_inertia);
	body_set_position(body, center);
}

void body_set_center(body_t *body, point_t center, int relative)
{
	if (!relative) {
		body->position_prev.x = center.x - (body->position.x - body->position_prev.x);
		body->position_prev.y = center.y - (body->position.y - body->position_prev.y);
		body->position.x      = center.x;
		body->position.y      = center.y;
	} else {
		body->position_prev.x += center.x;
		body->position_prev.y += center.y;
		body->position.x += center.x;
		body->position.y += center.y;
	}
}

void body_set_position(body_t *body, point_t position)
{
	vec2    delta;
	count_t i;
	vec2_sub(delta, position, body->position);
	body->position_prev.x += delta.x;
	body->position_prev.y += delta.y;

	for (i = 0; i < array_len(body->parts); i++) {
		body_t *part = body->parts[i];
		part->position.x += delta.x;
		part->position.y += delta.y;
		vertices_translate(part->vertices, array_len(part->vertices), delta, 1, sizeof(vertex_t));
		vertices_update_aabb(part->vertices, (aabb_t *) &part->bound, body->velocity, 1);
	}
}

void body_set_angle(body_t *body, real angle)
{
	count_t i;
	real    delta = angle - body->angle;
	body->angle_prev += delta;

	for (i = 0; i < array_len(body->parts); i++) {
		body_t *part = body->parts[i];
		part->angle += delta;
		vertices_rotate(part->vertices, array_len(part->vertices), body->position, delta, sizeof(vertex_t));
		axes_rotate(part->axes, array_len(part->axes), delta);
		vertices_update_aabb(part->vertices, (aabb_t *) &part->bound, body->velocity, 1);
		if (i > 0) {
			vec2_rotate_about(part->position, part->position, body->position, delta);
		}
	}
}

void body_set_velocity(body_t *body, vec2 velocity)
{
	body->position_prev.x = body->position.x - velocity.x;
	body->position_prev.y = body->position.y - velocity.y;
	body->velocity.x      = velocity.x;
	body->velocity.y      = velocity.y;
	body->speed           = vec2_len(body->velocity);
}

void body_set_angular_velocity(body_t *body, real angular_velocity)
{
	body->angle_prev       = body->angle - angular_velocity;
	body->angular_velocity = angular_velocity;
	body->angular_speed    = r_abs(body->angular_velocity);
}

void body_translate(body_t *body, vec2 translation)
{
	vec2_add(translation, body->position, translation);
	body_set_position(body, translation);
}

void body_rotate(body_t *body, real rotation, point_t point, int with_point)
{
	if (!with_point) {
		body_set_angle(body, body->angle + rotation);
	} else {
		real c  = r_cos(rotation),
		     s  = r_sin(rotation),
		     dx = body->position.x - point.x,
		     dy = body->position.y - point.y;
		body_set_position(body, vec2_(
		                                point.x + (dx * c - dy * s),
		                                point.y + (dx * s + dy * c)));
		body_set_angle(body, body->angle + rotation);
	}
}

void body_scale(body_t *body, real scale_x, real scale_y, point_t point)
{
	count_t i, n_axes;
	real    total_area = 0,
	     total_inertia = 0;

	for (i = 0; i < array_len(body->parts); i++) {
		body_t *part = body->parts[i];

		/* scale vertices */
		vertices_scale(part->vertices, array_len(part->vertices), point, scale_x, scale_y, sizeof(vertex_t));

		/* update properties */
		array_resize(part->axes, array_len(part->vertices));
		axes_set_from_vertices(part->vertices, array_len(part->vertices), part->axes, &n_axes);
		array_set_len(part->axes, n_axes);
		part->area = vertices_area(part->vertices, array_len(part->vertices), 0, sizeof(vertex_t));
		body_set_mass(part, body->density * part->area);

		/* update inertia (requires vertices to be at origin) */
		vertices_translate(part->vertices, array_len(part->vertices), part->position, -1, sizeof(vertex_t));
		body_set_inertia(part, BODY_inertia_scale * vertices_inertia(part->vertices, array_len(part->vertices), part->mass, sizeof(vertex_t)));
		vertices_translate(part->vertices, array_len(part->vertices), part->position, 1, sizeof(vertex_t));

		if (i > 0) {
			total_area += part->area;
			total_inertia += part->inertia;
		}

		/* scale position */
		part->position.x = point.x + (part->position.x - point.x) * scale_x;
		part->position.y = point.y + (part->position.y - point.y) * scale_y;

		/* update bounds */
		vertices_update_aabb(part->vertices, (aabb_t *) &part->bound, body->velocity, 1);
	}

	/* handle parent body */
	if (array_len(body->parts) > 1) {
		body->area = total_area;

		if (!body->is_static) {
			body_set_mass(body, body->density * total_area);
			body_set_inertia(body, total_inertia);
		}
	}

	/* handle circles */
	if (body->circle_radius) {
		if (scale_x == scale_y) {
			body->circle_radius *= scale_x;
		} else {
			/* body is no longer a circle */
			body->circle_radius = -1;
		}
	}
}

void body_update(body_t *body, real deltaTime, real time_scale, real correction)
{
	count_t i;
	real    delta_time_squared = r_pow2(deltaTime * time_scale * body->time_scale);

	/* from the previous step */
	real friction_air = 1 - body->friction_air * time_scale * body->time_scale,
	     vel_prev_x   = body->position.x - body->position_prev.x,
	     vel_prev_y   = body->position.y - body->position_prev.y;

	/* update velocity with Verlet integration */
	body->velocity.x = (vel_prev_x * friction_air * correction) + (body->force.x / body->mass) * delta_time_squared;
	body->velocity.y = (vel_prev_y * friction_air * correction) + (body->force.y / body->mass) * delta_time_squared;

	body->position_prev.x = body->position.x;
	body->position_prev.y = body->position.y;
	body->position.x += body->velocity.x;
	body->position.y += body->velocity.y;

	/* update angular velocity with Verlet integration */
	body->angular_velocity = ((body->angle - body->angle_prev) * friction_air * correction) + (body->torque / body->inertia) * delta_time_squared;
	body->angle_prev       = body->angle;
	body->angle += body->angular_velocity;

	/* track speed and acceleration */
	body->speed         = vec2_len(body->velocity);
	body->angular_speed = r_abs(body->angular_velocity);

	/* transform the body geometry */
	for (i = 0; i < array_len(body->parts); i++) {
		body_t *part = body->parts[i];

		vertices_translate(part->vertices, array_len_(part->vertices), body->velocity, 1, sizeof(vertex_t));

		if (i > 0) {
			part->position.x += body->velocity.x;
			part->position.y += body->velocity.y;
		}

		if (body->angular_velocity != 0) {
			vertices_rotate(part->vertices, array_len(part->vertices), body->position, body->angular_velocity, sizeof(vertex_t));
			axes_rotate(part->axes, array_len(part->axes), body->angular_velocity);
			if (i > 0) {
				vec2_rotate_about(part->position, part->position, body->position, body->angular_velocity);
			}
		}

		vertices_update_aabb(part->vertices, (aabb_t *) &part->bound, body->velocity, 1);
	}
}

void body_apply_force(body_t *body, point_t position, vec2 force)
{
	vec2 offset;
	body->force.x += force.x;
	body->force.y += force.y;
	vec2_sub(offset, position, body->position);
	body->torque += offset.x * force.y - offset.y * force.x;
}

void body_print_data(body_t *body, FILE *fp)
{
	count_t i;
	fprintf(fp, "Body: %" PRIu64 " (parent: %" PRIu64 ")\n", body->id, body->parent ? body->parent->id : -1);
	fprintf(fp, "position: (%f, %f)\n", body->position.x, body->position.y);
	fprintf(fp, "bounds: (%f, %f) (%f, %f)\n", body->bound.min.x, body->bound.min.y, body->bound.max.x, body->bound.max.y);
	fprintf(fp, "position_prev: (%f, %f)\n", body->position_prev.x, body->position_prev.y);
	fprintf(fp, "angle: %f\n", body->angle);
	fprintf(fp, "angle_prev: %f\n", body->angle_prev);
	fprintf(fp, "vertices(%" PRIu64 "): ", array_len(body->vertices));
	for (i = 0; i < array_len(body->vertices); i++)
		fprintf(fp, "(%.2f, %.2f)", body->vertices[i].x, body->vertices[i].y);
	fprintf(fp, "\n");
	fprintf(fp, "axes(%" PRIu64 "): ", array_len(body->axes));
	for (i = 0; i < array_len(body->axes); i++)
		fprintf(fp, "(%.2f, %.2f)", body->axes[i].x, body->axes[i].y);
	fprintf(fp, "\n");
	fprintf(fp, "total_contacts: %" PRIu64 "\n", body->total_contacts);
	fprintf(fp, "force: (%f, %f)\n", body->force.x, body->force.y);
	fprintf(fp, "torque: %f\n", body->torque);
	fprintf(fp, "speed: %f\n", body->speed);
	fprintf(fp, "angular_speed: %f\n", body->angular_speed);
	fprintf(fp, "velocity: (%f, %f)\n", body->velocity.x, body->velocity.y);
	fprintf(fp, "angular_velocity: %f\n", body->angular_velocity);
	fprintf(fp, "motion: %f\n", body->motion);
	fprintf(fp, "position_impulse: (%f, %f)\n", body->position_impulse.x, body->position_impulse.y);
	fprintf(fp, "constraint_impulse: (%f, %f)\n", body->constraint_impulse.x, body->constraint_impulse.y);
	fprintf(fp, "constraint_impulse_angle: %f\n", body->constraint_impulse_angle);

	fprintf(fp, "area: %f\n", body->area);
	fprintf(fp, "mass: %f\n", body->mass);
	fprintf(fp, "inertia: %f\n", body->inertia);
	fprintf(fp, "density: %f\n", body->density);
	fprintf(fp, "restitution: %f\n", body->restitution);
	fprintf(fp, "friction: %f\n", body->friction);
	fprintf(fp, "friction_air: %f\n", body->friction_air);
	fprintf(fp, "friction_static: %f\n", body->friction_static);

	fprintf(fp, "slop: %f\n", body->slop);
	fprintf(fp, "time_scale: %f\n", body->time_scale);

	fprintf(fp, "sleep_threshold: %f\n", body->sleep_threshold);
	fprintf(fp, "sleep_counter: %f\n", body->sleep_counter);
	fprintf(fp, "is_sensor: %d\n", body->is_sensor);
	fprintf(fp, "is_sleeping: %d\n", body->is_sleeping);
	fprintf(fp, "is_static: %d\n", body->is_static);
}
