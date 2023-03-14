#include "data_structure/tree_printer.h"
#include "utils/utils.h"

#include <stdio.h>
#include <string.h>

#define MAX_(x, y) ((x) > (y) ? (x) : (y))
#define MIN_(x, y) ((x) > (y) ? (y) : (x))
#define PROFILE_MAX_DEPTH 1000
#define INFINITY (1 << 20)

typedef struct log_lef log_leaf;
struct log_lef {
	log_leaf *left, *right;

	int  edge_length; /* length of the edge from this node to its children */
	int  height, label_len;
	int  parent_dir; /* -1 = left, 0 = root, 1 = right */
	char label[TREE_PRINTER_MAX_VAL_LEN];
};

int left_profile[PROFILE_MAX_DEPTH];
int right_profile[PROFILE_MAX_DEPTH];

/**
 * @brief adjust gap between left and right leaves
 */
int gap = 1;

/**
 * @brief used for printing next node in the same level,
 * 		this is the x coordinate of the next char printed
 */
int print_next;

log_leaf *foo___(opus_tree_printer_context *t)
{
	log_leaf *leaf;
	void     *ori_cur;

	if (t->cur == NULL) return NULL;

	leaf        = (log_leaf *) malloc(sizeof(log_leaf));
	ori_cur     = t->cur;
	t->cur      = t->get_left(ori_cur);
	leaf->left  = foo___(t);
	t->cur      = t->get_right(ori_cur);
	leaf->right = foo___(t);

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
log_leaf *build_log_tree_(opus_tree_printer_context *t)
{
	log_leaf *leaf;
	if (t == NULL || t->tree == NULL) return NULL;

	leaf = foo___(t);

	leaf->parent_dir = 0;
	return leaf;
}

void destroy_log_tree_(log_leaf *leaf)
{
	if (leaf == NULL) return;
	destroy_log_tree_(leaf->left);
	destroy_log_tree_(leaf->right);
	OPUS_FREE(leaf); /* destroy current leaf only if its children are destroyed */
}

/**
 * @brief The following function fills in the tree_printer_l_profile array for the given tree.
 * 		It assumes that the center of the label of the root of this tree
 * 		is located at a position (x,y).  It assumes that the edge_length
 * 		fields have been computed for this tree.
 */
void compute_l_profile_(log_leaf *leaf, int x, int y)
{
	int i, is_left;
	if (leaf == NULL) return;
	is_left         = (leaf->parent_dir == -1);
	left_profile[y] = MIN_(left_profile[y], x - ((leaf->label_len - is_left) / 2));
	if (leaf->left != NULL) {
		for (i = 1; i <= leaf->edge_length && y + i < PROFILE_MAX_DEPTH; i++) {
			left_profile[y + i] = MIN_(left_profile[y + i], x - i);
		}
	}
	compute_l_profile_(leaf->left, x - leaf->edge_length - 1, y + leaf->edge_length + 1);
	compute_l_profile_(leaf->right, x + leaf->edge_length + 1, y + leaf->edge_length + 1);
}

void compute_r_profile_(log_leaf *leaf, int x, int y)
{
	int i, not_left;
	if (leaf == NULL) return;
	not_left         = (leaf->parent_dir != -1);
	right_profile[y] = MAX_(right_profile[y], x + ((leaf->label_len - not_left) / 2));
	if (leaf->right != NULL) {
		for (i = 1; i <= leaf->edge_length && y + i < PROFILE_MAX_DEPTH; i++) {
			right_profile[y + i] = MAX_(right_profile[y + i], x + i);
		}
	}
	compute_r_profile_(leaf->left, x - leaf->edge_length - 1, y + leaf->edge_length + 1);
	compute_r_profile_(leaf->right, x + leaf->edge_length + 1, y + leaf->edge_length + 1);
}

/**
 * @brief This function fills in the edge_length and
 * 		height fields of the specified tree
 */
void compute_edge_lengths_(log_leaf *node)
{
	int h, h_min, i, delta;
	if (node == NULL) return;
	compute_edge_lengths_(node->left);
	compute_edge_lengths_(node->right);

	/* first fill in the edge_length of node */
	if (node->right == NULL && node->left == NULL) {
		node->edge_length = 0;
	} else {
		if (node->left != NULL) {
			for (i = 0; i < node->left->height && i < PROFILE_MAX_DEPTH; i++) {
				right_profile[i] = -INFINITY;
			}
			compute_r_profile_(node->left, 0, 0);
			h_min = node->left->height;
		} else {
			h_min = 0;
		}
		if (node->right != NULL) {
			for (i = 0; i < node->right->height && i < PROFILE_MAX_DEPTH; i++) {
				left_profile[i] = INFINITY;
			}
			compute_l_profile_(node->right, 0, 0);
			h_min = MIN_(node->right->height, h_min);
		} else {
			h_min = 0;
		}
		delta = 4;
		for (i = 0; i < h_min; i++) {
			delta = MAX_(delta, gap + 1 + right_profile[i] - left_profile[i]);
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
		h = MAX_(node->left->height + node->edge_length + 1, h);
	}
	if (node->right != NULL) {
		h = MAX_(node->right->height + node->edge_length + 1, h);
	}
	node->height = h;
}

/**
 * @brief This function prints the given level of the given tree, assuming
 * 		that the node has the given x coordinate.
 */
void print_level_(log_leaf *node, int x, int level)
{
	int i, is_left;
	if (node == NULL) return;
	is_left = (node->parent_dir == -1);
	if (level == 0) {
		for (i = 0; i < (x - print_next - ((node->label_len - is_left) / 2)); i++) {
			printf(" ");
		}
		print_next += i;
		printf("%s", node->label);
		print_next += node->label_len;
	} else if (node->edge_length >= level) {
		if (node->left != NULL) {
			for (i = 0; i < (x - print_next - (level)); i++) {
				printf(" ");
			}
			print_next += i;
			printf("/");
			print_next++;
		}
		if (node->right != NULL) {
			for (i = 0; i < (x - print_next + (level)); i++) {
				printf(" ");
			}
			print_next += i;
			printf("\\");
			print_next++;
		}
	} else {
		print_level_(node->left,
		             x - node->edge_length - 1,
		             level - node->edge_length - 1);
		print_level_(node->right,
		             x + node->edge_length + 1,
		             level - node->edge_length - 1);
	}
}

void opus_tree_printer(opus_tree_printer_context *t)
{
	log_leaf *root;
	int       x_min, i;
	if (t == NULL) return;
	t->cur = t->tree;
	root   = build_log_tree_(t);
	compute_edge_lengths_(root);
	for (i = 0; i < root->height && i < PROFILE_MAX_DEPTH; i++) {
		left_profile[i] = INFINITY;
	}
	compute_l_profile_(root, 0, 0);
	x_min = 0;
	for (i = 0; i < root->height && i < PROFILE_MAX_DEPTH; i++) {
		x_min = MIN_(x_min, left_profile[i]);
	}
	for (i = 0; i < root->height; i++) {
		print_next = 0;
		print_level_(root, -x_min, i);
		printf("\n");
	}
	if (root->height >= PROFILE_MAX_DEPTH)
		OPUS_WARNING("(This tree is taller than %d, and may be drawn incorrectly.)\n", PROFILE_MAX_DEPTH);
	destroy_log_tree_(root);
}
