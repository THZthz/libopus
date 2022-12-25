/**
 * @file bvh.c
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


#include "physics/algorithm/bvh.h"

#include <stdlib.h>

#include "data_structure/array.h"
#include "physics/physics.h"

static void bvh_tree_draw_internal(plutovg_t *pluto, bvh_node_t *node, int flag)
{
	if (node) {
		if (!node->is_leaf) bvh_tree_draw_internal(pluto, node->left, 0);
		plutovg_rect(pluto, node->aabb.min_x, node->aabb.min_y, node->aabb.max_x - node->aabb.min_x, node->aabb.max_y - node->aabb.min_y);
		plutovg_stroke(pluto);
		if (!node->is_leaf) bvh_tree_draw_internal(pluto, node->right, 1);
	}
}

/* This function assumes that the type of "body" is "body_t *" defined in "physics.h" */
static void bvh_get_body_aabb_default_(aabb_t *aabb, void *b, int type)
{
	body_t *body = b;
	aabb->max_x  = body->bound.max.x;
	aabb->max_y  = body->bound.max.y;
	aabb->min_x  = body->bound.min.x;
	aabb->min_y  = body->bound.min.y;
}

static void bvh_get_body_aabb_(bvh_tree_t *tree, aabb_t *aabb, void *b, int type)
{
	if (tree->get_body_aabb_) tree->get_body_aabb_(tree, aabb, b, type);
	else
		bvh_get_body_aabb_default_(aabb, b, type);
}

bvh_tree_t *bvh_tree_create()
{
	bvh_tree_t *tree = (bvh_tree_t *) malloc(sizeof(bvh_tree_t));
	bvh_tree_init(tree);

	return tree;
}

void bvh_tree_init(bvh_tree_t *bvh)
{
	bvh->hierarchy      = NULL;
	bvh->get_body_aabb_ = NULL;
	bvh->leaf_count     = 0;

	array_create(bvh->bodies, sizeof(bvh_body_t));
}

void bvh_tree_destroy_hierarchy(bvh_node_t *node)
{
	if (node) {
		if (!node->is_leaf) {
			bvh_tree_destroy_hierarchy(node->left);
			bvh_tree_destroy_hierarchy(node->right);
		}
		free(node);
	}
}

void bvh_tree_destroy(bvh_tree_t *bvh)
{
	bvh_tree_destroy_hierarchy(bvh->hierarchy);
	bvh->hierarchy = NULL;

	array_destroy(bvh->bodies);
	free(bvh);
}

void bvh_node_init(bvh_node_t *node)
{
	node->is_leaf = 0;
	node->left    = NULL;
	node->right   = NULL;
	node->parent  = NULL;
	node->height  = 1;
}

bvh_node_t *bvh_node_create()
{
	bvh_node_t *node = (bvh_node_t *) malloc(sizeof(bvh_node_t));
	bvh_node_init(node);

	return node;
}

void bvh_node_destroy(bvh_node_t *node)
{
	free(node);
}

/**
 * build from scratch using surface area heuristic
 * @param bvh
 */
void bvh_tree_build_SAH(bvh_tree_t *bvh)
{
	/* TODO: finish this (top-down method) */
}

void bvh_tree_draw(plutovg_t *pluto, bvh_tree_t *bvh)
{
	if (bvh->hierarchy) {
		plutovg_set_source_rgba(pluto, 0.3f, 0.4f, 0.5f, 0.3f);
		plutovg_set_line_width(pluto, 2.f);
		bvh_tree_draw_internal(pluto, bvh->hierarchy, 1);
	}
}

/**
 * perform a left or right rotation if node A is imbalanced,
 * @param bvh
 * @param index_node
 * @return  the new node whose location is the index_node you input
 */
bvh_node_t *bvh_tree_balance_tree(bvh_tree_t *bvh, bvh_node_t *index_node)
{
	int         balance;
	bvh_node_t *A, *B, *C;

	ASSERT(index_node != NULL);

	A = index_node;
	if (A->is_leaf || A->height < 2) {
		return A;
	}

	B = A->left;
	C = A->right;

	balance = C->height - B->height;

	/* Rotate C up */
	if (balance > 1) {
		bvh_node_t *F = C->left;
		bvh_node_t *G = C->right;

		/* Swap A and C */
		C->left   = A;
		C->parent = A->parent;
		A->parent = C;

		/* A's old parent should point to C */
		if (C->parent != NULL) {
			if (C->parent->left == A) {
				C->parent->left = C;
			} else {
				ASSERT(C->parent->right == A);
				C->parent->right = C;
			}
		} else {
			bvh->hierarchy = C;
		}

		/* Rotate */
		if (F->height > G->height) {
			C->right  = F;
			A->right  = G;
			G->parent = A;
			aabb_combine(&(B->aabb), &(G->aabb), &(A->aabb));
			aabb_combine(&(A->aabb), &(F->aabb), &(C->aabb));

			A->height = 1 + r_max(B->height, G->height);
			C->height = 1 + r_max(A->height, F->height);
		} else {
			C->right  = G;
			A->right  = F;
			F->parent = A;
			aabb_combine(&(B->aabb), &(F->aabb), &(A->aabb));
			aabb_combine(&(A->aabb), &(G->aabb), &(C->aabb));

			A->height = 1 + r_max(B->height, F->height);
			C->height = 1 + r_max(A->height, G->height);
		}

		return C;
	}

	/* Rotate B up */
	if (balance < -1) {
		bvh_node_t *D = B->left;
		bvh_node_t *E = B->right;

		/* Swap A and B */
		B->left   = A;
		B->parent = A->parent;
		A->parent = B;

		/* A's old parent should point to B */
		if (B->parent != NULL) {
			if (B->parent->left == A) {
				B->parent->left = B;
			} else {
				ASSERT(B->parent->right == A);
				B->parent->right = B;
			}
		} else {
			bvh->hierarchy = B;
		}

		/* Rotate */
		if (D->height > E->height) {
			B->right  = D;
			A->left   = E;
			E->parent = A;
			aabb_combine(&(C->aabb), &(E->aabb), &(A->aabb));
			aabb_combine(&(A->aabb), &(D->aabb), &(B->aabb));

			A->height = 1 + r_max(C->height, E->height);
			B->height = 1 + r_max(A->height, D->height);
		} else {
			B->right  = E;
			A->left   = D;
			D->parent = A;
			aabb_combine(&(C->aabb), &(D->aabb), &(A->aabb));
			aabb_combine(&(A->aabb), &(E->aabb), &(B->aabb));

			A->height = 1 + r_max(C->height, D->height);
			B->height = 1 + r_max(A->height, E->height);
		}

		return B;
	}

	return A;
}

/**
 * Dynamically insert a leaf into the BVH tree, will re-balance the tree for better traverse efficiency.
 * This algorithm is originally from Box2D's dynamic tree by Erin Catto, I am really benefited a lot from this.
 * @param bvh the BVH tree (binary and AVL-conformed)
 * @param body the body you want to insert into the tree
 * @param type the type of the body (or shape if you want to say), you can leave whatever you want
 * @return the leaf contains the body you insert
 */
bvh_node_t *bvh_tree_insert_leaf(bvh_tree_t *bvh, void *body, int type)
{
	bvh_body_t *container;
	bvh_node_t *leaf, *index_node;
	aabb_t     *leaf_aabb, *combined_aabb;
	bvh_node_t *best_sibling, *new_parent, *old_parent, *refit_current;

	/* construct BVH specialized body container for the body you insert */
	container       = (bvh_body_t *) malloc(sizeof(bvh_body_t));
	container->body = body;
	container->type = type;
	bvh_get_body_aabb_(bvh, &(container->aabb), body, type); /* update its aabb bounding box */
	leaf          = bvh_node_create();
	leaf->is_leaf = 1;
	leaf->body    = container;
	aabb_copy(&(leaf->aabb), &(container->aabb)); /* copy aabb of the container directly since there is only one body */

	/* the bvh tree is empty, directly insert the body */
	if (bvh->hierarchy == NULL) {
		bvh->leaf_count++;
		bvh->hierarchy = leaf; /* update new hierarchy */
		leaf->parent   = NULL;
		leaf->height   = 1;
		return leaf;
	}

	/* stage 1: find the best sibling */
	index_node    = bvh->hierarchy; /* current node we are searching */
	leaf_aabb     = &(leaf->aabb);
	combined_aabb = aabb_create(0.0, 0.0, 0.0, 0.0);
	while (!index_node->is_leaf) {
		aabb_t aabb;
		double cost1; /* cost of descending into left */
		double cost2; /* cost of descending into right */
		double area, combined_area, cost, inheritance_cost, perimeter_aabb;

		area = aabb_get_perimeter(&(index_node->aabb));
		aabb_combine(&(index_node->aabb), leaf_aabb, combined_aabb);
		combined_area = aabb_get_perimeter(combined_aabb);

		/* cost of creating a new parent for this node and the new leaf */
		cost = 2.0 * combined_area;

		/* minimum cost of pushing the leaf further down the tree */
		inheritance_cost = 2.0 * (combined_area - area);

		aabb_combine(leaf_aabb, &(index_node->left->aabb), &aabb);
		if (index_node->left->is_leaf) {
			cost1 = aabb_get_perimeter(&aabb) + inheritance_cost;
		} else {
			double old_area, new_area;

			old_area = aabb_get_perimeter(&(index_node->left->aabb));
			new_area = aabb_get_perimeter(&aabb);
			cost1    = new_area - old_area + inheritance_cost;
		}

		aabb_combine(leaf_aabb, &(index_node->right->aabb), &aabb);
		if (index_node->right->is_leaf) {
			cost2 = aabb_get_perimeter(&aabb) + inheritance_cost;
		} else {
			double old_area, new_area;

			old_area = aabb_get_perimeter(&(index_node->right->aabb));
			new_area = aabb_get_perimeter(&aabb);
			cost2    = new_area - old_area + inheritance_cost;
		}

		/* descend according to the minimum cost. */
		if (cost < cost1 && cost < cost2) {
			break;
		}

		/* descend */
		if (cost1 < cost2) {
			index_node = index_node->left;
		} else {
			index_node = index_node->right;
		}
	}
	aabb_destroy(combined_aabb);

	/* stage 2: create new parent node */
	best_sibling       = index_node; /* the best node to insert the body */
	new_parent         = bvh_node_create();
	old_parent         = index_node->parent;
	new_parent->parent = old_parent;
	new_parent->height = best_sibling->height + 1;
	aabb_combine(leaf_aabb, &best_sibling->aabb, &new_parent->aabb);
	if (old_parent == NULL) { /* the best_sibling is the root */
		bvh->hierarchy       = new_parent;
		new_parent->left     = best_sibling;
		new_parent->right    = leaf;
		leaf->parent         = new_parent;
		best_sibling->parent = new_parent;
	} else { /* the best_sibling is not the root */
		if (old_parent->left == best_sibling) {
			old_parent->left = new_parent;
		} else {
			old_parent->right = new_parent;
		}

		new_parent->left     = best_sibling;
		new_parent->right    = leaf;
		new_parent->parent   = old_parent;
		best_sibling->parent = new_parent;
		leaf->parent         = new_parent;
	}

	/* stage 3: refit all the parents to ensure parent node enclose children */

	/* if we choose best_node children as best sibling, start refitting from best_node, otherwise from its parent */
	refit_current = leaf->parent;
	while (refit_current != NULL) {
		bvh_node_t *left, *right;

		refit_current = bvh_tree_balance_tree(bvh, refit_current);

		left  = refit_current->left;
		right = refit_current->right;

		if (left == NULL || right == NULL) {
			ERROR_("Insert leaf failed, the tree structure is invalid. (function: bvh_tree_insert_leaf)\n");
			exit(-1);
		}

		aabb_combine(&(left->aabb), &(right->aabb), &(refit_current->aabb));
		refit_current->height = 1 + r_max(left->height, right->height);


		refit_current = refit_current->parent;
	}

	return leaf;
}

/**
 *
 * @param bvh
 * @param leaf
 */
void bvh_tree_remove_leaf(bvh_tree_t *bvh, bvh_node_t *leaf)
{
	bvh_node_t *parent, *grandParent, *sibling;
	if (leaf == bvh->hierarchy) {
		bvh->hierarchy = NULL;
		return;
	}

	parent      = leaf->parent;
	grandParent = parent->parent;
	if (parent->left == leaf) {
		sibling = parent->right;
	} else {
		sibling = parent->left;
	}

	if (grandParent != NULL) {
		bvh_node_t *index;

		/* Destroy parent and connect sibling to grandParent. */
		if (grandParent->left == parent) {
			grandParent->left = sibling;
		} else {
			grandParent->right = sibling;
		}
		sibling->parent = grandParent;
		bvh_node_destroy(parent);

		/* Adjust ancestor bounds. */
		index = grandParent;
		while (index != NULL) {
			bvh_node_t *left, *right;

			index = bvh_tree_balance_tree(bvh, index);

			left  = index->left;
			right = index->right;

			aabb_combine(&(left->aabb), &(right->aabb), &(index->aabb));
			index->height = 1 + r_max(left->height, right->height);

			index = index->parent;
		}
	} else {
		bvh->hierarchy  = sibling;
		sibling->parent = NULL;
		bvh_node_destroy(parent);
	}
}

/**
 * find the leaf with body container contains the body you input, if the body is not in the tree, return NULL
 * @param bvh
 * @param body
 * @param type
 * @return
 */
bvh_node_t *bvh_tree_find_leaf(bvh_tree_t *bvh, void *body, int type)
{
	aabb_t       aabb;
	bvh_node_t **stack;
	bvh_get_body_aabb_(bvh, &aabb, body, type);
	array_create(stack, 10);
	array_push(stack, &(bvh->hierarchy));

	while (array_len(stack) > 0) {
		bvh_node_t *current = stack[array_len(stack) - 1];
		array_pop(stack);

		if (current->is_leaf) {
			if (current->body->body == body) {
				return current;
			}
		} else {
			if (current->left && aabb_is_overlap_with(&aabb, &(current->left->aabb))) {
				array_push(stack, &(current->left));
			}
			if (current->right && aabb_is_overlap_with(&aabb, &(current->right->aabb))) {
				array_push(stack, &(current->right));
			}
		}
	}

	array_destroy(stack);
	return NULL;
}

/**
 * @brief Get an array of bodies (in container 'struct Collisions_BVH_Body *") which might possibly collide with the body you passed into.
 * 		This might cost much CPU, so do not use this if you have to deal with hordes of bodies at a time
 * @param bvh BVH tree
 * @param leaf the leaf containing the body you want to check, you can use 'magnum_bvh_find_leaf' to find the leaf in the BVH tree
 * @param result array (in magnum_array2.h)
 */
bvh_body_t **bvh_tree_get_collision_potentials(bvh_tree_t *bvh, bvh_node_t *leaf, bvh_body_t **result)
{
	bvh_node_t **stack;
	array_create(stack, 10);
	array_push(stack, &(bvh->hierarchy));

	array_clear(result);
	while (array_len(stack) > 0) {
		bvh_node_t *current = stack[array_len(stack) - 1];
		if (current == NULL) break;
		array_pop(stack);

		if (current->is_leaf) {
			if (current->body != leaf->body)
				array_push(result, &(current->body));
		} else {
			if (current->left && aabb_is_overlap_with(&(leaf->aabb), &(current->left->aabb)))
				array_push(stack, &(current->left));
			if (current->right && aabb_is_overlap_with(&(leaf->aabb), &(current->right->aabb)))
				array_push(stack, &(current->right));
		}
	}

	array_destroy(stack);

	return result;
}

static void bvh_check_two_branches(bvh_node_t *n1, bvh_node_t *n2, void (*callback)(bvh_body_t *, bvh_body_t *, void *), void *data)
{
	if (n1 == NULL || n2 == NULL) {
		if (n1 == NULL) bvh_for_each_potential_pair(n2, callback, data);
		if (n2 == NULL) bvh_for_each_potential_pair(n1, callback, data);
		return;
	}

	if (aabb_is_overlap_with(&(n1->aabb), &(n2->aabb))) {
		if (n1->is_leaf) {
			if (n2->is_leaf) {
				callback(n1->body, n2->body, data);
			} else {
				bvh_check_two_branches(n2->left, n1, callback, data);
				bvh_check_two_branches(n2->right, n1, callback, data);
			}
		} else {
			if (n2->is_leaf) {
				bvh_check_two_branches(n1->left, n2, callback, data);
				bvh_check_two_branches(n1->right, n2, callback, data);
			} else {
				bvh_check_two_branches(n1->left, n2->left, callback, data);
				bvh_check_two_branches(n1->left, n2->right, callback, data);
				bvh_check_two_branches(n1->right, n2->left, callback, data);
				bvh_check_two_branches(n1->right, n2->right, callback, data);
			}
		}
	}
	bvh_for_each_potential_pair(n1, callback, data);
	bvh_for_each_potential_pair(n2, callback, data);
}

void bvh_for_each_potential_pair(bvh_node_t *bvh_node, void (*callback)(bvh_body_t *, bvh_body_t *, void *), void *data)
{
	if (bvh_node == NULL) return;

	if (!bvh_node->is_leaf) {
		bvh_check_two_branches(bvh_node->left, bvh_node->right, callback, data);
	}
}
