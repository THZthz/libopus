/**
 * @file avl.h
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
#ifndef AVL_H
#define AVL_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdio.h>
#include "utils/utils.h"

#define MAX_(a, b) ((a) > (b) ? (a) : (b))
#define MIN_(a, b) ((a) < (b) ? (a) : (b))


typedef struct opus_avl      opus_avl;
typedef struct opus_avl_leaf opus_avl_leaf;

typedef int (*opus_avl_compare_cb)(opus_avl *avl, const void *ele_ptr_a, const void *ele_ptr_b);

struct opus_avl_leaf {
	int height;

	opus_avl_leaf *left, *right;
};

struct opus_avl {
	opus_avl_leaf *root;

	uint64_t leaf_count;
	uint64_t ele_size;

	void *temp_;

	opus_avl_compare_cb compare_;
};

opus_avl      *opus_avl_init(opus_avl *avl, uint64_t ele_size, opus_avl_compare_cb compare);
opus_avl      *opus_avl_create(uint64_t ele_size, opus_avl_compare_cb compare);
void           opus_avl_done(opus_avl *avl);
void           opus_avl_destroy(opus_avl *avl);
void           opus_avl_insert(opus_avl *avl, void *ele_ptr);
opus_avl_leaf *get_max_leaf(opus_avl *avl, opus_avl_leaf *root);
opus_avl_leaf *get_min_leaf(opus_avl *avl, opus_avl_leaf *root);
void          *opus_avl_delete(opus_avl *avl, void *ele_ptr);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* AVL_H */

