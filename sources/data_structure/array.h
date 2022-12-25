/**
 * @file array.h
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/8/7
 *
 * @breif A dynamic array macro helper
 *
 * @development_log
 *
 */
#ifndef ARRAY_H
#define ARRAY_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "utils/utils.h"

typedef struct array_header {
	uint64_t cap, len;
	uint32_t ele_size;
} array_header_t;

#define array_not_null(v) ASSERT(v)

#define array_get_header(_arr) ((array_header_t *) ((char *) (_arr) - sizeof(array_header_t)))
#define array_get_body(_header) ((void *) ((char *) (_header) + sizeof(array_header_t)))

#define array_len(_arr) (array_get_header(_arr)->len)
#define array_cap(_arr) (array_get_header(_arr)->cap)
#define array_ele_size(_arr) (array_get_header(_arr)->ele_size)
#define array_set_len(_arr, _len) \
	do {                          \
		array_len(_arr) = (_len); \
	} while (0)

#define array_reserve(_arr, _c)                \
	do {                                       \
		(_arr) = array_reserve_((_arr), (_c)); \
	} while (0)
static INLINE void *array_reserve_(void *arr, uint64_t count)
{
	array_header_t *h = array_get_header(arr);
	if (h->len + count > h->cap) {
		/* always allocate spaces of the multiple of 2 */
		while (h->len + count > h->cap) h->cap = h->cap == 0 ? 8 : 2 * h->cap;

		h = (array_header_t *) realloc(h, sizeof(array_header_t) + h->ele_size * h->cap);
		array_not_null(h);
	}
	return array_get_body(h); /* return the new data address in case it changes */
}

#define array_resize(_arr, _size)                                              \
	do {                                                                       \
		array_header_t *sh_ = array_get_header(_arr);                          \
		if (sh_->len < (_size)) {                                              \
			array_reserve((_arr), (_size) -sh_->len);                          \
			/*memset(&(_arr)[sh->len], 0, sh->ele_size *((_size) -sh->len));*/ \
		}                                                                      \
		array_len(_arr) = (_size);                                                  \
	} while (0)

static INLINE void array_push_(void *arr, uint32_t ele_size, uint64_t *len, void *ele_ptr)
{
	memcpy((char *) arr + (*len) * ele_size, ele_ptr, ele_size);
	(*len)++;
}
#define array_push(_arr, _ele_ptr)                                              \
	do {                                                                        \
		array_reserve((_arr), 1);                                               \
		/*array_push_((_arr), array_ele_size(_arr), &array_len(_arr), (_ele_ptr));*/  \
        memcpy((char *) (_arr) + array_ele_size(_arr) * array_len(_arr), (_ele_ptr), array_ele_size(_arr)); \
array_len(_arr)++;\
	} while (0)
#define array_push_val(_arr, _ele_type, _val) \
	do {                                      \
		_ele_type _temp = (_val);             \
		array_push((_arr), &_temp);           \
	} while (0)

static INLINE void *array_pop_(void *arr, uint32_t ele_size, uint64_t *len)
{
	return (void *) ((char *) arr + ele_size * (--*len));
}
#define array_pop(_arr) array_pop_((_arr), array_ele_size(_arr), &array_len(_arr))

static INLINE void array_insert_(void *arr, uint32_t ele_size, uint64_t *len, void *ele_ptr, uint64_t pos)
{
	if (LIKELY(pos < *len)) {
		memmove((char *) arr + ele_size * (pos + 1), (char *) arr + ele_size * pos, ele_size * (*len - pos));
		memcpy((char *) arr + ele_size * pos, ele_ptr, ele_size);
		(*len)++;
	} else if (UNLIKELY(pos == *len)) {
		memcpy((char *) arr + ele_size * pos, ele_ptr, ele_size);
		(*len)++;
	} else {
		perror("ARRAY::insert::You are inserting an element to a position out of bound");
		exit(-1); /* operation not permitted */
	}
}
#define array_insert(_arr, _ele_ptr, _pos)                                                 \
	do {                                                                                   \
		array_reserve((_arr), 1);                                                          \
		array_insert_((_arr), array_ele_size(_arr), &array_len(_arr), (_ele_ptr), (_pos)); \
	} while (0)
#define array_insert_val(_arr, _ele_type, _val, _pos) \
	do {                                              \
		_ele_type _temp = (_val);                     \
		array_insert((_arr), &_temp, (_pos));         \
	} while (0)

static INLINE void array_remove_(void *arr, uint32_t ele_size, uint64_t *len, uint64_t pos)
{
	if (pos < *len) {
		if (LIKELY(pos != *len - 1))
			memmove((char *) arr + ele_size * pos, (char *) arr + ele_size * (pos + 1), ele_size * (*len - pos - 1));
		(*len)--;
	} else {
		perror("ARRAY::remove::Trying to remove an element out of the bound of the array");
		exit(-1);
	}
}
#define array_remove(_arr, _pos)                                               \
	do {                                                                       \
		array_remove_((_arr), array_ele_size(_arr), &array_len(_arr), (_pos)); \
	} while (0)

static INLINE void array_concat_(void *dst, uint32_t ele_size, uint64_t *dst_len, void *src, uint64_t src_len)
{
	memcpy((char *) dst + ele_size * *dst_len, src, ele_size * src_len);
	*dst_len += src_len;
}
#define array_concat(_a, _b)                                                          \
	do {                                                                              \
		array_reserve((_a), array_len(_b));                                           \
		array_concat_((_a), array_ele_size(_a), &array_len(_a), (_b), array_len(_b)); \
	} while (0)

#define array_create(_arr, _ele_size)                                                             \
	do {                                                                                          \
		array_header_t *ch_ = (array_header_t *) malloc(sizeof(array_header_t) + (_ele_size) *8); \
		array_not_null(ch_);                                                                      \
                                                                                                  \
		ch_->len      = 0;                                                                        \
		ch_->cap      = 8;                                                                        \
		ch_->ele_size = (_ele_size);                                                              \
		(_arr)        = array_get_body(ch_);                                                      \
	} while (0)

#define array_destroy(_arr)           \
	do {                              \
		free(array_get_header(_arr)); \
		(_arr) = NULL;                \
	} while (0)

#define array_clear(_arr)    \
	do {                     \
		array_len(_arr) = 0; \
	} while (0)

#define array_foreach(_arr, _ele_type, _ptr_name)                                        \
	do {                                                                                 \
		uint32_t   _ele_size = array_ele_size(_arr);                                     \
		_ele_type *_ptr_name = (_arr);                                                   \
		_ele_type *_arr_end  = (void *) ((char *) (_arr) + _ele_size * array_len(_arr)); \
		for (; (_ptr_name) < _arr_end; (_ptr_name) = (void *) ((char *) (_ptr_name) + _ele_size)) {

#define array_foreach_end() \
	}                       \
	}                       \
	while (0)               \
		;

static INLINE void array_swap(void *arr, uint32_t ele_size, uint64_t i, uint64_t j)
{
	if (LIKELY(ele_size) < 512) {
		char temp[512];
		memcpy(temp, (char *) arr + ele_size * i, ele_size);
		memcpy((char *) arr + ele_size * i, (char *) arr + ele_size * j, ele_size);
		memcpy((char *) arr + ele_size * j, temp, ele_size);
	} else {
		perror("ARRAY::swap::The implementation does not allow swapping of elements of big size");
		exit(-1); /* operation not permitted */
	}
}

static INLINE void array_reverse_(void *arr, uint32_t ele_size, uint64_t len)
{
	uint64_t i = 0, j = len - 1;
	while (i < j) {
		array_swap(arr, ele_size, i, j);
		i++;
		j--;
	}
}
#define array_reverse(_arr) array_reverse_((_arr), array_ele_size(_arr), array_len(_arr))

static INLINE uint64_t array_index_(void *arr, uint32_t ele_size, uint64_t len, void *ele_ptr)
{
	uint64_t i;
	char    *ptr = arr;
	for (i = 0; i < len; ptr += ele_size)
		if (memcmp(ele_ptr, ptr, ele_size) == 0) return i;
	return (uint64_t) -1;
}
#define array_index(_arr, _ele_ptr) array_index_((_arr), array_ele_size(_arr), array_len(_arr), (_ele_ptr))

static uint64_t array_len_(void *arr)
{
	return array_len(arr);
}

static array_header_t *array_get_header_(void *arr)
{
	return array_get_header(arr);
}

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* ARRAY_H */
