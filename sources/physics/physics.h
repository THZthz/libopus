/**
 * @file physics.h
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
#ifndef PHYSICS_H
#define PHYSICS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdio.h>

#include "data_structure/avl_hash.h"
#include "math/geometry.h"
#include "utils/event.h"
#include "utils/utils.h"


#define cast_bounds(ptr) ((bounds_t *) (ptr))
#define MAX_ID_LEN (16)

typedef uint64_t                 count_t;
typedef vec2                     point_t;
typedef point_t                 *points_t;
typedef struct bounds            bounds_t;
typedef struct vertex            vertex_t;
typedef struct body              body_t;
typedef struct collision         collision_t;
typedef struct collision_pair    collision_pair_t;
typedef struct collision_pairs   collision_pairs_t;
typedef vec2                    *axes_t;
typedef vertex_t                *vertices_t;
typedef struct collision_contact collision_contact_t;
typedef struct constraint        constraint_t;
typedef struct composite         composite_t;
typedef struct detector          detector_t;
typedef struct physics_engine    physics_engine_t;
typedef struct physics_timing    physics_timing_t;

struct bounds {
	vec2 min, max;
};

struct vertex {
	real    x, y;
	count_t index;
	body_t *body;
	int     is_internal;
};

struct body_original_data {
	real restitution, friction, mass, inertia, density, inv_mass, inv_inertia;
};

struct body {
	uint64_t id;

	body_t   **parts;
	real       angle;
	vertices_t vertices;
	point_t    position;
	vec2       force;
	real       torque;
	vec2       position_impulse;
	vec2       constraint_impulse;
	real       constraint_impulse_angle;
	count_t    total_contacts;
	real       speed;
	real       angular_speed;
	vec2       velocity;
	real       angular_velocity;
	int        is_sensor;
	int        is_static;
	int        is_sleeping;
	real       motion;
	real       sleep_threshold;
	real       sleep_counter;
	real       restitution;

	real friction;
	real friction_air;
	real friction_static;

	uint64_t filter_category;
	uint64_t filter_mask;
	uint64_t filter_group;

	real     slop;
	real     time_scale;
	real     circle_radius;
	point_t  position_prev;
	real     angle_prev;
	body_t  *parent;
	axes_t   axes;
	real     density;
	real     area;
	real     mass;
	real     inertia;
	real     inv_mass, inv_inertia;
	bounds_t bound;

	struct body_original_data *original;
	struct body_original_data  original_data_;
};

struct constraint {
	uint64_t id;

	body_t *body_a, *body_b;
	vec2    point_a, point_b;
	real    angle_a, angle_b;
	real    length;
	real    stiffness;
	real    angular_stiffness;
	real    damping;
	real    len_ratio_;
};

struct collision_contact {
	vertex_t vertex;
	real     normal_impulse, tangent_impulse;
};

struct collision_pair {
	char    id[MAX_ID_LEN];
	body_t *body_a, *body_b;

	collision_t         *collision;
	collision_contact_t *contacts;
	collision_contact_t *active_contacts;

	real separation;
	int  is_active;
	int  confirmed_active;
	int  is_sensor;
	real time_created;
	real time_updated;
	real inv_mass;
	real friction;
	real friction_static;
	real restitution;
	real slop;
};

struct collision_pairs {
	avl_hash_map_t     table;
	collision_pair_t **list, **collision_start, **collision_active, **collision_end;
};

struct collision {
	collision_pair_t *pair;

	int      collided;
	body_t  *body_a, *body_b;
	body_t  *parent_a, *parent_b;
	real     depth;
	vec2     normal;
	vec2     tangent;
	vec2     penetration;
	vertex_t supports[5];
	int      n_supports;

	uint32_t ref_count_;
};

struct composite_cache {
	constraint_t **all_constraints;
	body_t       **all_bodies;
	composite_t  **all_composites;
};

struct composite {
	uint64_t       id;
	composite_t   *parent;
	int            is_modified;
	body_t       **bodies;
	constraint_t **constraints;
	composite_t  **composites;

	struct composite_cache cache_;
};

struct detector {
	collision_t      **collisions;
	collision_pairs_t *pairs;
	body_t	       **bodies;
};

struct physics_timing {
	real time_stamp;
	real time_scale;
	real last_delta;
	real last_elapsed;
};

struct physics_engine {
	composite_t       *world;
	collision_pairs_t *pairs;
	detector_t        *detector;

	count_t position_iterations;
	count_t velocity_iterations;
	count_t constraint_iterations;
	int     enable_sleeping;
	vec2    gravity;
	real    gravity_scale;

	real    fps;
	real    correction;
	real    delta, delta_min, delta_max;
	count_t delta_sample_size;
	real    counter_time_stamp;
	count_t frame_counter;
	real    time_prev;
	real    time_scale_prev;
	int     is_fixed;
	int     enabled;

	physics_timing_t timing;
};

uint64_t common_next_id();
uint64_t common_next_group(int is_non_colliding);
uint64_t common_next_category();
body_t  *common_rectangle(vec2 pos, real length, real width);
body_t  *common_polygon(vec2 pos, count_t n, ...);

vertices_t vertices_alloc();
vertices_t vertices_init(vertices_t vertices, points_t points, count_t n, body_t *body);
vertices_t vertices_create(points_t points, count_t n, body_t *body);
void       vertices_done(vertices_t vertices);
void       vertices_destroy(vertices_t vertices);
void       vertices_update_aabb(vertices_t vertices, aabb_t *aabb, vec2 velocity, int update_velocity);

void axes_set_from_vertices(vertices_t vertices, size_t n, axes_t axes, size_t *n_axes);
void axes_rotate(axes_t axes, size_t n, real angle);

uint64_t body_max_id();
body_t  *body_alloc();
body_t  *body_init(body_t *body);
body_t  *body_create();
void     body_done(body_t *body);
void     body_destroy(body_t *body);
void     body_set_static(body_t *body, int is_static);
void     body_set_mass(body_t *body, real mass);
void     body_set_density(body_t *body, real density);
void     body_set_inertia(body_t *body, real inertia);
void     body_set_vertices(body_t *body, vertices_t vertices);
void     body_set_parts(body_t *body, body_t **parts, count_t n, int auto_hull);
void     body_set_center(body_t *body, point_t center, int relative);
void     body_set_position(body_t *body, point_t position);
void     body_set_angle(body_t *body, real angle);
void     body_set_velocity(body_t *body, vec2 velocity);
void     body_set_angular_velocity(body_t *body, real angular_velocity);
void     body_translate(body_t *body, vec2 translation);
void     body_rotate(body_t *body, real rotation, point_t point, int with_point);
void     body_scale(body_t *body, real scale_x, real scale_y, point_t point);
void     body_update(body_t *body, real deltaTime, real timeScale, real correction);
void     body_apply_force(body_t *body, point_t position, vec2 force);

collision_t *collision_alloc();
collision_t *collision_init(collision_t *collision, body_t *body_a, body_t *body_b);
collision_t *collision_create(body_t *body_a, body_t *body_b);
void         collision_done(collision_t *collision);
void         collision_destroy(collision_t *collision);
collision_t *collision_set_reference(void **ref_obj, collision_t *collision);
collision_t *collision_get_pair_record(body_t *body_a, body_t *body_b, collision_pairs_t *pairs);
collision_t *collision_sat(body_t *body_a, body_t *body_b, collision_pairs_t *pairs);
collision_t *collision_collides(body_t *body_a, body_t *body_b, collision_pairs_t *pairs);

void              collision_pair_update(collision_pair_t *pair, collision_t *collision, real time_stamp);
void              collision_pair_destroy(collision_pair_t *pair);
void              collision_pair_done(collision_pair_t *pair);
collision_pair_t *collision_pair_create(collision_t *collision, real time_stamp);
collision_pair_t *collision_pair_init(collision_pair_t *pair, collision_t *collision, real time_stamp);
collision_pair_t *collision_pair_alloc();
void              collision_pair_get_id(char *id_out, body_t *a, body_t *b);
void              collision_pair_set_active(collision_pair_t *pair, int is_active, real time_stamp);

collision_pairs_t *collision_pairs_alloc();
collision_pairs_t *collision_pairs_init(collision_pairs_t *pairs);
collision_pairs_t *collision_pairs_create();
void               collision_pairs_done(collision_pairs_t *pairs);
void               collision_pairs_destroy(collision_pairs_t *pairs);
void               collision_pairs_update(collision_pairs_t *pairs, collision_t **collisions, count_t n, real time_stamp);
void               collision_pairs_clear(collision_pairs_t *pairs);

composite_t   *composite_alloc();
composite_t   *composite_init(composite_t *composite);
composite_t   *composite_create();
void           composite_done(composite_t *composite);
void           composite_destroy(composite_t *composite);
void           composite_set_modified(composite_t *composite, int is_modified, int update_parents, int update_children);
void           composite_add_composite(composite_t *composite, composite_t *add);
void           composite_remove_composite_at(composite_t *composite, count_t position);
void           composite_remove_composite(composite_t *composite, composite_t *re, int deep);
void           composite_add_body(composite_t *composite, body_t *add);
void           composite_remove_body_at(composite_t *composite, count_t position);
void           composite_remove_body(composite_t *composite, body_t *re, int deep);
void           composite_remove_constraint(composite_t *composite, constraint_t *re, int deep);
void           composite_add_constraint(composite_t *composite, constraint_t *add);
void           composite_remove_constraint_at(composite_t *composite, count_t position);
body_t       **composite_all_bodies(composite_t *composite);
composite_t  **composite_all_composites(composite_t *composite);
constraint_t **composite_all_constraints(composite_t *composite);

detector_t   *detector_alloc();
detector_t   *detector_init(detector_t *detector);
detector_t   *detector_create();
void          detector_done(detector_t *detector);
void          detector_destroy(detector_t *detector);
void          detector_set_bodies(detector_t *detector, body_t **bodies, count_t n);
void          detector_clear(detector_t *detector);
collision_t **detector_collisions(detector_t *detector);

physics_engine_t *physics_engine_alloc();
physics_engine_t *physics_engine_init(physics_engine_t *engine);
physics_engine_t *physics_engine_create();
void              physics_engine_done(physics_engine_t *engine);
void              physics_engine_destroy(physics_engine_t *engine);
void              physics_engine_update(physics_engine_t *engine, real delta, real correction);
void              physics_engine_tick(physics_engine_t *engine);

void resolver_pre_solve_position(collision_pair_t **pairs, count_t n);
void resolver_solve_position(collision_pair_t **pairs, count_t n, real time_scale);
void resolver_post_solve_position(body_t **bodies, count_t n);
void resolver_pre_solve_velocity(collision_pair_t **pairs, count_t n);
void resolver_solve_velocity(collision_pair_t **pairs, count_t n, real time_scale);
void resolver_pre_solve_all_constraints(body_t **bodies, count_t n);
void resolver_solve_all_constraints(constraint_t **constraints, count_t n, real time_scale);
void resolver_post_solve_all_constraints(body_t **bodies, count_t n);

void sleeping_set(body_t *body, int is_sleeping);
void sleeping_update(body_t **bodies, count_t bodies_len, real time_scale);
void sleeping_after_collisions(collision_pair_t **pairs, count_t pairs_len, real time_scale);

constraint_t  constraint_(body_t *ba, body_t *bb, vec2 pa, vec2 pb, real len_ratio);
constraint_t *constraint_alloc();
constraint_t *constraint_init(constraint_t *constraint, constraint_t op);
constraint_t *constraint_create(constraint_t op);
void          constraint_done(constraint_t *constraint);
void          constraint_destroy(constraint_t *constraint);
void          constraint_solve(constraint_t *constraint, real time_scale);

void body_print_data(body_t *body, FILE *fp);

extern real     BODY_inertia_scale;
extern uint64_t BODY_next_colliding_group_id;
extern uint64_t BODY_next_non_colliding_group_id;
extern uint64_t BODY_next_category;

extern real RESOLVER_resting_thresh;
extern real RESOLVER_resting_thresh_tangent;
extern real RESOLVER_position_dampen;
extern real RESOLVER_position_warming;
extern real RESOLVER_friction_normal_multiplier;

extern real SLEEPING_motion_wake_threshold;
extern real SLEEPING_motion_sleep_threshold;
extern real SLEEPING_min_bias;

extern real CONSTRAINT_warming;
extern real CONSTRAINT_torque_dampen;
extern real CONSTRAINT_min_length;


#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* PHYSICS_H */
