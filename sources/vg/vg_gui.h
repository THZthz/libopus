/**
 * @file vg_gui.h
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
#ifndef VG_GUI_H
#define VG_GUI_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "vg/vg_gl.h"
#include "engine/input.h"
#include "math/math.h"
#include "math/geometry.h"

enum {
	VG_GUI_UNKNOWN    = 0,
	VG_GUI_INFO_PANEL = 1
};

typedef struct vg_gui            vg_gui_t;
typedef struct vg_gui_component  vg_gui_component_t;
typedef struct vg_gui_region     vg_gui_region_t;
typedef struct vg_gui_info_panel vg_gui_info_panel_t;


struct vg_gui_region {
	opus_real x, y, w, h;
};

struct vg_gui {
	opus_input *input; /* input handler */
	opus_font  *font;

	vg_gui_component_t *components;
};

struct vg_gui_component {
	int             type;

	vg_gui_region_t region;
};

struct vg_gui_info_panel {
	int             type;

	vg_gui_region_t region;

	int is_visible;
	int is_closed;

	opus_real left_padding, top_padding;

	char *info;
};

vg_gui_t *vg_gui_create(opus_input *input);
void      vg_gui_destroy(vg_gui_t *gui);
void vg_gui_render_components(vg_gui_t *gui, opus_vg *vg, opus_gl_program *program);

void vg_gui_add_info_panel(vg_gui_t *gui, opus_real x, opus_real y, opus_real w, opus_real h, const char *info);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* VG_GUI_H */
