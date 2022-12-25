/**
 * @file bvh.h
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/10/17
 *
 * @example
 *
 * @development_log
 *
 */
#ifndef BVH_H
#define BVH_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "math/geometry.h"
#include "render/pluto/plutovg.h"


typedef struct bvh_tree bvh_tree_t;
typedef struct bvh_node bvh_node_t;
typedef struct bvh_body bvh_body_t;

typedef void (*bvh_get_body_aabb_cb)(bvh_tree_t *tree, aabb_t *aabb, void *body, int type);

/* a body structure used in BVH tree (property bodies) */
struct bvh_body {
	void  *body;
	aabb_t aabb;
	int    type;
};

struct bvh_tree {
	bvh_node_t *hierarchy;
	int         leaf_count;
	bvh_body_t *bodies; /* TODO all the bodies stored in the tree are represented as an index */

	bvh_get_body_aabb_cb get_body_aabb_;
};

struct bvh_node {
	aabb_t      aabb;    /* TODO: aabbs in tree node are fat aabb */
	int   is_leaf; /* indicate whether this node is interior */
	bvh_node_t *parent;
	int         height;
	bvh_node_t *left;
	union
	{
		bvh_node_t *right;
		bvh_body_t *body;
	};
};

void         bvh_tree_init(bvh_tree_t *bvh);
bvh_tree_t  *bvh_tree_create();
void         bvh_tree_destroy(bvh_tree_t *bvh);
void         bvh_node_init(bvh_node_t *node);
bvh_node_t  *bvh_node_create();
void         bvh_tree_destroy_hierarchy(bvh_node_t *node);
void         bvh_node_destroy(bvh_node_t *node);
void         bvh_tree_build_SAH(bvh_tree_t *bvh);
void         bvh_tree_draw(plutovg_t *pluto, bvh_tree_t *bvh);
bvh_node_t  *bvh_tree_balance_tree(bvh_tree_t *bvh, bvh_node_t *index_node);
bvh_node_t  *bvh_tree_insert_leaf(bvh_tree_t *bvh, void *body, int type);
void         bvh_tree_remove_leaf(bvh_tree_t *bvh, bvh_node_t *leaf);
bvh_node_t  *bvh_tree_find_leaf(bvh_tree_t *bvh, void *body, int type);
bvh_body_t **bvh_tree_get_collision_potentials(bvh_tree_t *bvh, bvh_node_t *leaf, bvh_body_t **result);
void         bvh_for_each_potential_pair(bvh_node_t *bvh_node, void (*callback)(bvh_body_t *, bvh_body_t *, void *), void *data);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* BVH_H */
