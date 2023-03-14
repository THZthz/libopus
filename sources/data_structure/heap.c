/**
 * @file heap.c
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

#include "data_structure/heap.h"

#define ELE_PTR(_heap, _i) ((char *) (_heap)->data_ + (_heap)->ele_size * (_i))
#define PARENT(_i) ((_i) / 2)
#define LEFT(_i) ((_i) *2)
#define RIGHT(_i) ((_i) *2 + 1)

uint64_t opus_heap_parent(uint64_t index)
{
	return index / 2;
}

uint64_t opus_heap_left_child(uint64_t index)
{
	return 2 * index;
}

uint64_t opus_heap_right_child(uint64_t index)
{
	return 2 * index + 1;
}

opus_heap *opus_heap_init(opus_heap *heap, uint64_t ele_size, opus_heap_compare_cb compare)
{
	if (heap) {
		heap->ele_size   = ele_size;
		heap->last_index = 0;
		opus_arr_create(heap->data_, ele_size);
		opus_arr_reserve(heap->data_, OPUS_HEAP_INITIAL_SIZE + 1);
		opus_arr_set_len(heap->data_, 1);
		heap->compare_ = compare;
	}
	return heap;
}

opus_heap *opus_heap_create(uint64_t ele_size, opus_heap_compare_cb compare)
{
	opus_heap *heap;
	heap = OPUS_MALLOC(sizeof(opus_heap));
	return opus_heap_init(heap, ele_size, compare);
}

void opus_heap_done(opus_heap *heap)
{
	opus_arr_destroy(heap->data_);
}

void opus_heap_destroy(opus_heap *heap)
{
	opus_heap_done(heap);
	OPUS_FREE(heap);
}

void opus_heap_swap(opus_heap *heap, uint64_t i, uint64_t j)
{
	memcpy(heap->data_, ELE_PTR(heap, i), heap->ele_size);
	memcpy(ELE_PTR(heap, i), ELE_PTR(heap, j), heap->ele_size);
	memcpy(ELE_PTR(heap, j), heap->data_, heap->ele_size);
}

/**
 * @brief iteratively compare child with parent, and exchange them if the child is bigger (max binary heap)
 * 		until child is not bigger than its parent or it becomes the root
 * @param heap
 * @param index
 */
void opus_heap_sift_up(opus_heap *heap, uint64_t index)
{
	uint64_t child, parent;
	for (child  = index;
	     parent = PARENT(child), child > 1 && heap->compare_(heap, ELE_PTR(heap, parent), ELE_PTR(heap, child)) < 0;
	     child  = parent)
        opus_heap_swap(heap, child, PARENT(child));
}

/**
 * @brief find the child we need to compare with parent
 * @param heap
 * @param index
 * @return
 */
static uint64_t proper_child_(opus_heap *heap, uint64_t index)
{
	return index * 2 +
	       (index * 2 + 1 <= heap->last_index &&
	        heap->compare_(heap, ELE_PTR(heap, RIGHT(index)), ELE_PTR(heap, LEFT(index))) > 0);
}

/**
 * @brief iteratively compare parent with its bigger child and swap them if parent is smaller (max binary heap)
 * 		until parent is not smaller than any of its child or it becomes the leaf
 * @param heap
 * @param index
 */
void opus_heap_sift_down(opus_heap *heap, uint64_t index)
{
	uint64_t parent, child;
	for (parent = index, child = proper_child_(heap, index);
	     child <= heap->last_index && heap->compare_(heap, ELE_PTR(heap, child), ELE_PTR(heap, parent)) > 0;
	     parent = child, child = proper_child_(heap, child))
		opus_heap_swap(heap, parent, child);
}

void opus_heap_insert(opus_heap *heap, void *ele_ptr)
{
	opus_arr_push(heap->data_, ele_ptr);
	heap->last_index++;
	opus_heap_sift_up(heap, heap->last_index);
}

/**
 * @brief pop root
 * @param heap
 * @return pointer of element, notice that this is convenient for releasing memory
 */
void *opus_heap_pop(opus_heap *heap)
{
	opus_heap_swap(heap, 1, 0);
	opus_heap_sift_down(heap, 0);
	return heap->data_;
}

void *opus_heap_top(opus_heap *heap)
{
	return ELE_PTR(heap, 1);
}

/**
 * @brief assume heap's data is now an unsorted array, we build binary heap based on it
 * @param heap
 * @param n size of the array (data)
 */
void opus_heap_update(opus_heap *heap, uint64_t n)
{
	uint64_t i;
	opus_arr_set_len(heap->data_, 0);
	opus_arr_reserve(heap->data_, 1 + n);
	memmove(ELE_PTR(heap, 1), ELE_PTR(heap, 0), n * heap->ele_size);
	for (i = n / 2; i > 0; --i)
		opus_heap_sift_down(heap, i);
	opus_arr_set_len(heap->data_, n + 1);
	heap->last_index = n;
}
