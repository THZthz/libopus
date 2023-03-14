/**
 * @file avl_demo.c
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

#include "data_structure/tree_printer.h"
#include "data_structure/avl.h"


int c(opus_avl *avl, const void *a, const void *b)
{
	return *(int *) a - *(int *) b;
}

void *l(void *x)
{
	return ((opus_avl_leaf *) x)->left;
}

void *r(void *x)
{
	return ((opus_avl_leaf *) x)->right;
}

void v2t(char *t, void *x)
{
	x = (char *) x + sizeof(opus_avl_leaf);
	sprintf(t, "%d", *(int *) x);
}

int main()
{
	int i;

	opus_avl              *avl;
	opus_tree_printer_context t;

	avl = opus_avl_create(sizeof(int), c);
	for (i = 0; i < 10; i++) opus_avl_insert(avl, &i);
	t.cur       = avl->root;
	t.tree      = avl->root;
	t.get_left  = l;
	t.get_right = r;
	t.val2text  = v2t;
	opus_tree_printer(&t);
	printf("\n\n");
	i = 7;
	opus_avl_delete(avl, &i);
	opus_tree_printer(&t);
	printf("\n\n");
	i = 8;
	opus_avl_delete(avl, &i);
	opus_tree_printer(&t);
	opus_avl_destroy(avl);

	return 0;
}
