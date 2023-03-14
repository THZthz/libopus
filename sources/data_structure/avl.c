/**
 * @file avl.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2023/3/13
 *
 * @example
 *
 * @development_log
 *
 */

#include "data_structure/avl.h"

static OPUS_INLINE int left_height_(opus_avl_leaf *leaf)
{
	if (!leaf->left) return -1;
	return leaf->left->height;
}

static OPUS_INLINE int right_height_(opus_avl_leaf *leaf)
{
	if (!leaf->right) return -1;
	return leaf->right->height;
}

/**
 *       b                     a
 *      / \                   / \
 *     a   e --------------> c   b
 *    / \                   / \
 *   c   d                 d   e
 */
/**
 * @return the root of the sub-tree; the node where this node used to be.
 */
static opus_avl_leaf *rotate_right_(opus_avl_leaf *leaf)
{
	opus_avl_leaf *A, *B, *D;

	B         = leaf;
	A         = B->left;
	D         = A->right;
	B->left   = D;
	A->right  = B;
	B->height = MAX_(left_height_(B), right_height_(B)) + 1;
	A->height = MAX_(left_height_(A), B->height) + 1;

	return A;
}

/**
 *     a                        b
 *    / \                      / \
 *   c   b   ------------->   a   e
 *      / \                  / \
 *     d   e                c   d
 */
/**
 * @return the root of the sub-tree; the node where this node used to be.
 */
static opus_avl_leaf *rotate_left_(opus_avl_leaf *leaf)
{
	opus_avl_leaf *A, *C;

	A         = leaf;
	C         = A->right;
	A->right  = C->left;
	C->left   = A;
	A->height = MAX_(left_height_(A), right_height_(A)) + 1;
	C->height = MAX_(left_height_(C), A->height) + 1;

	return C;
}

static OPUS_INLINE void *leaf_ele_ptr_(opus_avl_leaf *leaf)
{
	return (char *) leaf + sizeof(opus_avl_leaf);
}

static int OPUS_INLINE balance_factor_(opus_avl_leaf *leaf)
{
	return left_height_(leaf) - right_height_(leaf);
}

static opus_avl_leaf *leaf_create_(uint64_t ele_size, void *ele_ptr)
{
	opus_avl_leaf *leaf;
	leaf         = OPUS_MALLOC(sizeof(opus_avl_leaf) + ele_size);
	leaf->left   = NULL;
	leaf->right  = NULL;
	leaf->height = 0;
	/* copy element's data */
	memcpy(leaf_ele_ptr_(leaf), ele_ptr, ele_size);
	return leaf;
}

static void leaf_destroy_(opus_avl_leaf *leaf)
{
	OPUS_FREE(leaf);
}

opus_avl *opus_avl_init(opus_avl *avl, uint64_t ele_size, opus_avl_compare_cb compare)
{
	if (avl) {
		avl->root       = NULL;
		avl->leaf_count = 0;
		avl->ele_size   = ele_size;
		avl->compare_   = compare;
		avl->temp_      = OPUS_MALLOC(ele_size);
	}
	return avl;
}

opus_avl *opus_avl_create(uint64_t ele_size, opus_avl_compare_cb compare)
{
	opus_avl *avl;
	avl = OPUS_MALLOC(sizeof(opus_avl));
	return opus_avl_init(avl, ele_size, compare);
}

static void destroy_leaves_(opus_avl_leaf *leaf)
{
	if (!leaf) return;
	destroy_leaves_(leaf->left);
	destroy_leaves_(leaf->right);
	leaf_destroy_(leaf);
}

void opus_avl_done(opus_avl *avl)
{
	destroy_leaves_(avl->root);
	OPUS_FREE_R(avl->temp_);
}

void opus_avl_destroy(opus_avl *avl)
{
	opus_avl_done(avl);
	OPUS_FREE_R(avl);
}

static int compare_(opus_avl *avl, opus_avl_leaf *a, opus_avl_leaf *b)
{
	return avl->compare_(avl, leaf_ele_ptr_(a), leaf_ele_ptr_(b));
}

static opus_avl_leaf *insert_(opus_avl *avl, opus_avl_leaf *leaf, opus_avl_leaf *root)
{
	int c, factor;

	OPUS_RETURN_IF(leaf, !root); /* no root in this case */

	/* recursively insert leaf into appropriate place */
	c = compare_(avl, leaf, root);
	if (c < 0) {
		root->left = insert_(avl, leaf, root->left);
	} else if (c > 0) {
		root->right = insert_(avl, leaf, root->right);
	} else {
		avl->leaf_count--;
		return root;
	}

	root->height = MAX_(left_height_(root), right_height_(root)) + 1;
	factor       = balance_factor_(root);

	if (factor == 2) {                             /* left tree unbalanced */
		if (compare_(avl, leaf, root->left) < 0) { /* LL */
			root = rotate_right_(root);
		} else { /* LR */
			root->left = rotate_left_(root->left);
			return rotate_right_(root);
		}
	}

	if (factor == -2) {                             /* right tree unbalanced */
		if (compare_(avl, leaf, root->right) > 0) { /* RR */
			root = rotate_left_(root);
		} else { /* RL */
			root->right = rotate_right_(root->right);
			return rotate_left_(root);
		}
	}

	return root;
}

void opus_avl_insert(opus_avl *avl, void *ele_ptr)
{
	uint64_t       o;
	opus_avl_leaf *key;
	key       = leaf_create_(avl->ele_size, ele_ptr);
	o         = avl->leaf_count;
	avl->root = insert_(avl, key, avl->root);
	if (o < avl->leaf_count) leaf_destroy_(key); /* the key is not inserted */
	avl->leaf_count++;
}

opus_avl_leaf *get_max_leaf(opus_avl *avl, opus_avl_leaf *root)
{
	opus_avl_leaf *cur;
	cur = root;
	while (cur->right) cur = cur->right;
	return cur;
}

opus_avl_leaf *get_min_leaf(opus_avl *avl, opus_avl_leaf *root)
{
	opus_avl_leaf *cur;
	cur = root;
	while (cur->left) cur = cur->left;
	return cur;
}

static opus_avl_leaf *delete_(opus_avl *avl, opus_avl_leaf *leaf, opus_avl_leaf *root)
{
	int c, factor;

	opus_avl_leaf *min;

	if (!root) { /* can not find the key */
		avl->leaf_count++;
		return NULL;
	}

	c = compare_(avl, leaf, root);
	if (c < 0) {
		root->left = delete_(avl, leaf, root->left);
	} else if (c > 0) {
		root->right = delete_(avl, leaf, root->right);
	} else {
		if (!root->left && !root->right) {
			root = NULL;
		} else if (!root->left && root->right) {
			root = root->right;
		} else if (root->left && !root->right) {
			root = root->left;
		} else {
			min = get_min_leaf(avl, root->right);
			memcpy(avl->temp_, leaf_ele_ptr_(root), avl->ele_size);
			memcpy(leaf_ele_ptr_(root), leaf_ele_ptr_(min), avl->ele_size);
			root->right = delete_(avl, min, root->right);
		}
	}

	OPUS_RETURN_IF(NULL, !root);

	root->height = MAX_(left_height_(root), right_height_(root)) + 1;
	factor       = balance_factor_(root);

	if (factor == 2) {
		/* LL */
		if (balance_factor_(root->left) == 0 || balance_factor_(root->left) == 1)
			return rotate_right_(root);
		/* LR */
		if (balance_factor_(root->left) == -1) {
			root->left = rotate_left_(root->left);
			return rotate_right_(root);
		}
	}

	if (factor == -2) {
		/* RR */
		if (balance_factor_(root->right) == 0 || balance_factor_(root->right) == -1)
			return rotate_left_(root);
		/* RL */
		if (balance_factor_(root->right) == 1) {
			root->right = rotate_right_(root->right);
			return rotate_left_(root);
		}
	}

	return root;
}

void *opus_avl_delete(opus_avl *avl, void *ele_ptr)
{
	uint64_t       o;
	opus_avl_leaf *key;
	key       = leaf_create_(avl->ele_size, ele_ptr);
	o         = avl->leaf_count;
	avl->root = delete_(avl, key, avl->root);
	leaf_destroy_(key);
	if (o > avl->leaf_count)
		o = 1;
	else
		o = 0;
	avl->leaf_count--;
	return o ? NULL : avl->temp_;
}
