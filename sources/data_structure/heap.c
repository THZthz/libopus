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
#include "utils/utils.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define ELE_AT(heap, i) ((void *) ((char *) (heap)->data_ + (heap)->ele_size * (i)))
#define PARENT(id) (((id) -1) >> 1)
#define LEFT_CHILD(id) (2 * (id) + 1)

/**
 * @brief (adapted from python Lib/heapq.py)
 * @param heap a heap at all indices >= startpos, except possibly for pos
 * @param start_pos
 * @param pos the index of a leaf with a possibly out-of-order value
 * @return 0 if succeed
 */
int heap_sift_down(heap_t *heap, size_t start_pos, size_t pos)
{
	void *new_ele;

	new_ele = malloc(heap->ele_size);
	if (!new_ele) return 1;

	memcpy(new_ele, ELE_AT(heap, pos), heap->ele_size);

	/* follow the path to the root, moving parents down until finding a place "new_ele" fits. */
	while (pos > start_pos) {
		size_t parent_pos = PARENT(pos);
		void  *parent     = ELE_AT(heap, parent_pos);
		assert(pos >= 1);
		if (heap->compare_(heap, new_ele, parent) < 0) {
			memcpy(ELE_AT(heap, pos), parent, heap->ele_size);
			pos = parent_pos;
			continue;
		}
		break;
	}
	memcpy(ELE_AT(heap, pos), new_ele, heap->ele_size);
	free(new_ele);

	return 0;
}

/**
 * @brief (adapted from python Lib/heapq.py)
 * @param heap
 * @param pos
 * @return
 */
int heap_sift_up(heap_t *heap, size_t pos)
{
	size_t start_pos = pos;
	size_t end_pos   = heap->n_used;
	size_t child_pos;
	void  *new_ele;

	new_ele = malloc(heap->ele_size);
	if (!new_ele) return 1; /* no enough space */

	memcpy(new_ele, ELE_AT(heap, pos), heap->ele_size);

	/* bubble up the smaller child until hitting a leaf */
	child_pos = LEFT_CHILD(start_pos);

	while (child_pos < end_pos) {
		/* set child_pos to the index of smaller child */
		if (child_pos + 1 < end_pos && heap->compare_(heap, ELE_AT(heap, child_pos + 1), ELE_AT(heap, child_pos)) < 0) {
			child_pos++;
		}

		/* bubble the smaller child up */
		memcpy(ELE_AT(heap, pos), ELE_AT(heap, child_pos), heap->ele_size);

		pos       = child_pos;
		child_pos = LEFT_CHILD(pos);
	}

	/* the child at "pos" is empty now, so put "new_ele" here and bubble it up to its final resting place (by sifting its parents down) */
	memcpy(ELE_AT(heap, pos), new_ele, heap->ele_size);
	heap_sift_down(heap, start_pos, pos);

	free(new_ele);

	return 0;
}

/**
 * @brief (adapted from python Lib/heapq.py)
 * @param heap
 */
void heap_build(heap_t *heap)
{
	/* python Lib/heapq.py version: */
	size_t n = heap->n_used, i = n / 2 + 1;
	while (i-- > 0) heap_sift_up(heap, i);
}

void *heap_top(heap_t *heap)
{
	if (heap->n_used == 0) return NULL;
	return heap->data_;
}

void *heap_push2(heap_t *heap, void *ele)
{
	if (heap->n_used == heap->capacity) return NULL; /* the heap is full already */
	memcpy(ELE_AT(heap, heap->n_used), ele, heap->ele_size);
	heap_sift_down(heap, 0, heap->n_used);
	heap->n_used++;
	return ele;
}

void *heap_remove2(heap_t *heap, size_t pos)
{
	void *tmp;

	tmp = malloc(heap->ele_size);
	if (!tmp) return NULL;
	memcpy(tmp, ELE_AT(heap, pos), heap->ele_size);
	memcpy(ELE_AT(heap, pos), ELE_AT(heap, heap->n_used - 1), heap->ele_size);
	memcpy(ELE_AT(heap, heap->n_used - 1), tmp, heap->ele_size);
	free(tmp);
	heap->n_used--;
	heap_sift_up(heap, 0);
	return ELE_AT(heap, heap->n_used);
}

void *heap_pop2(heap_t *heap)
{
	return heap_remove2(heap, 0);
}

void *heap_remove(heap_t *heap, size_t idx)
{
	size_t c_idx;
	void  *temp, *ret;

	/* cannot remove any element */
	if (heap->n_used == 0) return NULL;
	if (idx >= heap->n_used) return NULL;

	/* if there only exist one element in the heap */
	if (idx == 0 && heap->n_used == 1) {
		heap->n_used--;
		return heap->data_;
	}

	temp = malloc(heap->ele_size);
	if (OPUS_UNLIKELY(!temp)) return NULL; /* unlikely */

	/* exchange current element and the last element */
	ret = ELE_AT(heap, heap->n_used - 1);
	memcpy(temp, ELE_AT(heap, idx), heap->ele_size);
	memcpy(ELE_AT(heap, idx), ret, heap->ele_size);
	memcpy(ret, temp, heap->ele_size);

	/* sink down */
	c_idx = LEFT_CHILD(idx);
	memcpy(temp, ELE_AT(heap, idx), heap->ele_size);
	heap->n_used--;
	while (c_idx < heap->n_used) {
		/* if right child is smaller */
		if (c_idx + 1 < heap->n_used && heap->compare_(heap, ELE_AT(heap, c_idx + 1), ELE_AT(heap, c_idx)) < 0) {
			c_idx++;
		}
		if (heap->compare_(heap, temp, ELE_AT(heap, c_idx)) <= 0) break;

		memcpy(ELE_AT(heap, idx), ELE_AT(heap, c_idx), heap->ele_size);
		idx   = c_idx;
		c_idx = LEFT_CHILD(idx);
	}
	memcpy(ELE_AT(heap, idx), temp, heap->ele_size);
	free(temp);

	return ret;
}

void *heap_push(heap_t *heap, void *ele)
{
	size_t p_idx, idx;

	/* the heap is full */
	if (heap->n_used == heap->capacity) return NULL;

	/* sift up */
	idx = heap->n_used;
	for (;;) {
		if (idx == 0) break; /* reach the root, which has no parents */

		p_idx = PARENT(idx);
		if (heap->compare_(heap, ele, ELE_AT(heap, p_idx)) >= 0) break;

		memcpy(ELE_AT(heap, idx), ELE_AT(heap, p_idx), heap->ele_size);
		idx = p_idx;
	}
	memcpy(ELE_AT(heap, idx), ele, heap->ele_size);
	heap->n_used++;

	return ele;
}

void *heap_pop(heap_t *heap)
{
	return heap_remove(heap, 0);
}
