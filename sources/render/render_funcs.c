/**
 * @file render_funcs.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/9/8
 *
 * @example
 *
 *
 * @development_log
 *
 */

#include "render/render_funcs.h"

#include <stdlib.h>

#include "render/pluto/plutovg.h"


painter_create_context_func  painter_create_context  = NULL;
painter_destroy_context_func painter_destroy_context = NULL;
painter_set_line_width_func  painter_set_line_width  = NULL;
painter_set_line_cap_func    painter_set_line_cap    = NULL;
painter_set_line_join_func   painter_set_line_join   = NULL;
painter_set_rgba_func        painter_set_rgba        = NULL;
painter_set_rgb_func         painter_set_rgb         = NULL;
painter_stroke_func          painter_stroke          = NULL;
painter_fill_func            painter_fill            = NULL;
painter_set_fill_rule_func   painter_set_fill_rule   = NULL;
painter_circle_func          painter_circle          = NULL;
painter_arc_func             painter_arc             = NULL;
painter_move_to_func         painter_move_to         = NULL;
painter_line_to_func         painter_line_to         = NULL;
painter_rect_func            painter_rect            = NULL;
/*painter_text_func            painter_text            = NULL;*/
painter_rotate_func          painter_rotate          = NULL;
painter_translate_func       painter_translate       = NULL;
painter_scale_func           painter_scale           = NULL;
painter_restore_func         painter_restore         = NULL;
painter_save_func            painter_save            = NULL;


int painter_setup_funcs()
{
	/* USE plutovg for now */
	painter_create_context  = (painter_create_context_func) plutovg_create;
	painter_destroy_context = (painter_destroy_context_func) plutovg_destroy;

	painter_set_line_width = (painter_set_line_width_func) plutovg_set_line_width;
	painter_set_line_join  = (painter_set_line_join_func) plutovg_set_line_join;
	painter_set_line_cap   = (painter_set_line_cap_func) plutovg_set_line_cap;

	painter_set_rgb       = (painter_set_rgb_func) plutovg_set_source_rgb;
	painter_set_rgba      = (painter_set_rgba_func) plutovg_set_source_rgba;
	painter_stroke        = (painter_stroke_func) plutovg_stroke;
	painter_fill          = (painter_fill_func) plutovg_fill;
	painter_set_fill_rule = (painter_set_fill_rule_func) plutovg_set_fill_rule;
	painter_circle        = (painter_circle_func) plutovg_circle;
	painter_arc           = (painter_arc_func) plutovg_arc;
	painter_move_to       = (painter_move_to_func) plutovg_move_to;
	painter_line_to       = (painter_line_to_func) plutovg_line_to;
	painter_rect          = (painter_rect_func) plutovg_rect;
	/*painter_text          = (painter_text_func) plutovg_text;*/

	painter_rotate    = (painter_rotate_func) plutovg_rotate;
	painter_translate = (painter_translate_func) plutovg_translate;
	painter_scale     = (painter_scale_func) plutovg_scale;
	painter_restore   = (painter_restore_func) plutovg_restore;
	painter_save      = (painter_save_func) plutovg_save;

	return 1;
}

/*
int x, y;
for (x = 0; x < 10; x++) {
    for (y = 0; y < 10; y++) {
        float rx = 10 + x * 40, ry = 10 + y * 40;
        plutovg_round_rect(vg, rx, ry, 35, 35, 5, 5);
        plutovg_set_fill_rule(vg, 0);
        plutovg_circle(vg, rx + 17.5, ry + 17.5, 10);
        plutovg_set_fill_rule(vg, 1);
        plutovg_fill(vg);
    }
}
 */
