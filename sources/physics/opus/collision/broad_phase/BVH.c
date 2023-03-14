/**
 * @file BVH.c
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

#include "data_structure/array.h"
#include "physics/opus/physics.h"
#include "physics/opus/physics_private.h"

static void opus_bvh_draw_internal(plutovg_t *pluto, opus_bvh_leaf *node, int flag)
{
	if (node) {
		if (!node->is_leaf) opus_bvh_draw_internal(pluto, node->left, 0);
		plutovg_rect(pluto, node->aabb.min.x, node->aabb.min.y, node->aabb.max.x - node->aabb.min.x, node->aabb.max.y - node->aabb.min.y);
		plutovg_stroke(pluto);
		if (!node->is_leaf) opus_bvh_draw_internal(pluto, node->right, 1);
	}
}

/* This function assumes that the type of "body" is "body_t *" defined in "physics.h" */
static void bvh_get_body_aabb_default_(opus_aabb *aabb, void *b, int type)
{
	opus_body *body;
	body  = b;
	*aabb = body->shape->bound;
}

static void opus_bvh_init(opus_bvh *bvh)
{
	bvh->hierarchy  = NULL;
	bvh->leaf_count = 0;

	opus_arr_create(bvh->bodies, sizeof(opus_body));
}

opus_bvh *opus_bvh_create(void)
{
	opus_bvh *tree = (opus_bvh *) malloc(sizeof(opus_bvh));
	opus_bvh_init(tree);

	return tree;
}

static void opus_bvh_destroy_hierarchy(opus_bvh_leaf *node)
{
	if (node) {
		if (!node->is_leaf) {
			opus_bvh_destroy_hierarchy(node->left);
			opus_bvh_destroy_hierarchy(node->right);
		}
		free(node);
	}
}

void opus_bvh_destroy(opus_bvh *bvh)
{
	opus_bvh_destroy_hierarchy(bvh->hierarchy);
	bvh->hierarchy = NULL;

	opus_arr_destroy(bvh->bodies);
	free(bvh);
}

static void bvh_node_init(opus_bvh_leaf *node)
{
	node->is_leaf = 0;
	node->left    = NULL;
	node->right   = NULL;
	node->parent  = NULL;
	node->height  = 1;
}

static opus_bvh_leaf *bvh_node_create(void)
{
	opus_bvh_leaf *node = (opus_bvh_leaf *) malloc(sizeof(opus_bvh_leaf));
	bvh_node_init(node);

	return node;
}

static void bvh_node_destroy(opus_bvh_leaf *node)
{
	free(node);
}

/**
 * build from scratch using surface area heuristic
 * @param bvh
 */
void opus_bvh_build_SAH(opus_bvh *bvh)
{
	/* TODO: finish this (top-down method) */
}

void opus_bvh_render(plutovg_t *pluto, opus_bvh *bvh)
{
	if (bvh->hierarchy) {
		plutovg_set_source_rgba(pluto, 0.3f, 0.4f, 0.5f, 0.3f);
		plutovg_set_line_width(pluto, 2.f);
		opus_bvh_draw_internal(pluto, bvh->hierarchy, 1);
	}
}

/**
 * perform a left or right rotation if node A is imbalanced,
 * @param bvh
 * @param index_node
 * @return  the new node whose location is the index_node you input
 */
static opus_bvh_leaf *opus_bvh_balance_tree(opus_bvh *bvh, opus_bvh_leaf *index_node)
{
	int              balance;
	opus_bvh_leaf   *A, *B, *C;

	OPUS_ASSERT(index_node != NULL);

	A = index_node;
	if (A->is_leaf || A->height < 2) {
		return A;
	}

	B = A->left;
	C = A->right;

	balance = C->height - B->height;

	/* Rotate C up */
	if (balance > 1) {
		opus_bvh_leaf *F = C->left;
		opus_bvh_leaf *G = C->right;

		/* Swap A and C */
		C->left   = A;
		C->parent = A->parent;
		A->parent = C;

		/* A's old parent should point to C */
		if (C->parent != NULL) {
			if (C->parent->left == A) {
				C->parent->left = C;
			} else {
				OPUS_ASSERT(C->parent->right == A);
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
			opus_aabb_combine(&(B->aabb), &(G->aabb), &(A->aabb));
			opus_aabb_combine(&(A->aabb), &(F->aabb), &(C->aabb));

			A->height = 1 + opus_max(B->height, G->height);
			C->height = 1 + opus_max(A->height, F->height);
		} else {
			C->right  = G;
			A->right  = F;
			F->parent = A;
			opus_aabb_combine(&(B->aabb), &(F->aabb), &(A->aabb));
			opus_aabb_combine(&(A->aabb), &(G->aabb), &(C->aabb));

			A->height = 1 + opus_max(B->height, F->height);
			C->height = 1 + opus_max(A->height, G->height);
		}

		return C;
	}

	/* Rotate B up */
	if (balance < -1) {
		opus_bvh_leaf *D = B->left;
		opus_bvh_leaf *E = B->right;

		/* Swap A and B */
		B->left   = A;
		B->parent = A->parent;
		A->parent = B;

		/* A's old parent should point to B */
		if (B->parent != NULL) {
			if (B->parent->left == A) {
				B->parent->left = B;
			} else {
				OPUS_ASSERT(B->parent->right == A);
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
			opus_aabb_combine(&(C->aabb), &(E->aabb), &(A->aabb));
			opus_aabb_combine(&(A->aabb), &(D->aabb), &(B->aabb));

			A->height = 1 + opus_max(C->height, E->height);
			B->height = 1 + opus_max(A->height, D->height);
		} else {
			B->right  = E;
			A->left   = D;
			D->parent = A;
			opus_aabb_combine(&(C->aabb), &(D->aabb), &(A->aabb));
			opus_aabb_combine(&(A->aabb), &(E->aabb), &(B->aabb));

			A->height = 1 + opus_max(C->height, D->height);
			B->height = 1 + opus_max(A->height, E->height);
		}

		return B;
	}

	return A;
}

/**
 * Dynamically insert a leaf into the BVH tree, will re-balance the tree for better destroy_leaves_ efficiency.
 * This algorithm is originally from Box2D's dynamic tree by Erin Catto, I am really benefited a lot from this.
 * @param bvh the BVH tree (binary and AVL-conformed)
 * @param body the body you want to insert into the tree
 * @param type the type of the body (or shape if you want to say), you can leave whatever you want
 * @return the leaf contains the body you insert
 */
opus_bvh_leaf *opus_bvh_insert(opus_bvh *bvh, opus_body *body)
{
	opus_bvh_leaf   *leaf, *index_node;
	opus_aabb       *leaf_aabb, *combined_aabb;
	opus_bvh_leaf   *best_sibling, *new_parent, *old_parent, *refit_current;

	leaf           = bvh_node_create();
	leaf->is_leaf  = 1;
	leaf->right    = (void *) body;
	opus_aabb_copy(&(leaf->aabb), &(body->shape->bound)); /* copy aabb of the container directly since there is only one body */

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
	combined_aabb = opus_aabb_create(0.0, 0.0, 0.0, 0.0);
	while (!index_node->is_leaf) {
		opus_aabb aabb;
		double cost1; /* cost of descending into left */
		double cost2; /* cost of descending into right */
		double area, combined_area, cost, inheritance_cost, perimeter_aabb;

		area = opus_aabb_perimeter(&(index_node->aabb));
		opus_aabb_combine(&(index_node->aabb), leaf_aabb, combined_aabb);
		combined_area = opus_aabb_perimeter(combined_aabb);

		/* cost of creating a new parent for this node and the new leaf */
		cost = 2.0 * combined_area;

		/* minimum cost of pushing the leaf further down the tree */
		inheritance_cost = 2.0 * (combined_area - area);

		opus_aabb_combine(leaf_aabb, &(index_node->left->aabb), &aabb);
		if (index_node->left->is_leaf) {
			cost1 = opus_aabb_perimeter(&aabb) + inheritance_cost;
		} else {
			double old_area, new_area;

			old_area = opus_aabb_perimeter(&(index_node->left->aabb));
			new_area = opus_aabb_perimeter(&aabb);
			cost1    = new_area - old_area + inheritance_cost;
		}

		opus_aabb_combine(leaf_aabb, &(index_node->right->aabb), &aabb);
		if (index_node->right->is_leaf) {
			cost2 = opus_aabb_perimeter(&aabb) + inheritance_cost;
		} else {
			double old_area, new_area;

			old_area = opus_aabb_perimeter(&(index_node->right->aabb));
			new_area = opus_aabb_perimeter(&aabb);
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
	opus_aabb_destroy(combined_aabb);

	/* stage 2: create new parent node */
	best_sibling       = index_node; /* the best node to insert the body */
	new_parent         = bvh_node_create();
	old_parent         = index_node->parent;
	new_parent->parent = old_parent;
	new_parent->height = best_sibling->height + 1;
	opus_aabb_combine(leaf_aabb, &best_sibling->aabb, &new_parent->aabb);
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
		opus_bvh_leaf *left, *right;

		refit_current = opus_bvh_balance_tree(bvh, refit_current);

		left  = refit_current->left;
		right = refit_current->right;

		if (left == NULL || right == NULL) {
			OPUS_ERROR("Insert leaf failed, the tree structure is invalid. (function: opus_bvh_insert)\n");
			exit(-1);
		}

		opus_aabb_combine(&(left->aabb), &(right->aabb), &(refit_current->aabb));
		refit_current->height = 1 + opus_max(left->height, right->height);


		refit_current = refit_current->parent;
	}

	return leaf;
}

/**
 *
 * @param bvh
 * @param leaf
 */
void opus_bvh_remove(opus_bvh *bvh, opus_bvh_leaf *leaf)
{
	opus_bvh_leaf *parent, *grandParent, *sibling;
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
		opus_bvh_leaf *index;

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
			opus_bvh_leaf *left, *right;

			index = opus_bvh_balance_tree(bvh, index);

			left  = index->left;
			right = index->right;

			opus_aabb_combine(&(left->aabb), &(right->aabb), &(index->aabb));
			index->height = 1 + opus_max(left->height, right->height);

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
opus_bvh_leaf *opus_bvh_find(opus_bvh *bvh, opus_body *body)
{
	opus_aabb         aabb;
	opus_bvh_leaf   **stack;
	aabb = body->shape->bound;
	opus_arr_create(stack, 10);
	opus_arr_push(stack, &(bvh->hierarchy));

	while (opus_arr_len(stack) > 0) {
		opus_bvh_leaf *current = stack[opus_arr_len(stack) - 1];
		opus_arr_pop(stack);

		if (current->is_leaf) {
			if (current->right == (void *) body) {
				return current;
			}
		} else {
			if (current->left && opus_aabb_is_overlap(&aabb, &(current->left->aabb))) {
				opus_arr_push(stack, &(current->left));
			}
			if (current->right && opus_aabb_is_overlap(&aabb, &(current->right->aabb))) {
				opus_arr_push(stack, &(current->right));
			}
		}
	}

	opus_arr_destroy(stack);
	return NULL;
}

/**
 * @brief Get an array of bodies (in container 'struct Collisions_BVH_Body *") which might possibly collide with the body you passed into.
 * 		This might cost much CPU, so do not use this if you have to deal with hordes of bodies at a time
 * @param bvh BVH tree
 * @param leaf the leaf containing the body you want to check, you can use 'magnum_bvh_find_leaf' to find the leaf in the BVH tree
 * @param result array (in magnum_array2.h)
 */
opus_body **opus_bvh_potentials(opus_bvh *bvh, opus_bvh_leaf *leaf, opus_body **result)
{
	opus_bvh_leaf **stack;
	opus_arr_create(stack, 10);
	opus_arr_push(stack, &(bvh->hierarchy));

	opus_arr_clear(result);
	while (opus_arr_len(stack) > 0) {
		opus_bvh_leaf *current = stack[opus_arr_len(stack) - 1];
		if (current == NULL) break;
		opus_arr_pop(stack);

		if (current->is_leaf) {
			if (current->right != leaf->right)
				opus_arr_push(result, &(current->right));
		} else {
			if (current->left && opus_aabb_is_overlap(&(leaf->aabb), &(current->left->aabb)))
				opus_arr_push(stack, &(current->left));
			if (current->right && opus_aabb_is_overlap(&(leaf->aabb), &(current->right->aabb)))
				opus_arr_push(stack, &(current->right));
		}
	}

	opus_arr_destroy(stack);

	return result;
}

static void bvh_check_two_branches(opus_bvh_leaf *n1, opus_bvh_leaf *n2, void (*callback)(opus_body *, opus_body *, void *), void *data)
{
	if (n1 == NULL || n2 == NULL) {
		if (n1 == NULL) opus_bvh_for_each_potential(n2, callback, data);
		if (n2 == NULL) opus_bvh_for_each_potential(n1, callback, data);
		return;
	}

	if (opus_aabb_is_overlap(&(n1->aabb), &(n2->aabb))) {
		if (n1->is_leaf) {
			if (n2->is_leaf) {
				callback((opus_body *) n1->right, (opus_body *) n2->right, data);
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
	opus_bvh_for_each_potential(n1, callback, data);
	opus_bvh_for_each_potential(n2, callback, data);
}

void opus_bvh_for_each_potential(opus_bvh_leaf *bvh_node, void (*callback)(opus_body *, opus_body *, void *), void *data)
{
	if (bvh_node == NULL) return;

	if (!bvh_node->is_leaf) {
		bvh_check_two_branches(bvh_node->left, bvh_node->right, callback, data);
	}
}
