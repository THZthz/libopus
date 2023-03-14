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
#include "math/geometry.h"

enum { OPUS_SHAPE_UNKNOWN,
	   OPUS_SHAPE_POLYGON = 1,
	   OPUS_SHAPE_CIRCLE  = 2 };
enum {
	OPUS_BODY_DYNAMIC   = 1,
	OPUS_BODY_KINEMATIC = 2,
	OPUS_BODY_STATIC    = 3,
	OPUS_BODY_BULLET    = 4
};
enum {
	OPUS_CONSTRAINT_UNKNOWN,
	OPUS_CONSTRAINT_DISTANCE = 1
};
enum {
	OPUS_JOINT_UNKNOWN,
	OPUS_JOINT_DISTANCE = 1,
	OPUS_JOINT_REVOLUTE = 2
};


typedef struct opus_body opus_body;

typedef struct opus_shape   opus_shape;
typedef struct opus_polygon opus_polygon;
typedef struct opus_circle  opus_circle;

typedef struct opus_joint          opus_joint;
typedef struct opus_joint_distance opus_joint_distance;
typedef struct opus_joint_revolute opus_joint_revolute;

typedef struct opus_constraint          opus_constraint;
typedef struct opus_constraint_distance opus_constraint_distance;

typedef struct opus_physics_world opus_physics_world;

typedef opus_vec2 (*opus_get_support_cb)(opus_shape *shape, opus_mat2d transform, opus_vec2 dir, size_t *index);
typedef opus_real (*opus_get_inertia_cb)(opus_shape *shape, opus_real mass);
typedef void (*opus_update_bound_cb)(opus_shape *shape, opus_real rotation, opus_vec2 position);
typedef opus_real (*opus_get_area_cb)(opus_shape *shape);
typedef void (*opus_joint_prepare_cb)(opus_joint *joint, opus_real dt);
typedef void (*opus_joint_solve_velocity_cb)(opus_joint *joint, opus_real dt);
typedef void (*opus_joint_solve_position_cb)(opus_joint *joint, opus_real dt);
typedef void (*opus_constraint_prepare_cb)(opus_constraint *constraint, opus_real dt);
typedef void (*opus_constraint_solve_velocity_cb)(opus_constraint *constraint, opus_real dt);
typedef void (*opus_constraint_solve_position_cb)(opus_constraint *constraint, opus_real dt);

struct opus_physics_world {
	int velocity_iteration;
	int position_iteration;
	int joint_iteration;
	int draw_contacts;

	opus_vec2 gravity;
	opus_real position_slop;
	opus_real position_bias;
	opus_real velocity_bias;
	opus_real constraint_bias;
	opus_real rest_factor;

	opus_body       **bodies;
	opus_joint      **joints;
	opus_constraint **constraints;

	opus_hashmap contacts;

	opus_real  time_scale;
	opus_real  start_time;
	opus_real  current_time;
	opus_real  last_time;
	opus_real  elapsed_time;
	opus_real  delta_min, delta_max;
	opus_real *delta_history;
	size_t     delta_history_max_size;
	int        is_delta_fixed;
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
	opus_get_area_cb     get_area;
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

struct opus_joint {
	int type;

	opus_joint_prepare_cb        prepare;
	opus_joint_solve_position_cb solve_position;
	opus_joint_solve_velocity_cb solve_velocity;
};

struct opus_joint_distance {
	opus_joint _;

	opus_body *body;
	opus_vec2  offset; /* offset of the anchor point in the body, relative to its position */
	opus_vec2  anchor; /* anchor of the world */
	opus_vec2  normal_;
	opus_real  bias_, bias_factor;
	opus_real  min_distance, max_distance;
	opus_real  effective_mass_;
	opus_real  accumulated_impulse_;
};

struct opus_constraint {
	int type;

	opus_constraint_prepare_cb        prepare;
	opus_constraint_solve_velocity_cb solve_velocity;
	opus_constraint_solve_position_cb solve_position;
};

struct opus_constraint_distance {
	opus_constraint _;

	opus_body *A, *B;
};

struct opus_joint_revolute {
	opus_joint _;

	opus_body *A, *B;
	opus_vec2  local_a, local_b;
	opus_real  damping;
	opus_real  damping_ratio;
	opus_real  stiffness;
	opus_real  frequency;
	opus_real  max_force;
	opus_real  gamma;
	opus_vec2  bias;
	opus_vec2  impulse;
	opus_real  effective_mass[4];
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
void         opus_shape_destroy(opus_shape *shape);

opus_body *opus_body_init(opus_body *body);
opus_body *opus_body_create(void);
void       opus_body_done(opus_body *body);
void       opus_body_destroy(opus_body *body);
opus_vec2  opus_body_w2l(opus_body *body, opus_vec2 point);
opus_vec2  opus_body_l2w(opus_body *body, opus_vec2 point);
void       opus_body_set_shape(opus_body *body, opus_shape *shape);
void       opus_body_set_inertia(opus_body *body, opus_real inertia);
void       opus_body_set_mass(opus_body *body, opus_real mass);
opus_real  opus_body_get_area(opus_body *body);
void       opus_body_set_density(opus_body *body, opus_real density);
void       opus_body_set_position(opus_body *body, opus_vec2 position);
void       opus_body_apply_impulse(opus_body *body, opus_vec2 impulse, opus_vec2 r);
void       opus_body_apply_force(opus_body *body, opus_vec2 force, opus_vec2 r);
void       opus_body_clear_force(opus_body *body);
void       opus_body_integrate_velocity(opus_body *body, opus_real dt);
void       opus_body_integrate_forces(opus_body *body, opus_real dt);

opus_physics_world *opus_physics_world_create(void);
void                opus_physics_world_destroy(opus_physics_world *world);

void opus_physics_world_step(opus_physics_world *world, opus_real dt);

opus_body       *opus_physics_world_add_polygon(opus_physics_world *world, opus_vec2 position, opus_vec2 *vertices, size_t n);
opus_body       *opus_physics_world_add_n_polygon(opus_physics_world *world, opus_vec2 position, opus_real radius, int n);
opus_body       *opus_physics_world_add_rect(opus_physics_world *world, opus_vec2 position, opus_real width, opus_real height, opus_real rotation);
opus_body       *opus_physics_world_add_circle(opus_physics_world *world, opus_vec2 position, opus_real radius);
opus_joint      *opus_physics_world_add_distance_joint(opus_physics_world *world,
                                                       opus_body *body, opus_vec2 offset, opus_vec2 anchor,
                                                       opus_real min_distance, opus_real max_distance);
opus_joint      *opus_physics_world_add_revolute_joint(opus_physics_world *world,
                                                       opus_body *A, opus_body *B, opus_vec2 offset_a, opus_vec2 offset_b);
opus_constraint *opus_physics_world_add_distance_constraint(opus_physics_world *world,
                                                            opus_body *A, opus_body *B, opus_vec2 offset_a, opus_vec2 offset_b);

void opus_physics_world_remove_body(opus_physics_world *world, opus_body *body);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* PHYSICS_H */
