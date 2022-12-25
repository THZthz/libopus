/**
 * @file gui_demo.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/11/4
 *
 * @example
 *
 * @development_log
 *
 */

#include <stdio.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include "core/external/emscripten/emscripten.h"
#include "core/external/emscripten/emscripten/html5.h"
#endif /* __EMSCRIPTEN__ */

#include "core/data_structure/array.h"
#include "core/engine/engine.h"
#include "core/math/geometry.h"
#include "core/render/color.h"
#include "core/render/pluto/plutovg-private.h"
#include "core/render/pluto/plutovg.h"
#include "core/render/render_utils.h"
#include "core/utils/event.h"
#include "core/engine/gui.h"

struct parameters {
	plutovg_surface_t *surface;
	plutovg_t         *vg;
	plutovg_font_t    *font;
} paras = {NULL, NULL, NULL};

int on_pointer_down(event_hub_t *hub, event_t *e, void *args)
{
	return 0;
}

int main()
{
	void preload(engine_t * eng);
	void update(engine_t * eng, double delta);
	void render(engine_t * eng);
	void cleanup(engine_t * eng);

	event_cb  l[2];
	engine_t *engine = NULL;
	int       width = 800, height = 800;

#ifdef __EMSCRIPTEN__
	printf("EMSCRIPTEN SPECIFIED WHEN COMPILING\n");
#endif
#ifdef __EMSCRIPTEN_ON_MOBILE__
	printf("EMSCRIPTENON_MOBILE SPECIFIED WHEN COMPILING\n");
#endif

	engine = engine_create(width, height, "sandbox");
	engine_set_preload(engine, preload);
	engine_set_update(engine, update);
	engine_set_render(engine, render);
	engine_set_cleanup(engine, cleanup);
	l[0] = &on_pointer_down;
	event_hub_on(engine->event_hub, engine_event_on_pointer_down, event_create(l, 1, -1, engine));
	engine_start(engine);
	engine_destroy(engine);

	return 0;
}

void preload(engine_t *eng)
{
	/* load rendering context */
	eng->data_width                 = eng->width;
	eng->data_height                = eng->height;
	eng->draw_data_to_current_frame = core_true; /* draw data to screen */

	paras.surface = plutovg_surface_create(eng->width, eng->height);
	paras.vg      = plutovg_create(paras.surface);

	eng->data = paras.surface->data;

	{ /* load font */
		char *consola_font_file_path = "../assets/fonts/consola.ttf";
#ifdef __EMSCRIPTEN__
		consola_font_file_path = "consola.ttf";
#endif
		paras.font = plutovg_font_load_from_file(consola_font_file_path, 18);
		plutovg_set_font(paras.vg, paras.font);
	}
}

void update(engine_t *eng, double delta)
{
}

void render(engine_t *eng)
{
	plutovg_t *vg = paras.vg;
	memset(eng->data, 255, eng->width * eng->height * 4);

	gui_button(vg, gui_button_style("hello world", 100, 100, 100, 30, 2, 19, 2, 0, 2, 0, COLOR_BLACK, COLOR_RED, COLOR_WHITE));
}

void cleanup(engine_t *eng)
{
	plutovg_font_destroy(paras.font);
	plutovg_surface_destroy(paras.surface);
	plutovg_destroy(paras.vg);
}
