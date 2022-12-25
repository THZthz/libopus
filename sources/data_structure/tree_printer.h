#ifndef MAGNUM_TREE_PRINTER_H
#define MAGNUM_TREE_PRINTER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdlib.h>

#define TREE_PRINTER_MAX_VAL_LEN 64

typedef void *(*tree_printer_GET_LEFT_CHILD)(void *cur);
typedef void *(*tree_printer_GET_RIGHT_CHILD)(void *cur);
typedef void (*tree_printer_VAL2TEXT)(char *text, void *cur);

typedef struct {
	void                        *cur;
	void                        *tree;
	tree_printer_GET_LEFT_CHILD  get_left;
	tree_printer_GET_RIGHT_CHILD get_right;
	tree_printer_VAL2TEXT        val2text;
} tree_printer_context_t;

extern tree_printer_GET_LEFT_CHILD  tree_printer_get_left_child;
extern tree_printer_GET_RIGHT_CHILD tree_printer_get_right_child;
extern tree_printer_VAL2TEXT        tree_printer_val2text;

/**
 * start to print the \b binary \b tree
 */
void tree_printer(tree_printer_context_t *t);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* MAGNUM_TREE_PRINTER_H */