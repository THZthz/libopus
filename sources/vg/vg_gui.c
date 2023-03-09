/**
 * @file vg_gui.c
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

#include <string.h>
#include <stdlib.h>

#include "vg/vg_color.h"
#include "data_structure/array.h"
#include "vg/vg_gui.h"
#include "utils/utils.h"

static void set_region_(vg_gui_region_t *region, opus_real x, opus_real y, opus_real w, opus_real h)
{
	region->x = x;
	region->y = y;
	region->w = w;
	region->h = h;
}

vg_gui_t *vg_gui_create(vg_input_t *input)
{
	char font_file_path[] = "../assets/fonts/VarelaRound-Regular.ttf";

	vg_gui_t *gui;
	OPUS_RETURN_IF(NULL, !input);

	gui = malloc(sizeof(vg_gui_t));
	OPUS_RETURN_IF(NULL, !gui);

	gui->input = input;

	opus_arr_create(gui->components, sizeof(vg_gui_info_panel_t)); /* FIXME */
	gui->font = vg_gl_font_create(font_file_path);
	vg_gl_font_set_size(gui->font, 18);

	return gui;
}

/**
 * @brief
 * @param gui
 * @param vg
 * @param program preset1
 */
void vg_gui_render_components(vg_gui_t *gui, opus_vg *vg, vg_gl_program_t *program)
{
	size_t i;
	for (i = 0; i < opus_arr_len(gui->components); i++) {
		int type = gui->components[i].type;
		switch (type) {
			case 1: /* info panel */
			{
				vg_gui_info_panel_t *panel = (void *) &gui->components[i];
				size_t               str_n = strlen(panel->info);

				vg_gl_font_set_bound(gui->font,
				                     panel->region.w - 2 * panel->left_padding,
				                     panel->region.h - 2 * panel->top_padding);
				if (panel->region.w <= 0 || panel->region.h <= 0) {
					opus_real x, y;
					vg_gl_font_measure_text(gui->font, panel->info, str_n, &x, &y);
					panel->region.w     = x * 12 / 10;
					panel->region.h     = y * 17 / 15 + gui->font->size / 2;
					panel->left_padding = panel->region.w / 10;
					panel->top_padding  = panel->region.h / 15;
				}

				opus_vg_begin(vg);
				opus_vg_rect(vg, panel->region.x, panel->region.y, panel->region.w, panel->region.h);
				opus_vg_end_fill_path(vg);
				vg_gl_p1_render_fill(program, vg, COLOR_WHITE, 1);

				vg->line_width = 2;
				opus_vg_begin(vg);
				opus_vg_rect(vg, panel->region.x, panel->region.y, panel->region.w, panel->region.h);
				opus_vg_end_stroke_path(vg);
				/*vg_gl_p1_render_stroke(program, vg, 65./255,67./255,69./255, 1);*/
				vg_gl_p1_render_stroke(program, vg, COLOR255(67,67,67), 1);

				opus_vg_begin(vg);
				vg_gl_font_set_bound(gui->font,
				                     panel->region.w - 2 * panel->left_padding,
				                     panel->region.h - 2 * panel->top_padding);
				vg_gl_font_generate_path(gui->font, vg, panel->info, str_n,
				                         panel->region.x + panel->left_padding,
				                         panel->region.y + panel->top_padding + gui->font->size);
				vg_gl_font_fill(gui->font, vg, program);
				break;
			}
			default:
				/* fall-through */
			case 0:
				OPUS_ERROR("vg_gui_render_components::unknown component type\n");
		}
	}
}

void vg_gui_destroy(vg_gui_t *gui)
{
	opus_arr_destroy(gui->components);
	free(gui);
}

void vg_gui_add_info_panel(vg_gui_t *gui, opus_real x, opus_real y, opus_real w, opus_real h, const char *info)
{
	vg_gui_info_panel_t ip;

	set_region_(&ip.region, x, y, w, h);
	ip.type       = VG_GUI_INFO_PANEL;
	ip.is_closed  = 0;
	ip.is_visible = 0;
	ip.info       = (char *) info;

	ip.left_padding = w / 10;
	ip.top_padding  = h / 15;

	opus_arr_push(gui->components, &ip);
}
