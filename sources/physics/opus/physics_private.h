/**
 * @file physics_private.h
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2023/3/3
 *
 * @example
 *
 * @development_log
 *
 */
#ifndef PHYSICS_PRIVATE_H
#define PHYSICS_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "physics/opus/physics.h"


typedef struct opus_contact  opus_contact;
typedef struct opus_contacts opus_contacts;
typedef struct opus_bvh      opus_bvh;
typedef struct opus_bvh_leaf opus_bvh_leaf;

typedef void (*opus_sap_cb)(opus_body *A, opus_body *B, opus_mat2d ta, opus_mat2d tb, void *data);

/**
 * @brief result pass to collision detection algorithm, like SAT or GJK
 */
struct opus_overlap_result {
	/* reference edge master */
	opus_shape *A;
	/* incident edge master */
	opus_shape *B;

	opus_mat2d transform_a, transform_b;
	int        is_overlap;
	opus_real  separation; /* penetration depth, positive value */

	/**
	 * @brief a vector points to B from A, you can push B in this direction
	 * 		with the length of "penetration" depth, then the two overlapping shape can be separated.
	 */
	opus_vec2 normal;
};

/**
 * @brief result of clip algorithm
 */
struct opus_clip_result {
	opus_shape *A, *B;

	size_t    n_support;
	opus_vec2 supports[2][2];
};

struct opus_bvh {
	int leaf_count;

	opus_bvh_leaf *hierarchy;
	opus_body     *bodies; /* TODO all the bodies stored in the tree are represented as an index */
};

struct opus_bvh_leaf {
	opus_aabb      aabb;    /* TODO: aabbs in tree node are fat aabb */
	int            is_leaf; /* indicate whether this node is interior */
	opus_bvh_leaf *parent;
	int            height;
	opus_bvh_leaf *left, *right;
};

struct opus_contacts {
	char id[32];

	opus_body     *A, *B;
	opus_contact **contacts;
};

struct opus_contact {
	int        is_active;
	opus_body *A, *B;
	opus_vec2  pa, pb;
	opus_real  effective_mass_normal;
	opus_real  effective_mass_tangent;
	opus_real  normal_impulse;
	opus_real  tangent_impulse;
	opus_vec2  ra;
	opus_vec2  rb;
	opus_vec2  normal;
	opus_vec2  tangent;
	opus_real  depth;
	opus_vec2  position_corrective_impulse;
	opus_vec2  velocity_bias;
};

/**
 * @brief max shape struct size, remember to call "opus_shape_get_max_struct_size_" to initialize
 */
extern unsigned int OPUS_physics_shape_max_size;

size_t opus_get_physics_id();
void   opus_recycle_physics_id(size_t id);

void opus_shape_get_max_struct_size_();

opus_vec2 opus_shape_polygon_get_support(opus_shape *shape, opus_mat2d transform, opus_vec2 dir, size_t *index);
opus_real opus_shape_polygon_get_inertia(opus_shape *shape, opus_real mass);
void      opus_shape_polygon_update_bound(opus_shape *shape, opus_real rotation, opus_vec2 position);
opus_vec2 opus_shape_circle_get_support(opus_shape *shape, opus_mat2d transform, opus_vec2 dir, size_t *index);
opus_real opus_shape_circle_get_inertia(opus_shape *shape, opus_real mass);
void      opus_shape_circle_update_bound(opus_shape *shape, opus_real rotation, opus_vec2 position);

void opus_body_step_position(opus_body *body, opus_real dt);
void opus_body_step_velocity(opus_body *body, opus_vec2 gravity, int enable_damping,
                             opus_real linear_velocity_damping,
                             opus_real angular_velocity_damping, opus_real dt);

opus_contacts *opus_contacts_create(opus_body *A, opus_body *B);
void           opus_contacts_destroy(opus_contacts *contacts);
opus_contact  *opus_contact_create(opus_body *A, opus_body *B, opus_vec2 pa, opus_vec2 pb, opus_vec2 normal, opus_real depth);
void           opus_contact_destroy(opus_contact *contact);
char          *opus_contacts_id(opus_body *A, opus_body *B);

opus_bvh      *opus_bvh_create();
void           opus_bvh_destroy(opus_bvh *bvh);
void           opus_bvh_build_SAH(opus_bvh *bvh);
void           opus_bvh_render(plutovg_t *pluto, opus_bvh *bvh);
opus_bvh_leaf *opus_bvh_insert(opus_bvh *bvh, opus_body *body);
void           opus_bvh_remove(opus_bvh *bvh, opus_bvh_leaf *leaf);
opus_bvh_leaf *opus_bvh_find(opus_bvh *bvh, opus_body *body);
opus_body    **opus_bvh_potentials(opus_bvh *bvh, opus_bvh_leaf *leaf, opus_body **result);
void           opus_bvh_for_each_potential(opus_bvh_leaf *bvh_node, void (*callback)(opus_body *, opus_body *, void *), void *data);

opus_overlap_result opus_SAT(opus_shape *A, opus_shape *B, opus_mat2d transform_a, opus_mat2d transform_b);
opus_clip_result    opus_VCLIP(opus_overlap_result overlap);
void                opus_SAP(opus_body **bodies, size_t n, opus_sap_cb callback, void *data);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* PHYSICS_PRIVATE_H */
