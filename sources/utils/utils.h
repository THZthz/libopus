/**
 * @file core_utils.h
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/8/24
 *
 * @example
 *
 * @development_log
 *
 */
#ifndef CORE_UTILS_H
#define CORE_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef __STDC_VERSION__
#define OPUS_C90
#elif __STDC_VERSION__ == 199901L
#define OPUS_C99
#elif __STDC_VERSION__ == 201112L
#define OPUS_C11
#else
#define OPUS_C_UNKNOWN
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#define OPUS_RETURN_IF(ret, cond) if (cond) return ret
#define OPUS_TO_STRING_(v) OPUS_TO_STRING__(v)
#define OPUS_TO_STRING__(v) #v
#define OPUS_DO_NOTHING_() \
	do {             \
		for (; 0;)   \
			;        \
	} while (0)

/* clang-format off */
#ifdef _NDEBUG
#define OPUS_ASSERT(cond)
#define OPUS_INFO
#define OPUS_ERROR
#define OPUS_WARNING
#define OPUS_NOT_NULL
#else
#define OPUS_ASSERT(cond) if (!(cond)) OPUS_ERROR("assertion ["OPUS_TO_STRING_(cond)"] failed \n\tAT "__FILE__":"OPUS_TO_STRING_(__LINE__)":\n");
#define OPUS_NOT_NULL(v) OPUS_ASSERT(v)
#define OPUS_INFO opus_info_impl
#define OPUS_ERROR opus_error_impl
#define OPUS_WARNING opus_warning_impl
#endif
/* clang-format on */

#ifdef __GNUC__
#define OPUS_LIKELY(x) __builtin_expect(!!(x), 1)
#define OPUS_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define OPUS_UNUSED __attribute__((unused))
#else
#define LIKELY(x) x
#define UNLIKELY(x) x
#define UNUSED
#pragma warning(disable : 4996) /* For fscanf */
#endif /*__GNUC__ */

#ifdef OPUS_C99
#define OPUS_SELECT_FUNC_ARGS_0_3__(x, A, B, C, FUNC, ...) FUNC
#define OPUS_SELECT_FUNC_ARGS_0_3(_0, _1, _2, _3, ...) \
	OPUS_SELECT_FUNC_ARGS_0_3__(, ##__VA_ARGS__,       \
	                       _3(__VA_ARGS__),       \
	                       _2(__VA_ARGS__),       \
	                       _1(__VA_ARGS__),       \
	                       _0(__VA_ARGS__))
#endif

#ifndef OPUS_INLINE
#if defined(__GNUC__)

#if (__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1))
#define OPUS_INLINE __inline__ __attribute__((always_inline))
#else
#define OPUS_INLINE __inline__
#endif /* INLINE */

#elif (defined(_MSC_VER) || defined(__WATCOMC__))
#define OPUS_INLINE __inline
#else
#define OPUS_INLINE
#endif
#endif /* INLINE　*/

void opus_info_impl(char *format, ...);
void opus_error_impl(char *format, ...);
void opus_warning_impl(char *format, ...);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* CORE_UTILS_H */
