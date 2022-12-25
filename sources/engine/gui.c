/**
 * @file engine_gui.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/10/15
 *
 * @example
 *
 * @development_log
 *
 */

#include "engine/gui.h"

#include <stdarg.h>
#include <string.h>

#include "render/color.h"
#include "render/pluto/plutovg.h"
#include "render/render_utils.h"


gui_button_style_t gui_button_style(char *text, real x, real y, real w, real h,
                                    real bw, real fs,
                                    real ml, real mr, real mt, real mb,
                                    real br, real bg, real bb,
                                    real or, real og, real ob,
                                    real tr, real tg, real tb)
{
	gui_button_style_t style = {0};
	strncpy(style.text, text, GUI_MAX_TEXT_SIZE);
	style.x             = x;
	style.y             = y;
	style.w             = w;
	style.h             = h;
	style.bw            = bw;
	style.fs            = fs;
	style.margin_left   = ml;
	style.margin_right  = mr;
	style.margin_top    = mt;
	style.margin_bottom = mb;
	style.br            = br;
	style.bg            = bg;
	style.bb            = bb;
	style.or            = or ;
	style.og            = og;
	style.ob            = ob;
	style.tr            = tr;
	style.tg            = tg;
	style.tb            = tb;

	return style;
}

                 /* return true if this button is clicked */
int gui_button(plutovg_t *vg, gui_button_style_t style)
{
	plutovg_rect(vg, style.x, style.y, style.w, style.h);

	plutovg_set_source_rgb(vg, style.br, style.bg, style.bb);
	plutovg_fill_preserve(vg);
	plutovg_set_source_rgb(vg, style.or, style.og, style.ob);
	plutovg_set_line_width(vg, style.bw);
	plutovg_stroke(vg);

	plutovg_set_source_rgb(vg, style.tr, style.tg, style.tb);
	plutovg_set_font_size(vg, style.fs);
	painter_text_box(vg, style.text, (int) strlen(style.text),
	                 style.x + style.margin_left, style.y + style.margin_top,
	                 style.w - style.margin_left - style.margin_right, style.h - style.margin_top - style.margin_bottom,
	                 0, 0, 0, 0);
	plutovg_fill(vg);

	return 1;
}
