/**
 * @file heap.h
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/12/15
 *
 * @example
 *
 * @development_log
 *
 */
#ifndef HEAP_H
#define HEAP_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stddef.h>


typedef struct heap heap_t;
typedef int (*heap_compare_cb)(heap_t *heap, void *ele_a, void *ele_b);

/* minimum binary heap, the first element is the smallest */
struct heap {
	void *data_;
	void *context_;

	size_t capacity;
	size_t n_used;
	size_t ele_size;

	heap_compare_cb compare_;
};

void *heap_top(heap_t *heap);
int   heap_sift_up(heap_t *heap, size_t pos);
int   heap_sift_down(heap_t *heap, size_t start_pos, size_t pos);
void  heap_build(heap_t *heap);
void *heap_push(heap_t *heap, void *ele);
void *heap_remove(heap_t *heap, size_t idx);
void *heap_pop(heap_t *heap);
void *heap_push2(heap_t *heap, void *ele);
void *heap_pop2(heap_t *heap);
void *heap_remove2(heap_t *heap, size_t pos);

#ifdef __cplusplus
};
#endif /* __cplusplus */


#endif /* HEAP_H */
