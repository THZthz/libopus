/**
 * @file core_utils.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/10/16
 *
 * @example
 *
 * @development_log
 *
 */

#include <stdarg.h>
#include <stdio.h>

#define SOKOL_TIME_IMPL
#include "utils/utils.h"
#include "external/sokol_time.h"

void opus_info_impl(char *format, ...)
{
	va_list args;
	va_start(args, format);
	fprintf(stdout, "[INFO] ");
	vfprintf(stdout, format, args);
}

void opus_error_impl(char *format, ...)
{
	va_list args;
	va_start(args, format);
	fprintf(stderr, "[ERROR] ");
	vfprintf(stderr, format, args);
}

void opus_warning_impl(char *format, ...)
{
	va_list args;
	va_start(args, format);
	fprintf(stderr, "[WARNING] ");
	vfprintf(stderr, format, args);
}
