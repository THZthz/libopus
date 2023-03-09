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

typedef struct opus_arr_head {
	uint64_t cap, len;
	uint32_t ele_size;
} opus_arr_head;

#define opus_arr_not_null(v) OPUS_ASSERT(v)

#define opus_arr_get_head(_arr) ((opus_arr_head *) ((char *) (_arr) - sizeof(opus_arr_head)))
#define opus_arr_get_body(_header) ((void *) ((char *) (_header) + sizeof(opus_arr_head)))

#define opus_arr_len(_arr) (opus_arr_get_head(_arr)->len)
#define opus_arr_cap(_arr) (opus_arr_get_head(_arr)->cap)
#define opus_arr_ele_size(_arr) (opus_arr_get_head(_arr)->ele_size)
#define opus_arr_set_len(_arr, _len) \
	do {                             \
		opus_arr_len(_arr) = (_len); \
	} while (0)

#define opus_arr_reserve(_arr, _c)                \
	do {                                          \
		(_arr) = opus_arr_reserve_((_arr), (_c)); \
	} while (0)

#define opus_arr_resize(_arr, _size)                                           \
	do {                                                                       \
		opus_arr_head *sh_ = opus_arr_get_head(_arr);                          \
		if (sh_->len < (_size)) {                                              \
			opus_arr_reserve((_arr), (_size) -sh_->len);                       \
			/*memset(&(_arr)[sh->len], 0, sh->ele_size *((_size) -sh->len));*/ \
		}                                                                      \
		opus_arr_set_len((_arr), (_size));                                     \
	} while (0)

#define opus_arr_push(_arr, _ele_ptr)                                                     \
	do {                                                                                  \
		opus_arr_reserve((_arr), 1);                                                      \
		opus_arr_push_((_arr), opus_arr_ele_size(_arr), &opus_arr_len(_arr), (_ele_ptr)); \
	} while (0)

#define opus_arr_push_v(_arr, _ele_type, _val) \
	do {                                       \
		_ele_type _temp = (_val);              \
		opus_arr_push((_arr), &_temp);         \
	} while (0)

#define opus_arr_pop(_arr) opus_arr_pop_((_arr), opus_arr_ele_size(_arr), &opus_arr_len(_arr))

#define opus_arr_insert(_arr, _ele_ptr, _pos)                                                       \
	do {                                                                                            \
		opus_arr_reserve((_arr), 1);                                                                \
		opus_arr_insert_((_arr), opus_arr_ele_size(_arr), &opus_arr_len(_arr), (_ele_ptr), (_pos)); \
	} while (0)

#define opus_arr_insert_v(_arr, _ele_type, _val, _pos) \
	do {                                               \
		_ele_type _temp = (_val);                      \
		opus_arr_insert((_arr), &_temp, (_pos));       \
	} while (0)

#define opus_arr_remove(_arr, _pos)                                                     \
	do {                                                                                \
		opus_arr_remove_((_arr), opus_arr_ele_size(_arr), &opus_arr_len(_arr), (_pos)); \
	} while (0)

#define opus_arr_concat(_a, _b)                                                                   \
	do {                                                                                          \
		opus_arr_reserve((_a), opus_arr_len(_b));                                                 \
		opus_arr_concat_((_a), opus_arr_ele_size(_a), &opus_arr_len(_a), (_b), opus_arr_len(_b)); \
	} while (0)

#define opus_arr_create(_arr, _ele_size)                                                       \
	do {                                                                                       \
		opus_arr_head *ch_ = (opus_arr_head *) malloc(sizeof(opus_arr_head) + (_ele_size) *8); \
		opus_arr_not_null(ch_);                                                                \
                                                                                               \
		ch_->len      = 0;                                                                     \
		ch_->cap      = 8;                                                                     \
		ch_->ele_size = (_ele_size);                                                           \
		(_arr)        = opus_arr_get_body(ch_);                                                \
	} while (0)

#define opus_arr_destroy(_arr)         \
	do {                               \
		if (!(_arr)) break;            \
		free(opus_arr_get_head(_arr)); \
		(_arr) = NULL;                 \
	} while (0)

#define opus_arr_clear(_arr)    \
	do {                        \
		opus_arr_len(_arr) = 0; \
	} while (0)

#define opus_arr_swap(_arr, _i, _j) opus_arr_swap_((_arr), opus_arr_ele_size(_arr), (_i), (_j))

#define opus_arr_reverse(_arr) opus_arr_reverse_((_arr), opus_arr_ele_size(_arr), opus_arr_len(_arr))

#define opus_arr_index(_arr, _ele_ptr) opus_arr_index_((_arr), opus_arr_ele_size(_arr), opus_arr_len(_arr), (_ele_ptr))

opus_arr_head *opus_arr_get_header_(void *arr);
uint64_t       opus_arr_len_(void *arr);
void          *opus_arr_reserve_(void *arr, uint64_t count);
void           opus_arr_push_(void *arr, uint32_t ele_size, uint64_t *len, void *ele_ptr);
void          *opus_arr_pop_(void *arr, uint32_t ele_size, uint64_t *len);
void           opus_arr_insert_(void *arr, uint32_t ele_size, uint64_t *len, void *ele_ptr, uint64_t pos);
void           opus_arr_remove_(void *arr, uint32_t ele_size, uint64_t *len, uint64_t pos);
void           opus_arr_concat_(void *dst, uint32_t ele_size, uint64_t *dst_len, void *src, uint64_t src_len);
void           opus_arr_swap_(void *arr, uint32_t ele_size, uint64_t i, uint64_t j);
void           opus_arr_reverse_(void *arr, uint32_t ele_size, uint64_t len);
uint64_t       opus_arr_index_(void *arr, uint32_t ele_size, uint64_t len, void *ele_ptr);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* ARRAY_H */
