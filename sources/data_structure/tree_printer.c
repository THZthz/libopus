#include "data_structure/tree_printer.h"

#include <stdio.h>
#include <string.h>

#define tree_printer_max(x, y) ((x) > (y) ? (x) : (y))
#define tree_printer_min(x, y) ((x) > (y) ? (y) : (x))
#define tree_printer_MAX_HEIGHT 1000
#define tree_printer_INFINITY (1 << 20)

typedef struct tree_printer_ascii_node_struct tree_printer_log_leaf;
struct tree_printer_ascii_node_struct {
	tree_printer_log_leaf *left, *right;

	int  edge_length; /* length of the edge from this node to its children */
	int  height, label_len;
	int  parent_dir; /* -1 = left, 0 = root, 1 = right */
	char label[TREE_PRINTER_MAX_VAL_LEN];
};

int tree_printer_l_profile[tree_printer_MAX_HEIGHT];
int tree_printer_r_profile[tree_printer_MAX_HEIGHT];

/**
 * @brief adjust gap between left and right leaves
 */
int tree_printer_gap = 1;

/**
 * @brief used for printing next node in the same level,
 * 		this is the x coordinate of the next char printed
 */
int tree_printer_print_next;

tree_printer_log_leaf *tree_printer_build_log_tree_recursive(tree_printer_context_t *t)
{
	tree_printer_log_leaf *leaf;
	void                  *ori_cur;

	if (t->cur == NULL) return NULL;

	leaf        = (tree_printer_log_leaf *) malloc(sizeof(tree_printer_log_leaf));
	ori_cur     = t->cur;
	t->cur      = t->get_left(ori_cur);
	leaf->left  = tree_printer_build_log_tree_recursive(t);
	t->cur      = t->get_right(ori_cur);
	leaf->right = tree_printer_build_log_tree_recursive(t);

	if (leaf->left != NULL) {
		leaf->left->parent_dir = -1;
	}

	if (leaf->right != NULL) {
		leaf->right->parent_dir = 1;
	}

	t->val2text(leaf->label, ori_cur);
	leaf->label_len = (int) strlen(leaf->label);

	return leaf;
}

/**
 * @brief build a mirror tree to ease the logging of the tree
 */
tree_printer_log_leaf *tree_printer_build_log_tree(tree_printer_context_t *t)
{
	tree_printer_log_leaf *leaf;
	if (t == NULL || t->tree == NULL) return NULL;
	leaf             = tree_printer_build_log_tree_recursive(t);
	leaf->parent_dir = 0;
	return leaf;
}

void tree_printer_destroy_log_tree(tree_printer_log_leaf *leaf)
{
	if (leaf == NULL) return;
	tree_printer_destroy_log_tree(leaf->left);
	tree_printer_destroy_log_tree(leaf->right);
	free(leaf); /* destroy current leaf only if its children are destroyed */
}

/**
 * @brief The following function fills in the tree_printer_l_profile array for the given tree.
 * 		It assumes that the center of the label of the root of this tree
 * 		is located at a position (x,y).  It assumes that the edge_length
 * 		fields have been computed for this tree.
 */
void tree_printer_compute_l_profile(tree_printer_log_leaf *leaf, int x, int y)
{
	int i, is_left;
	if (leaf == NULL) return;
	is_left                     = (leaf->parent_dir == -1);
	tree_printer_l_profile[y] = tree_printer_min(tree_printer_l_profile[y], x - ((leaf->label_len - is_left) / 2));
	if (leaf->left != NULL) {
		for (i = 1; i <= leaf->edge_length && y + i < tree_printer_MAX_HEIGHT; i++) {
			tree_printer_l_profile[y + i] = tree_printer_min(tree_printer_l_profile[y + i], x - i);
		}
	}
	tree_printer_compute_l_profile(leaf->left, x - leaf->edge_length - 1, y + leaf->edge_length + 1);
	tree_printer_compute_l_profile(leaf->right, x + leaf->edge_length + 1, y + leaf->edge_length + 1);
}

void tree_printer_compute_r_profile(tree_printer_log_leaf *leaf, int x, int y)
{
	int i, not_left;
	if (leaf == NULL) return;
	not_left                    = (leaf->parent_dir != -1);
	tree_printer_r_profile[y] = tree_printer_max(tree_printer_r_profile[y], x + ((leaf->label_len - not_left) / 2));
	if (leaf->right != NULL) {
		for (i = 1; i <= leaf->edge_length && y + i < tree_printer_MAX_HEIGHT; i++) {
			tree_printer_r_profile[y + i] = tree_printer_max(tree_printer_r_profile[y + i], x + i);
		}
	}
	tree_printer_compute_r_profile(leaf->left, x - leaf->edge_length - 1, y + leaf->edge_length + 1);
	tree_printer_compute_r_profile(leaf->right, x + leaf->edge_length + 1, y + leaf->edge_length + 1);
}

/**
 * @brief This function fills in the edge_length and
 * 		height fields of the specified tree
 */
void tree_printer_compute_edge_lengths(tree_printer_log_leaf *node)
{
	int h, h_min, i, delta;
	if (node == NULL) return;
	tree_printer_compute_edge_lengths(node->left);
	tree_printer_compute_edge_lengths(node->right);

	/* first fill in the edge_length of node */
	if (node->right == NULL && node->left == NULL) {
		node->edge_length = 0;
	} else {
		if (node->left != NULL) {
			for (i = 0; i < node->left->height && i < tree_printer_MAX_HEIGHT; i++) {
				tree_printer_r_profile[i] = -tree_printer_INFINITY;
			}
			tree_printer_compute_r_profile(node->left, 0, 0);
			h_min = node->left->height;
		} else {
			h_min = 0;
		}
		if (node->right != NULL) {
			for (i = 0; i < node->right->height && i < tree_printer_MAX_HEIGHT; i++) {
				tree_printer_l_profile[i] = tree_printer_INFINITY;
			}
			tree_printer_compute_l_profile(node->right, 0, 0);
			h_min = tree_printer_min(node->right->height, h_min);
		} else {
			h_min = 0;
		}
		delta = 4;
		for (i = 0; i < h_min; i++) {
			delta = tree_printer_max(delta, tree_printer_gap + 1 + tree_printer_r_profile[i] - tree_printer_l_profile[i]);
		}

		/* If the node has two children of height 1, then we allow the */
		/* two leaves to be within 1, instead of 2 */
		if (((node->left != NULL && node->left->height == 1) ||
		     (node->right != NULL && node->right->height == 1)) &&
		    delta > 4) {
			delta--;
		}

		node->edge_length = ((delta + 1) / 2) - 1;
	}

	/* now fill in the height of node */
	h = 1;
	if (node->left != NULL) {
		h = tree_printer_max(node->left->height + node->edge_length + 1, h);
	}
	if (node->right != NULL) {
		h = tree_printer_max(node->right->height + node->edge_length + 1, h);
	}
	node->height = h;
}

/**
 * @brief This function prints the given level of the given tree, assuming
 * 		that the node has the given x coordinate.
 */
void tree_printer_print_level(tree_printer_log_leaf *node, int x, int level)
{
	int i, is_left;
	if (node == NULL) return;
	is_left = (node->parent_dir == -1);
	if (level == 0) {
		for (i = 0; i < (x - tree_printer_print_next - ((node->label_len - is_left) / 2)); i++) {
			printf(" ");
		}
		tree_printer_print_next += i;
		printf("%s", node->label);
		tree_printer_print_next += node->label_len;
	} else if (node->edge_length >= level) {
		if (node->left != NULL) {
			for (i = 0; i < (x - tree_printer_print_next - (level)); i++) {
				printf(" ");
			}
			tree_printer_print_next += i;
			printf("/");
			tree_printer_print_next++;
		}
		if (node->right != NULL) {
			for (i = 0; i < (x - tree_printer_print_next + (level)); i++) {
				printf(" ");
			}
			tree_printer_print_next += i;
			printf("\\");
			tree_printer_print_next++;
		}
	} else {
		tree_printer_print_level(node->left,
		                         x - node->edge_length - 1,
		                         level - node->edge_length - 1);
		tree_printer_print_level(node->right,
		                         x + node->edge_length + 1,
		                         level - node->edge_length - 1);
	}
}

__attribute__((unused)) void tree_printer(tree_printer_context_t *t)
{
	tree_printer_log_leaf *root;
	int                   x_min, i;
	if (t == NULL) return;
	t->cur = t->tree;
	root   = tree_printer_build_log_tree(t);
	tree_printer_compute_edge_lengths(root);
	for (i = 0; i < root->height && i < tree_printer_MAX_HEIGHT; i++) {
		tree_printer_l_profile[i] = tree_printer_INFINITY;
	}
	tree_printer_compute_l_profile(root, 0, 0);
	x_min = 0;
	for (i = 0; i < root->height && i < tree_printer_MAX_HEIGHT; i++) {
		x_min = tree_printer_min(x_min, tree_printer_l_profile[i]);
	}
	for (i = 0; i < root->height; i++) {
		tree_printer_print_next = 0;
		tree_printer_print_level(root, -x_min, i);
		printf("\n");
	}
#ifndef NDEBUG
	if (root->height >= tree_printer_MAX_HEIGHT) {
		printf("(This tree is taller than %d, and may be drawn incorrectly.)\n", tree_printer_MAX_HEIGHT);
	}
#endif /* NDEBUG */
	tree_printer_destroy_log_tree(root);
}
