#ifndef AVL_H
#define AVL_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include <stddef.h>
#include <stdint.h>

/**************************************************************************************************************************************************************
 * AVL tree
 *************************************************************************************************************************************************************/

#include "utils/utils.h"

/* you can change this by config.h or predefined macro */
#ifndef TREE_ASSERTION
#define TREE_ASSERTION(x) ((void) 0)
#endif

struct avltree_node
{
	struct avltree_node *left;
	struct avltree_node *right;
	struct avltree_node *parent; /* pointing to node itself for empty node */
	int                 height; /* equals to 1 + max height in childs */
};

struct avltree_root
{
	struct avltree_node *node; /* root node */
};

typedef struct avltree_node avltree_node_t;
typedef struct avltree      avltree_t;

#define AVL_LEFT 0  /* left child index */
#define AVL_RIGHT 1 /* right child index */

#define AVL_OFFSET(TYPE, MEMBER) ((size_t) & ((TYPE *) 0)->MEMBER)

#define AVL_NODE2DATA(n, o) ((void *) ((size_t) (n) - (o)))
#define AVL_DATA2NODE(d, o) ((avltree_node_t *) ((size_t) (d) + (o)))

#define AVL_ENTRY(ptr, type, member) \
    ((type *) AVL_NODE2DATA(ptr, AVL_OFFSET(type, member)))

#define avl_node_init(node)        \
    do {                           \
        ((node)->parent) = (node); \
    } while (0)
#define avl_node_empty(node) ((node)->parent == (node))

#define AVL_LEFT_HEIGHT(node) (((node)->left) ? ((node)->left)->height : 0)
#define AVL_RIGHT_HEIGHT(node) (((node)->right) ? ((node)->right)->height : 0)

avltree_node_t *avltree_node_first(struct avltree_root *root);
avltree_node_t *avltree_node_last(struct avltree_root *root);
avltree_node_t *avltree_node_next(avltree_node_t *node);
avltree_node_t *avltree_node_prev(avltree_node_t *node);

void avltree_node_replace(avltree_node_t *victim, avltree_node_t *newnode, struct avltree_root *root);

static INLINE void avl_node_link(avltree_node_t *node, avltree_node_t *parent, avltree_node_t **avl_link)
{
	node->parent = parent;
	node->height = 0;
	node->left   = node->right = NULL;
	avl_link[0] = node;
}

/* avl insert rebalance and erase */
void avltree_node_post_insert(avltree_node_t *node, struct avltree_root *root);
void avltree_node_erase(avltree_node_t *node, struct avltree_root *root);

/* tear down the whole tree */
avltree_node_t *avltree_node_tear(struct avltree_root *root, avltree_node_t **next);

#define avl_node_find(root, what, compare_fn, res_node) \
    do {                                                \
        struct avltree_node *__n = (root)->node;        \
        (res_node)               = NULL;                \
        while (__n) {                                   \
            int __hr = (compare_fn) (what, __n);        \
            if (__hr == 0) {                            \
                (res_node) = __n;                       \
                break;                                  \
            } else if (__hr < 0) {                      \
                __n = __n->left;                        \
            } else {                                    \
                __n = __n->right;                       \
            }                                           \
        }                                               \
    } while (0)


#define avl_node_add(root, newnode, compare_fn, duplicate_node) \
    do {                                                        \
        struct avltree_node **__link      = &((root)->node);    \
        struct avltree_node  *__parent    = NULL;               \
        struct avltree_node  *__duplicate = NULL;               \
        int                   __hr        = 1;                  \
        while (__link[0]) {                                     \
            __parent = __link[0];                               \
            __hr     = (compare_fn) (newnode, __parent);        \
            if (__hr == 0) {                                    \
                __duplicate = __parent;                         \
                break;                                          \
            } else if (__hr < 0) {                              \
                __link = &(__parent->left);                     \
            } else {                                            \
                __link = &(__parent->right);                    \
            }                                                   \
        }                                                       \
        (duplicate_node) = __duplicate;                         \
        if (__duplicate == NULL) {                              \
            avl_node_link(newnode, __parent, __link);           \
            avl_node_post_insert(newnode, root);                \
        }                                                       \
    } while (0)

struct avltree
{
	struct avltree_root root;   /* avl root */
	size_t              offset; /* node offset in user data structure */
	size_t              size;   /* size of user data structure */
	size_t              count;  /* node count */
	/* returns 0 for equal, -1 for n1 < n2, 1 for n1 > n2 */
	int (*compare)(const void *n1, const void *n2);
};

/* initialize avltree, use AVL_OFFSET(type, member) for "offset"
 * eg:
 *     avl_tree_init(&mytree, mystruct_compare,
 *          sizeof(struct mystruct_t),
 *          AVL_OFFSET(struct mystruct_t, node));
 */
void avltree_init(avltree_t *tree, int (*compare)(const void *, const void *), size_t size, size_t offset);

void *avltree_first(avltree_t *tree);
void *avltree_last(avltree_t *tree);
void *avltree_next(avltree_t *tree, void *data);
void *avltree_prev(avltree_t *tree, void *data);

/* require a temporary user structure (data) which contains the key */
void *avltree_search(avltree_t *tree, const void *data);
void *avltree_nearest(avltree_t *tree, const void *data);

/* returns NULL for success, otherwise returns conflict node with same key */
void *avltree_insert(avltree_t *tree, void *data);

void avltree_remove(avltree_t *tree, void *data);
void avltree_replace(avltree_t *tree, void *victim, void *newdata);

void avltree_clear(avltree_t *tree, void (*destroy)(void *data));

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* AVL_H */
