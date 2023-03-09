/**
 * @file array.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2023/3/6
 *
 * @example
 *
 * @development_log
 *
 */

#include "data_structure/array.h"


OPUS_INLINE opus_arr_head *opus_arr_get_header_(void *arr)
{
	return opus_arr_get_head(arr);
}

OPUS_INLINE uint64_t opus_arr_len_(void *arr)
{
	return opus_arr_len(arr);
}

OPUS_INLINE void *opus_arr_reserve_(void *arr, uint64_t count)
{
	opus_arr_head *h = opus_arr_get_head(arr);
	if (h->len + count > h->cap) {
		/* always allocate spaces of the multiple of 2 */
		while (h->len + count > h->cap) h->cap = h->cap == 0 ? 8 : 2 * h->cap;

		h = (opus_arr_head *) realloc(h, sizeof(opus_arr_head) + h->ele_size * h->cap);
		opus_arr_not_null(h);
	}
	return opus_arr_get_body(h); /* return the new data address in case it changes */
}

/**
 * @brief will not re-alloc array's memory, make sure that array have enough capacity
 * @param arr
 * @param ele_size
 * @param len
 * @param ele_ptr
 */
OPUS_INLINE void opus_arr_push_(void *arr, uint32_t ele_size, uint64_t *len, void *ele_ptr)
{
	memcpy((char *) arr + (*len) * ele_size, ele_ptr, ele_size);
	(*len)++;
}

OPUS_INLINE void *opus_arr_pop_(void *arr, uint32_t ele_size, uint64_t *len)
{
	if (*len == 0) return NULL;
	--(*len);
	return (void *) ((char *) arr + ele_size * *len);
}

OPUS_INLINE void opus_arr_insert_(void *arr, uint32_t ele_size, uint64_t *len, void *ele_ptr, uint64_t pos)
{
	if (OPUS_LIKELY(pos < *len)) {
		memmove((char *) arr + ele_size * (pos + 1), (char *) arr + ele_size * pos, ele_size * (*len - pos));
		memcpy((char *) arr + ele_size * pos, ele_ptr, ele_size);
		(*len)++;
	} else if (OPUS_UNLIKELY(pos == *len)) {
		memcpy((char *) arr + ele_size * pos, ele_ptr, ele_size);
		(*len)++;
	} else {
		OPUS_ERROR("ARRAY::insert::You are inserting an element to a position out of bound");
	}
}

OPUS_INLINE void opus_arr_remove_(void *arr, uint32_t ele_size, uint64_t *len, uint64_t pos)
{
	if (pos < *len) {
		if (OPUS_LIKELY(pos != *len - 1))
			memmove((char *) arr + ele_size * pos, (char *) arr + ele_size * (pos + 1), ele_size * (*len - pos - 1));
		(*len)--;
	} else {
		OPUS_ERROR("ARRAY::remove::Trying to remove an element out of the bound of the array");
	}
}

OPUS_INLINE void opus_arr_concat_(void *dst, uint32_t ele_size, uint64_t *dst_len, void *src, uint64_t src_len)
{
	memcpy((char *) dst + ele_size * *dst_len, src, ele_size * src_len);
	*dst_len += src_len;
}

OPUS_INLINE void opus_arr_swap_(void *arr, uint32_t ele_size, uint64_t i, uint64_t j)
{
	if (OPUS_LIKELY(ele_size) < 512) {
		char temp[512];
		memcpy(temp, (char *) arr + ele_size * i, ele_size);
		memcpy((char *) arr + ele_size * i, (char *) arr + ele_size * j, ele_size);
		memcpy((char *) arr + ele_size * j, temp, ele_size);
	} else {
		OPUS_ERROR("ARRAY::swap::The implementation does not allow swapping of elements of big size");
	}
}

OPUS_INLINE void opus_arr_reverse_(void *arr, uint32_t ele_size, uint64_t len)
{
	uint64_t i = 0, j = len - 1;
	while (i < j) {
		opus_arr_swap(arr, i, j);
		i++;
		j--;
	}
}

OPUS_INLINE uint64_t opus_arr_index_(void *arr, uint32_t ele_size, uint64_t len, void *ele_ptr)
{
	uint64_t i;
	char    *ptr = arr;
	for (i = 0; i < len; ptr += ele_size)
		if (memcmp(ele_ptr, ptr, ele_size) == 0) return i;
	return (uint64_t) -1;
}
