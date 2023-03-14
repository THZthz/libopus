#ifndef MAGNUM_TREE_PRINTER_H
#define MAGNUM_TREE_PRINTER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdlib.h>

#define TREE_PRINTER_MAX_VAL_LEN 64

typedef void *(*opus_tree_printer_get_left_child_cb)(void *cur);
typedef void *(*opus_tree_printer_get_right_child_cb)(void *cur);
typedef void (*opus_tree_printer_val2text_cb)(char *text, void *cur);

typedef struct {
	void *cur;
	void *tree;

	opus_tree_printer_get_left_child_cb get_left;
	opus_tree_printer_get_right_child_cb get_right;
	opus_tree_printer_val2text_cb        val2text;
} opus_tree_printer_context;

/**
 * start to print the \b binary \b tree
 */
void opus_tree_printer(opus_tree_printer_context *t);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* MAGNUM_TREE_PRINTER_H */
