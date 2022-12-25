/**
 * @file render_utils.h
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
#ifndef RENDER_UTILS_H
#define RENDER_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stddef.h>
#include "math/math.h"
#include "render/pluto/plutovg.h"

void painter_line(plutovg_t *vg, real sx, real sy, real ex, real ey);
void painter_path(plutovg_t *vg, vec2 *path, size_t n);
void painter_arrow(plutovg_t *vg, vec2 origin, vec2 dir, real deflection, real len_head, real len_body);
void painter_text(plutovg_t *vg, const char *text, real x, real y);
void painter_textn(plutovg_t *vg, const char *text, int len, real x, real y);
void painter_text_box(plutovg_t *vg, const char *text, int len,
                      real x, real y, real max_width, real max_height, real margin_x, real margin_y,
                      int split_word, real line_spacing);


#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* RENDER_UTILS_H */
