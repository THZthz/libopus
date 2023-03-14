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
#include <stdint.h>

#include "data_structure/array.h"
#include "utils/utils.h"

#define OPUS_HEAP_INITIAL_SIZE (64)

typedef struct opus_heap opus_heap;

typedef int (*opus_heap_compare_cb)(opus_heap *heap, const void *ele_ptr_a, const void *ele_ptr_b);

/**
 * @brief <p>
 * 		A Binary Heap is a <b>complete binary tree</b> which is used to store data efficiently
 * 		to get the max or min element based on its structure. Usually it is implemented as an array
 * 		</p>
 */
struct opus_heap {
	uint64_t ele_size;
	uint64_t last_index; /* index of the last element of the heap, 0 means no elements in heap */

	void *data_;

	opus_heap_compare_cb compare_;
};


uint64_t   opus_heap_parent(uint64_t index);
uint64_t   opus_heap_left_child(uint64_t index);
uint64_t   opus_heap_right_child(uint64_t index);

opus_heap *opus_heap_init(opus_heap *heap, uint64_t ele_size, opus_heap_compare_cb compare);
opus_heap *opus_heap_create(uint64_t ele_size, opus_heap_compare_cb compare);
void       opus_heap_done(opus_heap *heap);
void       opus_heap_destroy(opus_heap *heap);

void       opus_heap_swap(opus_heap *heap, uint64_t i, uint64_t j);
void       opus_heap_sift_up(opus_heap *heap, uint64_t index);
void       opus_heap_sift_down(opus_heap *heap, uint64_t index);

void       opus_heap_insert(opus_heap *heap, void *ele_ptr);
void      *opus_heap_pop(opus_heap *heap);
void      *opus_heap_top(opus_heap *heap);
void       opus_heap_update(opus_heap *heap, uint64_t n);

#ifdef __cplusplus
};
#endif /* __cplusplus */


#endif /* HEAP_H */
