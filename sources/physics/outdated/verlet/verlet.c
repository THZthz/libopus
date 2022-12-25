/**
 * @file core_physics.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/2/13
 *
 * @example
 *
 * @development_log
 *
 */

#include "verlet.h"

#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define VERLET_ABS(x) ((verlet_real_t) fabs((double) (x)))

size_t g_id_count = 0;

verlet_vec_t         g_test_axis;
verlet_vec_t         g_axis;
verlet_vec_t         g_center;
verlet_vec_t         g_line;
verlet_vec_t         g_response;
verlet_vec_t         g_rel_vel;
verlet_vec_t         g_tangent;
verlet_vec_t         g_rel_tangent_vel;
verlet_real_t        g_depth;
verlet_constraint_t *g_edge;
verlet_vertex_t     *g_vertex;

void verlet_vec_set(verlet_vec_t *vec, verlet_real_t x, verlet_real_t y)
{
	vec->x = x;
	vec->y = y;
}

verlet_vec_t verlet_vec_neg(verlet_vec_t vec)
{
	verlet_vec_t ret;
	ret.x = -vec.x;
	ret.y = -vec.y;
	return ret;
}

verlet_vec_t verlet_vec_sub(verlet_vec_t va, verlet_vec_t vb)
{
	verlet_vec_t ret;
	ret.x = va.x - vb.x;
	ret.y = va.y - vb.y;
	return ret;
}

verlet_vec_t verlet_vec_scale(verlet_vec_t v, verlet_real_t s)
{
	verlet_vec_t ret;
	ret.x = v.x * s;
	ret.y = v.y * s;
	return ret;
}

verlet_real_t verlet_vec_dot(verlet_vec_t va, verlet_vec_t vb)
{
	return va.x * vb.x + va.y * vb.y;
}

verlet_real_t verlet_vec_square_dist(verlet_vec_t va, verlet_vec_t vb)
{
	return (va.x - vb.x) * (va.x - vb.x) + (va.y - vb.y) * (va.y - vb.y);
}

verlet_vec_t verlet_vec_perp(verlet_vec_t v)
{
	verlet_vec_t ret;
	ret.x = -v.y;
	ret.y = v.x;
	return ret;
}

verlet_vec_t verlet_vec_normal(verlet_vec_t v0, verlet_vec_t v1)
{
	verlet_vec_t ret;
	/* perpendicular */
	verlet_real_t nx = v0.y - v1.y, ny = v1.x - v0.x;
	/* normalize */
	verlet_real_t len = (verlet_real_t) 1.0 / (verlet_real_t) sqrt((double) (nx * nx + ny * ny));
	ret.x                   = nx * len;
	ret.y                   = ny * len;
	return ret;
}

void verlet_verlet_integrate(verlet_world_t *world, verlet_vertex_t *vertex)
{
	verlet_vec_t *p;
	verlet_real_t x, y;

	if (vertex->pin) return;

	p = &vertex->position;
	x = vertex->position.x, y = vertex->position.y;

	p->x += world->viscosity * p->x - world->viscosity * p->x;
	p->y += world->viscosity * p->y - world->viscosity * vertex->old_position.y + world->gravity;

	verlet_vec_set(&vertex->old_position, x, y);

	/* screen limits */
	if (world->enable_world_bound) {
		if (p->y < world->world_bound_y) p->y = 0;
		else if (p->y > world->world_bound_y + world->world_bound_h) {
			p->x -= (p->y - world->world_bound_y + world->world_bound_h) * (p->x - x) * world->friction_ground;
			p->y = world->world_bound_y + world->world_bound_h;
		}

		if (p->x < world->world_bound_x) p->x = 0;
		else if (p->x > world->world_bound_x + world->world_bound_w)
			p->x = world->world_bound_x + world->world_bound_w;
	}
}

verlet_body_t *verlet_body_create()
{
	verlet_body_t *body = (verlet_body_t *) malloc(sizeof(verlet_body_t));
	if (body == NULL) return NULL;
	body->id   = g_id_count++;
	body->mass = (verlet_real_t) 1.0;
	return body;
}

void verlet_body_destroy(verlet_body_t *body)
{
	free(body->vertices);
	free(body->constraints);
	free(body->edges);
	free(body);
}

void verlet_body_set_vertices(verlet_body_t *body, verlet_vec_t *points, size_t n)
{
	size_t i;
	body->vertices = (verlet_vertex_t *) malloc(sizeof(verlet_vertex_t) * n);
	body->v_count  = n;
	for (i = 0; i < n; i++) {
		verlet_vertex_t *v;
		v                 = &body->vertices[i];
		v->parent         = body;
		v->pin            = core_false;
		v->position.x     = points[i].x;
		v->position.y     = points[i].y;
		v->old_position.x = points[i].x;
		v->old_position.y = points[i].y;
	}
}

/**
 * @brief do not check identical constraint here
 * @param constraints {1, 2, 3, 4, ...}
 * @param n should be a multiple of 2
 */
void verlet_body_set_constraints(verlet_body_t *body, const unsigned int *constraints, size_t n)
{
	size_t i;
	body->constraints = (verlet_constraint_t *) malloc(sizeof(verlet_constraint_t) * n / 2);
	body->c_count     = n / 2;
	for (i = 0; i < n / 2; i++) {
		verlet_constraint_t *constraint;
		constraint         = &body->constraints[i];
		constraint->parent = body;
		constraint->v0     = &body->vertices[constraints[i * 2]];
		constraint->v1     = &body->vertices[constraints[i * 2 + 1]];
		constraint->dist   = verlet_vec_square_dist(constraint->v0->position, constraint->v1->position);
	}
}

/**
 * @brief set the outline of the body, should be a convex polygon
 * @param body
 * @param edges indices of the constraints which compose the outline
 * @param n length edges
 */
void verlet_body_set_edges(verlet_body_t *body, const unsigned int *edges, size_t n)
{
	size_t i;
	body->edges   = (unsigned int *) malloc(sizeof(unsigned int) * n);
	body->e_count = n;
	for (i = 0; i < n; i++) body->edges[i] = edges[i];
}

void verlet_body_stick_to(verlet_body_t *body, verlet_vertex_t *drag_vertex, verlet_vec_t pos)
{
	/* correct position */
	verlet_real_t s = drag_vertex->parent->mass * body->world->force_drag;
	drag_vertex->position.x += (pos.x - drag_vertex->position.x) / s;
	drag_vertex->position.y += (pos.y - drag_vertex->position.y) / s;
}

void verlet_body_update_bounding_box(verlet_body_t *body)
{
	size_t              i;
	verlet_real_t min_x = VERLET_REAL_MAX, min_y = VERLET_REAL_MAX, max_x = -VERLET_REAL_MAX, max_y = -VERLET_REAL_MAX;
	for (i = 0; i < body->v_count; i++) {
		verlet_vec_t *p = &body->vertices[i].position;

		if (p->x > max_x) max_x = p->x;
		if (p->y > max_y) max_y = p->y;
		if (p->x < min_x) min_x = p->x;
		if (p->y < min_y) min_y = p->y;
	}

	/* center */
	verlet_vec_set(&body->center, (verlet_real_t) ((min_x + max_x) * 0.5), (verlet_real_t) ((min_y + max_y) * 0.5));

	/* half extents */
	verlet_vec_set(&body->half_ex, (verlet_real_t) ((max_x - min_x) * 0.5), (verlet_real_t) ((max_y - min_y) * 0.5));
}

void verlet_body_project_to_axis(verlet_body_t *body, verlet_vec_t axis, verlet_real_t *min, verlet_real_t *max)
{
	size_t              i;
	verlet_real_t d = verlet_vec_dot(body->vertices[0].position, axis);
	*min = *max = d;

	for (i = 1; i < body->v_count; i++) {
		d = verlet_vec_dot(body->vertices[i].position, axis);
		if (d > *max) *max = d;
		if (d < *min) *min = d;
	}
}

void verlet_constraint_solve(verlet_constraint_t *constraint)
{
	verlet_real_t dx = constraint->v1->position.x - constraint->v0->position.x;
	verlet_real_t dy = constraint->v1->position.y - constraint->v0->position.y;

	/* using square root approximation */
	verlet_real_t delta = constraint->dist / (dx * dx + dy * dy + constraint->dist) - (verlet_real_t) 0.5;

	dx *= delta;
	dy *= delta;

	if (!constraint->v1->pin) {
		constraint->v1->position.x += dx;
		constraint->v1->position.y += dy;
	}
	if (!constraint->v0->pin) {
		constraint->v0->position.x -= dx;
		constraint->v0->position.y -= dy;
	}
}

core_bool verlet_SAT(verlet_body_t *B0, verlet_body_t *B1)
{
	verlet_real_t min_distance, t, smallest_dist;
	unsigned int        n0, n1;
	size_t              i, n;

	/* broad phase */
	if (!(0 > VERLET_ABS(B1->center.x - B0->center.x) - (B1->half_ex.x + B0->half_ex.x) &&
	      0 > VERLET_ABS(B1->center.y - B0->center.y) - (B1->half_ex.y + B0->half_ex.y)))
		return core_false; /* no aabb overlap */

	/* narrow phase */
	min_distance = VERLET_REAL_MAX;
	n0 = B0->e_count, n1 = B1->e_count;

	/* iterate through all the edges of both bodies */
	for (i = 0, n = n0 + n1; i < n; i++) {
		verlet_real_t min0, max0, min1, max1;
		verlet_real_t dist;

		/* get edge */
		verlet_constraint_t *edge = i < n0 ? &B0->constraints[B0->edges[i]] : &B1->constraints[B1->edges[i - n0]];

		/* calculate the perpendicular to this edge and normalize it */
		g_test_axis = verlet_vec_normal(edge->v0->position, edge->v1->position);

		/* project both bodies onto the normal */
		verlet_body_project_to_axis(B0, g_test_axis, &min0, &max0);
		verlet_body_project_to_axis(B1, g_test_axis, &min1, &max1);

		/* calculate the distance between the two intervals */
		dist = min0 < min1 ? min1 - max0 : min0 - max1;

		/* if the intervals don't overlap, return, since there is no collision */
		if (dist > 0) return core_false;
		else if (VERLET_ABS(dist) < min_distance) {
			min_distance = (verlet_real_t) VERLET_ABS(dist);

			/* save collision information */
			g_axis = g_test_axis;
			g_edge = edge;
		}
	}

	g_depth = min_distance;

	/* ensure collision edge in B1 and collision vertex in B0 */
	if (g_edge->parent != B1) {
		verlet_body_t *temp;
		temp = B1;
		B1   = B0;
		B0   = temp;
	}

	/* make sure that the collision normal is pointing at B1 */
	g_center = verlet_vec_sub(B0->center, B1->center);
	t        = verlet_vec_dot(g_center, g_axis);

	/* revert the collision normal if it points away from B1 */
	if (t < 0) g_axis = verlet_vec_neg(g_axis);

	smallest_dist = VERLET_REAL_MAX;
	for (i = 0; i < B0->v_count; i++) {
		verlet_real_t dist;
		/* measure the distance of the vertex from the line using the line equation */
		verlet_vertex_t *v = &B0->vertices[i];
		g_line                   = verlet_vec_sub(v->position, B1->center);
		dist                     = verlet_vec_dot(g_axis, g_line);

		/* set the smallest distance and the collision vertex */
		if (dist < smallest_dist) {
			smallest_dist = dist;
			g_vertex      = v;
		}
	}

	/* there is no separating axis. Report a collision! */
	return core_true;
}

void verlet_resolve_collision(verlet_world_t *world)
{
	verlet_real_t t, lambda;
	verlet_real_t m0, m1, tm;
	verlet_real_t relTv;
	verlet_vec_t *rt;

	/* cache vertices positions */
	verlet_vec_t *p0 = &g_edge->v0->position,
	                   *p1 = &g_edge->v1->position,
	                   *o0 = &g_edge->v0->old_position,
	                   *o1 = &g_edge->v1->old_position,
	                   *vp = &g_vertex->position,
	                   *vo = &g_vertex->old_position,
	                   *rs = &g_response;

	/* response vector */
	g_response = verlet_vec_scale(g_axis, g_depth);

	/* calculate where on the edge the collision vertex lies */
	t      = VERLET_ABS(p0->x - p1->x) > VERLET_ABS(p0->y - p1->y)
	                 ? (vp->x - rs->x - p0->x) / (p1->x - p0->x)
	                 : (vp->y - rs->y - p0->y) / (p1->y - p0->y);
	lambda = 1 / (t * t + (1 - t) * (1 - t));

	/* mass coefficient */
	m0 = g_vertex->parent->mass;
	m1 = g_edge->parent->mass;
	tm = m0 + m1;
	m0 = m0 / tm;
	m1 = m1 / tm;

	/* apply the collision response */
	p0->x -= rs->x * (1 - t) * lambda * m0;
	p0->y -= rs->y * (1 - t) * lambda * m0;
	p1->x -= rs->x * t * lambda * m0;
	p1->y -= rs->y * t * lambda * m0;

	vp->x += rs->x * m1;
	vp->y += rs->y * m1;

	/*
	 * collision friction
	 */

	/* compute relative velocity */
	verlet_vec_set(&g_rel_vel,
	                     vp->x - vo->x - (p0->x + p1->x - o0->x - o1->x) * (verlet_real_t) 0.5,
	                     vp->y - vo->y - (p0->y + p1->y - o0->y - o1->y) * (verlet_real_t) 0.5);

	/* axis perpendicular */
	g_tangent = verlet_vec_perp(g_axis);

	/* project the relative velocity onto tangent */
	relTv = verlet_vec_dot(g_rel_vel, g_tangent);
	verlet_vec_set(&g_rel_tangent_vel, g_tangent.x * relTv, g_tangent.y * relTv);
	rt = &g_rel_tangent_vel;

	/* apply tangent friction */
	vo->x += rt->x * world->friction * m1;
	vo->y += rt->y * world->friction * m1;

	o0->x -= rt->x * (1 - t) * world->friction * lambda * m0;
	o0->y -= rt->y * (1 - t) * world->friction * lambda * m0;
	o1->x -= rt->x * t * world->friction * lambda * m0;
	o1->y -= rt->y * t * world->friction * lambda * m0;
}

void verlet_world_integrate(verlet_world_t *world)
{
	size_t i, j;
	for (i = 0; i < world->n_bodies; i++) {
		verlet_body_t *body = world->bodies[i];
		for (j = 0; j < body->v_count; j++)
			verlet_verlet_integrate(world, &body->vertices[j]);
	}
}

void verlet_world_solve(verlet_world_t *world)
{
	size_t n, i, j;
	for (n = 0; n < world->num_iterations; n++) {
		/* solve constraints */
		for (i = 0; i < world->n_bodies; i++)
			for (j = 0; j < world->bodies[i]->c_count; j++)
				verlet_constraint_solve(&world->bodies[i]->constraints[j]);

		/* recalculate the bounding boxes */
		for (i = 0; i < world->n_bodies; i++)
			verlet_body_update_bounding_box(world->bodies[i]);

		/* collisions detection and resolution */
		for (i = 0; i < world->n_bodies - 1; i++) {
			verlet_body_t *b0 = world->bodies[i];
			for (j = i + 1; j < world->n_bodies; j++) {
				verlet_body_t *b1 = world->bodies[j];
				if (verlet_SAT(b0, b1)) verlet_resolve_collision(world);
			}
		}
	}
}

void verlet_world_update(verlet_world_t *world)
{
	verlet_world_integrate(world);
	verlet_world_solve(world);
}

void verlet_world_set_render(verlet_world_t *world, verlet_render_body_cb_t cb, void *render_context)
{
	world->render_context = render_context;
	world->render_body_cb = cb;
}

void verlet_world_render(verlet_world_t *world)
{
	size_t i;
	if (world->render_body_cb == NULL) return;
	for (i = 0; i < world->n_bodies; i++) {
		verlet_body_t *body = world->bodies[i];
		world->render_body_cb(world, body, world->render_context);
	}
}

verlet_world_t *verlet_world_create()
{
	verlet_world_t *world = (verlet_world_t *) malloc(sizeof(verlet_world_t));
	if (world == NULL) return NULL;
	world->render_body_cb    = NULL;
	world->render_context    = NULL;
	world->n_bodies          = 0;
	world->n_bodies_capacity = 10;
	world->bodies            = (verlet_body_t **) malloc(sizeof(verlet_body_t *) * 10);
	world->world_bound_x     = 0;
	world->world_bound_y     = 0;
	world->world_bound_w     = 800;
	world->world_bound_h     = 800;
	world->num_iterations    = 5;
	world->gravity           = (verlet_real_t) 0.01;
	world->friction          = (verlet_real_t) 0.2;
	world->friction_ground   = (verlet_real_t) 0.1;
	world->viscosity         = (verlet_real_t) 1.0;
	world->force_drag        = (verlet_real_t) 7.0;

	return world;
}

void verlet_world_destroy(verlet_world_t *world)
{
	size_t i;
	for (i = 0; i < world->n_bodies; i++) verlet_body_destroy(world->bodies[i]);
	free(world->bodies);
	free(world);
}

void verlet_world_add(verlet_world_t *world, verlet_body_t *body)
{
	if (world->n_bodies_capacity == world->n_bodies) {
		world->n_bodies_capacity *= 2;
		world->bodies = (verlet_body_t **) realloc(world->bodies, world->n_bodies_capacity);
		if (world->bodies == NULL) fprintf(stderr, "ERROR: verlet_world_add failed to allocate memory");
	}

	world->bodies[world->n_bodies++] = body;
	body->world = world;
}

verlet_body_t * verlet_world_add_quad(verlet_world_t *world,
                                                 verlet_vec_t *points)
{
	unsigned int         constraints[12] = {0, 1, 1, 2, 2, 3, 3, 0, 0, 2, 1, 3};
	unsigned int         edges[4]        = {0, 1, 2, 3};
	verlet_body_t *quad            = verlet_body_create();
	if (quad == NULL) return NULL;
	verlet_body_set_vertices(quad, points, 4);
	verlet_body_set_constraints(quad, constraints, 12);
	verlet_body_set_edges(quad, edges, 4);
	verlet_world_add(world, quad);
	return quad;
}

verlet_body_t *verlet_world_add_rect(verlet_world_t *world,
                                                 verlet_real_t x, verlet_real_t y,
                                                 verlet_real_t w, verlet_real_t h)
{
	verlet_body_t *rect;
	verlet_vec_t   points[4];
	verlet_vec_set(&points[0], x, y);
	verlet_vec_set(&points[1], x + w, y);
	verlet_vec_set(&points[2], x + w, y + h);
	verlet_vec_set(&points[3], x, y + h);
	rect = verlet_world_add_quad(world, points);
	return rect;
}

void verlet_world_remove(verlet_world_t *world, verlet_body_t *body)
{
	size_t i;
	for (i = 0; i < world->n_bodies; i++) {
		if (world->bodies[i] == body) break;
	}
	/* the pointer's content is what we want, and it is not the last of the bodies list */
	if (i != world->n_bodies && world->bodies[i] == body && world->n_bodies - i - 1 != 0) {
		verlet_body_destroy(world->bodies[i]);
		memmove(&world->bodies[i], &world->bodies[i + 1], sizeof(verlet_body_t *) * (world->n_bodies - i - 1));
		world->n_bodies--;
	}
}








































