/**
 * @file vg_utils.h
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/12/29
 *
 * @example
 *
 * @development_log
 *
 */
#ifndef VG_UTILS_H
#define VG_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stddef.h>
#include "math/math.h"
#include "vg/pluto/plutovg.h"

void vg_pl_line(plutovg_t *vg, opus_real sx, opus_real sy, opus_real ex, opus_real ey);
void vg_pl_line_vec(plutovg_t *vg, opus_vec2 s, opus_vec2 e);
void vg_pl_path(plutovg_t *vg, opus_vec2 *path, size_t n);
void vg_pl_arrow(plutovg_t *vg, opus_vec2 origin, opus_vec2 dir, opus_real deflection,
                 opus_real len_head, opus_real len_body);
void vg_pl_text(plutovg_t *vg, const char *text, opus_real x, opus_real y);
void vg_pl_text_n(plutovg_t *vg, const char *text, int len, opus_real x, opus_real y);
void vg_pl_text_box(plutovg_t *vg, const char *text, int len, opus_real x, opus_real y,
                    opus_real max_width, opus_real max_height, opus_real margin_x,
                    opus_real margin_y,
                      int split_word, opus_real line_spacing);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* VG_UTILS_H */

