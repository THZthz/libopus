/**
 * @file config.h
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/12/25
 *
 * @example
 *
 * @development_log
 *
 */
#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* the actual data type of "real" */
#define OPUS_CONFIG_REAL double
#define OPUS_CONFIG_REAL_MAX DBL_MAX
#define OPUS_CONFIG_REAL_MIN DBL_MIN
/* epsilon of data type "real" */
#define OPUS_CONFIG_REAL_EPSILON DBL_EPSILON
/* the prefix of all the functions and data types */
#define OPUS_(name) opus_##name

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* CONFIG_H */

