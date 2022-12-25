#include "data_structure/avl.h"


avltree_node_t *avltree_node_first(struct avltree_root *root)
{
	avltree_node_t *node = root->node;
	if (node == NULL) return NULL;
	while (node->left)
		node = node->left;
	return node;
}

avltree_node_t *avltree_node_last(struct avltree_root *root)
{
	avltree_node_t *node = root->node;
	if (node == NULL) return NULL;
	while (node->right)
		node = node->right;
	return node;
}

avltree_node_t *avltree_node_next(avltree_node_t *node)
{
	if (node == NULL) return NULL;
	if (node->right) {
		node = node->right;
		while (node->left)
			node = node->left;
	} else {
		while (1) {
			avltree_node_t *last = node;
			node                 = node->parent;
			if (node == NULL) break;
			if (node->left == last) break;
		}
	}
	return node;
}

avltree_node_t *avltree_node_prev(avltree_node_t *node)
{
	if (node == NULL) return NULL;
	if (node->left) {
		node = node->left;
		while (node->right)
			node = node->right;
	} else {
		while (1) {
			avltree_node_t *last = node;
			node                 = node->parent;
			if (node == NULL) break;
			if (node->right == last) break;
		}
	}
	return node;
}

static INLINE void
_avl_child_replace(avltree_node_t *oldnode, avltree_node_t *newnode,
                   avltree_node_t *parent, struct avltree_root *root)
{
	if (parent) {
		if (parent->left == oldnode)
			parent->left = newnode;
		else
			parent->right = newnode;
	} else {
		root->node = newnode;
	}
}

static INLINE avltree_node_t *
_avltree_node_rotate_left(avltree_node_t *node, struct avltree_root *root)
{
	avltree_node_t *right  = node->right;
	avltree_node_t *parent = node->parent;
	node->right            = right->left;
	TREE_ASSERTION(node && right);
	if (right->left)
		right->left->parent = node;
	right->left   = node;
	right->parent = parent;
	_avl_child_replace(node, right, parent, root);
	node->parent = right;
	return right;
}

static INLINE avltree_node_t *
_avltree_node_rotate_right(avltree_node_t *node, struct avltree_root *root)
{
	avltree_node_t *left   = node->left;
	avltree_node_t *parent = node->parent;
	node->left             = left->right;
	TREE_ASSERTION(node && left);
	if (left->right)
		left->right->parent = node;
	left->right  = node;
	left->parent = parent;
	_avl_child_replace(node, left, parent, root);
	node->parent = left;
	return left;
}

void avltree_node_replace(avltree_node_t *victim, avltree_node_t *newnode,
                          struct avltree_root *root)
{
	avltree_node_t *parent = victim->parent;
	_avl_child_replace(victim, newnode, parent, root);
	if (victim->left) victim->left->parent = newnode;
	if (victim->right) victim->right->parent = newnode;
	newnode->left   = victim->left;
	newnode->right  = victim->right;
	newnode->parent = victim->parent;
	newnode->height = victim->height;
}


/*--------------------------------------------------------------------*/
/* avl - node manipulation                                            */
/*--------------------------------------------------------------------*/

static INLINE int AVL_MAX(int x, int y)
{
	return (x < y) ? y : x;
}

static INLINE void
_avltree_node_height_update(avltree_node_t *node)
{
	int h0       = AVL_LEFT_HEIGHT(node);
	int h1       = AVL_RIGHT_HEIGHT(node);
	node->height = AVL_MAX(h0, h1) + 1;
}

static INLINE avltree_node_t *
_avltree_node_fix_l(avltree_node_t *node, struct avltree_root *root)
{
	avltree_node_t *right = node->right;
	int             rh0, rh1;
	TREE_ASSERTION(right);
	rh0 = AVL_LEFT_HEIGHT(right);
	rh1 = AVL_RIGHT_HEIGHT(right);
	if (rh0 > rh1) {
		right = _avltree_node_rotate_right(right, root);
		_avltree_node_height_update(right->right);
		_avltree_node_height_update(right);
		/* _avl_node_height_update(node); */
	}
	node = _avltree_node_rotate_left(node, root);
	_avltree_node_height_update(node->left);
	_avltree_node_height_update(node);
	return node;
}

static INLINE avltree_node_t *
_avltree_node_fix_r(avltree_node_t *node, struct avltree_root *root)
{
	avltree_node_t *left = node->left;
	int             rh0, rh1;
	TREE_ASSERTION(left);
	rh0 = AVL_LEFT_HEIGHT(left);
	rh1 = AVL_RIGHT_HEIGHT(left);
	if (rh0 < rh1) {
		left = _avltree_node_rotate_left(left, root);
		_avltree_node_height_update(left->left);
		_avltree_node_height_update(left);
		/* _avl_node_height_update(node); */
	}
	node = _avltree_node_rotate_right(node, root);
	_avltree_node_height_update(node->right);
	_avltree_node_height_update(node);
	return node;
}

static INLINE void
_avltree_node_rebalance(avltree_node_t *node, struct avltree_root *root)
{
	while (node) {
		int h0     = (int) AVL_LEFT_HEIGHT(node);
		int h1     = (int) AVL_RIGHT_HEIGHT(node);
		int diff   = h0 - h1;
		int height = AVL_MAX(h0, h1) + 1;
		if (node->height != height) {
			node->height = height;
		} else if (diff >= -1 && diff <= 1) {
			break;
		}
		/* printf("rebalance %d\n", avl_value(node)); */
		if (diff <= -2) {
			node = _avltree_node_fix_l(node, root);
		} else if (diff >= 2) {
			node = _avltree_node_fix_r(node, root);
		}
		node = node->parent;
		/* printf("parent %d\n", (!node)? -1 : avl_value(node)); */
	}
}

void avltree_node_post_insert(avltree_node_t *node, struct avltree_root *root)
{
	node->height = 1;
#if 0
	_avl_node_rebalance(node->parent, root);
#else
	for (node = node->parent; node; node = node->parent) {
		int h0     = (int) AVL_LEFT_HEIGHT(node);
		int h1     = (int) AVL_RIGHT_HEIGHT(node);
		int height = AVL_MAX(h0, h1) + 1;
		int diff   = h0 - h1;
		if (node->height == height) break;
		node->height = height;
		/* printf("rebalance %d\n", avl_value(node)); */
		if (diff <= -2) {
			node = _avltree_node_fix_l(node, root);
		} else if (diff >= 2) {
			node = _avltree_node_fix_r(node, root);
		}
		/* printf("parent %d\n", (!node)? -1 : avl_value(node)); */
	}
#endif
}

void avltree_node_erase(avltree_node_t *node, struct avltree_root *root)
{
	avltree_node_t *child, *parent;
	TREE_ASSERTION(node);
	if (node->left && node->right) {
		avltree_node_t *old = node;
		avltree_node_t *left;
		node = node->right;
		while ((left = node->left) != NULL)
			node = left;
		child  = node->right;
		parent = node->parent;
		if (child) {
			child->parent = parent;
		}
		_avl_child_replace(node, child, parent, root);
		if (node->parent == old)
			parent = node;
		node->left   = old->left;
		node->right  = old->right;
		node->parent = old->parent;
		node->height = old->height;
		_avl_child_replace(old, node, old->parent, root);
		TREE_ASSERTION(old->left);
		old->left->parent = node;
		if (old->right) {
			old->right->parent = node;
		}
	} else {
		if (node->left == NULL)
			child = node->right;
		else
			child = node->left;
		parent = node->parent;
		_avl_child_replace(node, child, parent, root);
		if (child) {
			child->parent = parent;
		}
	}
	if (parent) {
		_avltree_node_rebalance(parent, root);
	}
}


/* tear down the whole tree */
avltree_node_t *avltree_node_tear(struct avltree_root *root, avltree_node_t **next)
{
	avltree_node_t *node = *next;
	avltree_node_t *parent;
	if (node == NULL) {
		if (root->node == NULL)
			return NULL;
		node = root->node;
	}
	/* sink down to the leaf */
	while (1) {
		if (node->left) node = node->left;
		else if (node->right)
			node = node->right;
		else
			break;
	}
	/* tear down one leaf */
	parent = node->parent;
	if (parent == NULL) {
		*next      = NULL;
		root->node = NULL;
		return node;
	}
	if (parent->left == node) {
		parent->left = NULL;
	} else {
		parent->right = NULL;
	}
	node->height = 0;
	*next        = parent;
	return node;
}


/*====================================================================*/
/* avl_tree - easy interface                                          */
/*====================================================================*/

void avltree_init(avltree_t *tree, int (*compare)(const void *, const void *), size_t size, size_t offset)
{
	tree->root.node = NULL;
	tree->offset    = offset;
	tree->size      = size;
	tree->count     = 0;
	tree->compare   = compare;
}


void *avltree_first(avltree_t *tree)
{
	avltree_node_t *node = avltree_node_first(&tree->root);
	if (!node) return NULL;
	return AVL_NODE2DATA(node, tree->offset);
}

void *avltree_last(avltree_t *tree)
{
	avltree_node_t *node = avltree_node_last(&tree->root);
	if (!node) return NULL;
	return AVL_NODE2DATA(node, tree->offset);
}

/**
 * in-order traverse the avl tree, the output array is sorted according to the compare
 * function you give
 * @param tree
 * @param data
 * @return
 */
void *avltree_next(avltree_t *tree, void *data)
{
	avltree_node_t *nn;
	if (!data) return NULL;
	nn = AVL_DATA2NODE(data, tree->offset);
	nn = avltree_node_next(nn);
	if (!nn) return NULL;
	return AVL_NODE2DATA(nn, tree->offset);
}

void *avltree_prev(avltree_t *tree, void *data)
{
	avltree_node_t *nn;
	if (!data) return NULL;
	nn = AVL_DATA2NODE(data, tree->offset);
	nn = avltree_node_prev(nn);
	if (!nn) return NULL;
	return AVL_NODE2DATA(nn, tree->offset);
}


/**
 * require a temporary user structure (data) which contains the key
 * @param tree
 * @param data a pointer to a temporary user structure
 * @return User's struct if search succeed, otherwise NULL
 */
void *avltree_search(avltree_t *tree, const void *data)
{
	avltree_node_t *n                          = tree->root.node;
	int (*compare)(const void *, const void *) = tree->compare;
	int offset                                 = tree->offset;
	while (n) {
		void *nd = AVL_NODE2DATA(n, offset);
		int   hr = compare(data, nd);
		if (hr == 0) {
			return nd;
		} else if (hr < 0) {
			n = n->left;
		} else {
			n = n->right;
		}
	}
	return NULL;
}

/**
 * find the nearest node to the node you specify
 * @param tree
 * @param data
 * @return
 */
void *avltree_nearest(avltree_t *tree, const void *data)
{
	avltree_node_t *n                          = tree->root.node;
	avltree_node_t *p                          = NULL;
	int (*compare)(const void *, const void *) = tree->compare;
	int offset                                 = tree->offset;
	while (n) {
		void *nd = AVL_NODE2DATA(n, offset);
		int   hr = compare(data, nd);
		p        = n;
		if (hr == 0) {
			return nd;
		} else if (hr < 0) {
			n = n->left;
		} else {
			n = n->right;
		}
	}
	return (p) ? AVL_NODE2DATA(p, offset) : NULL;
}

/**
 *
 * @param tree
 * @param data
 * @return NULL for success, otherwise returns conflict node with same key
 */
void *avltree_insert(avltree_t *tree, void *data)
{
	avltree_node_t **link                      = &tree->root.node;
	avltree_node_t  *parent                    = NULL;
	avltree_node_t  *node                      = AVL_DATA2NODE(data, tree->offset);
	int (*compare)(const void *, const void *) = tree->compare;
	int offset                                 = tree->offset;
	while (link[0]) {
		void *pd;
		int   hr;
		parent = link[0];
		pd     = AVL_NODE2DATA(parent, offset);
		hr     = compare(data, pd);
		if (hr == 0) {
			return pd;
		} else if (hr < 0) {
			link = &(parent->left);
		} else {
			link = &(parent->right);
		}
	}
	avl_node_link(node, parent, link);
	avltree_node_post_insert(node, &tree->root);
	tree->count++;
	return NULL;
}

void avltree_remove(avltree_t *tree, void *data)
{
	avltree_node_t *node = AVL_DATA2NODE(data, tree->offset);
	if (!avl_node_empty(node)) {
		avltree_node_erase(node, &tree->root);
		node->parent = node;
		tree->count--;
	}
}

void avltree_replace(avltree_t *tree, void *victim, void *newdata)
{
	avltree_node_t *vicnode = AVL_DATA2NODE(victim, tree->offset);
	avltree_node_t *newnode = AVL_DATA2NODE(newdata, tree->offset);
	avltree_node_replace(vicnode, newnode, &tree->root);
	vicnode->parent = vicnode;
}

void avltree_clear(avltree_t *tree, void (*destroy)(void *data))
{
	avltree_node_t *next = NULL;
	avltree_node_t *node = NULL;
	while (1) {
		void *data;
		node = avltree_node_tear(&tree->root, &next);
		if (node == NULL) break;
		data         = AVL_NODE2DATA(node, tree->offset);
		node->parent = node;
		tree->count--;
		if (destroy) destroy(data);
	}
	TREE_ASSERTION(tree->count == 0);
}


#if 0

#include <stdio.h>
#include <stdlib.h>

#include "data_structure/tree.h"
#include "data_structure/tree/tree_printer.h"
#include "event.h"
#include "math/core_math.h"
#include "utils/core_utils.h"
#include "utils/leak_detector.h"

struct test {
	int            key;
	avltree_node_t node;
};

int avl_cmp_(const void *a, const void *b)
{
	return ((struct test *) a)->key - ((struct test *) b)->key;
}

struct test *avl_from_node(void *node) { return node - offsetof(struct test, node); }
void        *avl_get_left(void *cur)
{
	struct test *t = cur;
	void        *v = t->node.left;
	if (v == NULL) return NULL;
	return avl_from_node(v);
}
void *avl_get_right(void *cur)
{
	struct test *t = cur;
	void        *v = t->node.right;
	if (v == NULL) return NULL;
	return avl_from_node(v);
}
void avl_val2text(char *text, void *cur)
{
	struct test *t = cur;
	sprintf(text, "%d", t->key);
}

int main()
{
	avltree_t tree;
	avltree_init(&tree, avl_cmp_, sizeof(struct test), AVL_OFFSET(struct test, node));

	struct test arr[10];
	for (int i = 0; i < 10; i++) arr[i].key = core_random_lcrc(1, 20);
	for (int i = 0; i < 10; i++) avltree_insert(&tree, &arr[i]);

	struct test  t   = {1};
	struct test *res = avltree_search(&tree, &t);
	printf("%d\n", res->key);

	t.key = 6;
	res   = avltree_nearest(&tree, &t);
	printf("%d\n", res->key);

	struct test *cur = avltree_first(&tree), *last = avltree_last(&tree);
	printf("%d\n", cur->key);
	do {
		cur = avltree_next(&tree, cur);
		printf("%d\n", cur->key);
	} while (cur != last);

	tree_printer_context_t c2 = {
	        NULL,
	        avl_from_node(tree.root.node),
	        avl_get_left, avl_get_right, avl_val2text};
	tree_printer_work(&c2);

	return 0;
}
#endif /* 0 */