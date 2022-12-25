/**
 * @file painter_funcs.h
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/9/8
 *
 * @brief defines the basic functions of rendering, aimed at more flexible API
 *
 * @example
 *
 * @development_log
 *
 */
#ifndef RENDER_FUNCS_H
#define RENDER_FUNCS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "math/math.h"

typedef void *painter_context_t;
typedef int   painter_fill_rule_t;
typedef int   painter_line_join_t;
typedef int   painter_line_cap_t;

typedef painter_context_t *(*painter_create_context_func)();
typedef void (*painter_destroy_context_func)(painter_context_t *context);

typedef void (*painter_set_line_width_func)(painter_context_t *context, real width);
typedef void (*painter_set_line_join_func)(painter_context_t *context, painter_line_join_t join);
typedef void (*painter_set_line_cap_func)(painter_context_t *context, painter_line_cap_t cap);
typedef void (*painter_set_rgba_func)(painter_context_t *context, real r, real g, real b, real a);
typedef void (*painter_set_rgb_func)(painter_context_t *context, real r, real g, real b);
typedef void (*painter_stroke_func)(painter_context_t *context);
typedef void (*painter_fill_func)(painter_context_t *context);
typedef void (*painter_set_fill_rule_func)(painter_context_t *context, painter_fill_rule_t rule);
typedef void (*painter_circle_func)(painter_context_t *context, real cx, real cy, real r);
typedef void (*painter_arc_func)(painter_context_t *context, real cx, real cy, real r, real a0, real a1, int ccw);
typedef void (*painter_move_to_func)(painter_context_t *context, real x, real y);
typedef void (*painter_line_to_func)(painter_context_t *context, real x, real y);
typedef void (*painter_rect_func)(painter_context_t *context, real x, real y, real w, real h);

typedef void (*painter_rotate_func)(painter_context_t *context, real rotation);
typedef void (*painter_translate_func)(painter_context_t *context, real x, real y);
typedef void (*painter_scale_func)(painter_context_t *context, real x, real y);
typedef void (*painter_restore_func)(painter_context_t *context);
typedef void (*painter_save_func)(painter_context_t *context);

/*typedef void (*painter_text_func)(painter_context_t *context, const char *text, real x, real y);*/

enum {
	painter_line_cap_butt,
	painter_line_cap_round,
	painter_line_cap_square
};

enum {
	painter_line_join_miter,
	painter_line_join_round,
	painter_line_join_bevel
};

enum {
	painter_fill_rule_non_zero,
	painter_fill_rule_even_odd
};


int painter_setup_funcs();


extern painter_create_context_func  painter_create_context;
extern painter_destroy_context_func painter_destroy_context;
extern painter_set_line_width_func  painter_set_line_width;
extern painter_set_line_cap_func    painter_set_line_cap;
extern painter_set_line_join_func   painter_set_line_join;
extern painter_set_rgba_func        painter_set_rgba;
extern painter_set_rgb_func         painter_set_rgb;
extern painter_stroke_func          painter_stroke;
extern painter_fill_func            painter_fill;
extern painter_set_fill_rule_func   painter_set_fill_rule;
extern painter_circle_func          painter_circle;
extern painter_arc_func             painter_arc;
extern painter_move_to_func         painter_move_to;
extern painter_line_to_func         painter_line_to;
extern painter_rect_func            painter_rect;
/*extern painter_text_func            painter_text;*/
extern painter_rotate_func          painter_rotate;
extern painter_translate_func       painter_translate;
extern painter_scale_func           painter_scale;
extern painter_restore_func         painter_restore;
extern painter_save_func            painter_save;


#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* RENDER_FUNCS_H */
