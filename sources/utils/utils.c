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

#include "utils/utils.h"

void core_utils_info_implementation(char *format, ...)
{
	va_list args;
	va_start(args, format);
	fprintf(stdout, "[INFO] ");
	vfprintf(stdout, format, args);
}

void core_utils_error_implementation(char *format, ...)
{
	va_list args;
	va_start(args, format);
	fprintf(stderr, "[ERROR] ");
	vfprintf(stderr, format, args);
}

void core_utils_warning_implementation(char *format, ...)
{
	va_list args;
	va_start(args, format);
	fprintf(stderr, "[WARNING] ");
	vfprintf(stderr, format, args);
}
