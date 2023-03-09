/**
 * @file physics.h
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
#ifndef PHYSICS_H
#define PHYSICS_H

#include "vg/pluto/plutovg-private.h"
#include "vg/pluto/plutovg.h"
#include "vg/vg_color.h"
#include "vg/vg_utils.h"

#include "math/math.h"
#include "data_structure/hashmap.h"
#include "data_structure/avl_hash.h"
#include "math/geometry.h"

enum { PHYSICS_SHAPE_UNKNOWN = 0,
	   PHYSICS_SHAPE_POLYGON = 1,
	   PHYSICS_SHAPE_CIRCLE  = 2 };
enum {
	OPUS_BODY_DYNAMIC   = 1,
	OPUS_BODY_KINEMATIC = 2,
	OPUS_BODY_STATIC    = 3,
	OPUS_BODY_BULLET    = 4
};

typedef struct opus_body          opus_body;
typedef struct opus_shape         opus_shape;
typedef struct opus_physics_world opus_physics_world;

typedef struct opus_overlap_result opus_overlap_result;
typedef struct opus_clip_result    opus_clip_result;

typedef struct opus_polygon opus_polygon;
typedef struct opus_circle  opus_circle;

typedef opus_vec2 (*opus_get_support_cb)(opus_shape *shape, opus_mat2d transform, opus_vec2 dir, size_t *index);
typedef opus_real (*opus_get_inertia_cb)(opus_shape *shape, opus_real mass);
typedef void (*opus_update_bound_cb)(opus_shape *shape, opus_real rotation, opus_vec2 position);

struct opus_physics_world {
	int velocity_iteration;
	int position_iteration;

	int draw_contacts;

	opus_vec2 gravity;
	opus_real position_slop;
	opus_real bias_factor;
	opus_real rest_factor;

	opus_body **bodies;

	avl_hash_map_t contacts;
};

struct opus_body {
	int         type;
	size_t      id;
	opus_shape *shape;
	uint32_t    bitmask;

	opus_real area;
	opus_real density;
	opus_real mass;
	opus_real inv_mass;

	opus_real inertia;
	opus_real inv_inertia;

	opus_vec2 position;
	opus_vec2 velocity;

	opus_real rotation;
	opus_real angular_velocity;

	opus_vec2 force;
	opus_real torque;

	opus_real friction;
	opus_real restitution;

	opus_real motion;

	opus_body **parts;

	int is_sleeping;
};

struct opus_shape {
	int type_;

	opus_aabb bound;

	opus_get_support_cb  get_support;
	opus_get_inertia_cb  get_inertia;
	opus_update_bound_cb update_bound;
};

struct opus_polygon {
	opus_shape _;

	/**
	 * @brief Should be in CCW order.
	 */
	opus_vec2 *vertices;

	/**
	 * @brief Length of the vertices.
	 */
	size_t n;

	/**
	 * @brief Normally the center of the polygon is on the origin
	 */
	opus_vec2 center;
};

struct opus_circle {
	opus_shape _;
	opus_real  radius;
};


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

opus_polygon *opus_shape_polygon_init(opus_polygon *polygon, opus_vec2 *vertices, size_t n, opus_vec2 center);
opus_polygon *opus_shape_polygon_create(opus_vec2 *vertices, size_t n, opus_vec2 center);
void          opus_shape_polygon_done(opus_polygon *polygon);
void          opus_shape_polygon_destroy(opus_polygon *polygon);

opus_circle *opus_shape_circle_init(opus_circle *circle, opus_real radius);
opus_circle *opus_shape_circle_create(opus_real radius);
void         opus_shape_circle_done(opus_circle *circle);
void         opus_shape_circle_destroy(opus_circle *circle);

opus_body *opus_body_init(opus_body *body);
opus_body *opus_body_create();
void       opus_body_done(opus_body *body);
void       opus_body_destroy(opus_body *body);
opus_vec2  opus_body_w2l(opus_body *body, opus_vec2 point);
opus_vec2  opus_body_l2w(opus_body *body, opus_vec2 point);
void       opus_body_set_shape(opus_body *body, opus_shape *shape);
void       opus_body_set_inertia(opus_body *body, opus_real inertia);
void       opus_body_set_mass(opus_body *body, opus_real mass);
void       opus_body_set_density(opus_body *body, opus_real density);
void       opus_body_set_position(opus_body *body, opus_vec2 position);
void       opus_body_apply_impulse(opus_body *body, opus_vec2 impulse, opus_vec2 r);
void       opus_body_apply_force(opus_body *body, opus_vec2 force, opus_vec2 r);
void       opus_body_clear_force(opus_body *body);
void       opus_body_integrate_velocity(opus_body *body, opus_real dt);
void       opus_body_integrate_forces(opus_body *body, opus_real dt);

opus_physics_world *opus_physics_world_create();
void                opus_physics_world_destroy(opus_physics_world *world);
void                opus_physics_world_step(opus_physics_world *world, opus_real dt);
opus_body          *opus_physics_world_add_polygon(opus_physics_world *world, opus_vec2 position, opus_vec2 *vertices, size_t n);
opus_body          *opus_physics_world_add_rect(opus_physics_world *world, opus_vec2 position, opus_real width, opus_real height, opus_real rotation);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* PHYSICS_H */
