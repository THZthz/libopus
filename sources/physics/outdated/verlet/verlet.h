/**
 * @file core_physics.h
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/6/13
 *
 * @brief a simple but flexible verlet physics engine
 *
 * @example
 *
 * @development_log
 *
 */
#ifndef VERLET_H
#define VERLET_H

#include <float.h>
#include <stddef.h>

#include "core/utils/core_bool.h"

#ifndef VERLET_REAL
#define VERLET_REAL float
#define VERLET_REAL_MAX FLT_MAX
#endif /* VERLET_REAL */

typedef VERLET_REAL              verlet_real_t;
typedef struct verlet_vec        verlet_vec_t;
typedef struct verlet_vertex     verlet_vertex_t;
typedef struct verlet_constraint verlet_constraint_t;
typedef struct verlet_body       verlet_body_t;
typedef struct verlet_world      verlet_world_t;

typedef void (*verlet_render_body_cb_t)(verlet_world_t *world, verlet_body_t *body, void *render_context);

struct verlet_vec {
	verlet_real_t x, y;
};

struct verlet_vertex {
	core_bool            pin;
	verlet_body_t *parent;
	verlet_vec_t   position, old_position;
};

struct verlet_constraint {
	verlet_body_t   *parent;
	verlet_real_t    dist;
	verlet_vertex_t *v0, *v1;
};

struct verlet_body {
	size_t                     id;
	verlet_world_t      *world;
	verlet_vec_t         center;
	verlet_vec_t         half_ex;
	verlet_real_t        mass;
	unsigned int               v_count, c_count, e_count;
	verlet_vertex_t     *vertices;
	unsigned int              *edges;
	verlet_constraint_t *constraints;
};

struct verlet_world {
	verlet_real_t           world_bound_x, world_bound_y, world_bound_w, world_bound_h;
	size_t                        n_bodies, n_bodies_capacity;
	verlet_body_t         **bodies;
	void                         *render_context;
	verlet_render_body_cb_t render_body_cb;

	/*
	 * configuration
	 */

	core_bool           enable_world_bound;
	verlet_real_t gravity;
	verlet_real_t friction;
	verlet_real_t friction_ground;
	verlet_real_t viscosity;
	verlet_real_t force_drag;
	size_t              num_iterations;
};

verlet_body_t *verlet_body_create();
void                 verlet_body_destroy(verlet_body_t *body);
void                 verlet_body_set_vertices(verlet_body_t *body, verlet_vec_t *points, size_t n);
void                 verlet_body_set_constraints(verlet_body_t *body, const unsigned int *constraints, size_t n);
void                 verlet_body_set_edges(verlet_body_t *body, const unsigned int *edges, size_t n);

void verlet_body_stick_to(verlet_body_t *body, verlet_vertex_t *drag_vertex, verlet_vec_t pos);

verlet_world_t *verlet_world_create();

void verlet_world_destroy(verlet_world_t *world);
void verlet_world_add(verlet_world_t *world, verlet_body_t *body);
void verlet_world_remove(verlet_world_t *world, verlet_body_t *body);
void verlet_world_update(verlet_world_t *world);
void verlet_world_render(verlet_world_t *world);
void verlet_world_set_render(verlet_world_t *world, verlet_render_body_cb_t cb, void *render_context);

verlet_body_t *verlet_world_add_quad(verlet_world_t *world,
                                                 verlet_vec_t   *points);
verlet_body_t *verlet_world_add_rect(verlet_world_t *world,
                                                 verlet_real_t x, verlet_real_t y,
                                                 verlet_real_t w, verlet_real_t h);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* VERLET_H */
