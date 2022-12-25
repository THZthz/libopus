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
#define UTILS_C90
#elif __STDC_VERSION__ == 199901L
#define UTILS_C99
#elif __STDC_VERSION__ == 201112L
#define UTILS_C11
#else
#define UTILS_C_UNKNOWN
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#define TO_STRING(v) TO_STRING__(v)
#define TO_STRING__(v) #v
#define DO_NOTHING() \
	do {             \
		for (; 0;)   \
			;        \
	} while (0)

#ifdef ERROR_
#undef ERROR
#endif

/* clang-format off */
#ifdef _NDEBUG
#define ASSERT(cond)
#define INFO
#define ERROR
#define WARNING
#define NOT_NULL
#define VALIDATE(code)
#else
#define ASSERT(C) assert(C)
/*#define ASSERT(cond) if (!(cond)) ERROR("assertion ["TO_STRING(cond)"] failed \n\tAT "__FILE__":"TO_STRING(__LINE__)":\n");*/
#define NOT_NULL(v) ASSERT(v)
#define INFO core_utils_info_implementation
#define ERROR_ core_utils_error_implementation
#define WARNING core_utils_warning_implementation
#define VALIDATE(code) code
#endif
/* clang-format on */

#ifdef __GNUC__
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define UNUSED __attribute__((unused))
#else
#define LIKELY(x) x
#define UNLIKELY(x) x
#define UNUSED
#pragma warning(disable : 4996) /* For fscanf */
#endif /*__GNUC__ */

#ifdef UTILS_C99
#define SELECT_FUNC_ARGS_0_3__(x, A, B, C, FUNC, ...) FUNC
#define SELECT_FUNC_ARGS_0_3(_0, _1, _2, _3, ...) \
	SELECT_FUNC_ARGS_0_3__(, ##__VA_ARGS__,       \
	                       _3(__VA_ARGS__),       \
	                       _2(__VA_ARGS__),       \
	                       _1(__VA_ARGS__),       \
	                       _0(__VA_ARGS__))
#endif

#ifndef INLINE
#if defined(__GNUC__)

#if (__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1))
#define INLINE __inline__ __attribute__((always_inline))
#else
#define INLINE __inline__
#endif /* INLINE */

#elif (defined(_MSC_VER) || defined(__WATCOMC__))
#define INLINE __inline
#else
#define INLINE
#endif
#endif /* INLINE　*/

void core_utils_info_implementation(char *format, ...);
void core_utils_error_implementation(char *format, ...);
void core_utils_warning_implementation(char *format, ...);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* CORE_UTILS_H */
