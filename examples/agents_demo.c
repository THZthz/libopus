/**
 * @file agents_demo.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/12/22
 *
 * @example
 *
 * @development_log
 *
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef __EMSCRIPTEN__
#include "external/emscripten/emscripten.h"
#include "external/emscripten/emscripten/html5.h"
#endif /* __EMSCRIPTEN__ */

#include "data_structure/array.h"
#include "data_structure/matrix.h"
#include "engine/engine.h"
#include "game/agents.h"
#include "math/geometry.h"
#include "vg/vg_color.h"
#include "render/pluto/plutovg-private.h"
#include "render/pluto/plutovg.h"
#include "render/render_utils.h"
#include "utils/event.h"

struct parameters {
	plutovg_surface_t *surface;
	plutovg_t         *vg;
	plutovg_font_t    *font;

	agent_t a;
	vec2    path[5];

	unsigned n;
	agent_t  agents[100];
} paras = {NULL, NULL, NULL};

int on_pointer_down(event_hub_t *hub, event_t *e, void *args)
{
	return 0;
}

void run_engine()
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
}

int main()
{
	run_engine();

	return 0;
}

void preload(engine_t *eng)
{
	/* load rendering context */
	eng->data_width                 = eng->width;
	eng->data_height                = eng->height;
	eng->draw_data_to_current_frame = 1; /* draw data to screen */

	paras.surface = plutovg_surface_create(eng->width, eng->height);
	paras.vg      = plutovg_create(paras.surface);

	eng->data = paras.surface->data;

	{ /* load font */
		char *font_file_path = "../assets/fonts/consola.ttf";
#ifdef __EMSCRIPTEN__
		consola_font_file_path = "consola.ttf";
#endif
		paras.font = plutovg_font_load_from_file(font_file_path, 18);
		plutovg_set_font(paras.vg, paras.font);
	}

	{
		unsigned i;
		/*paras.a.max_vel = 999999;
		vec2_set(paras.a.pos, 80, 80);

		paras.path[0] = vec2_(100, 100);
		paras.path[1] = vec2_(150, 110);
		paras.path[2] = vec2_(200, 80);
		paras.path[3] = vec2_(230, 200);
		paras.path[4] = vec2_(100, 200);*/

		srand(time(NULL));

		paras.n = 100;
		for (i = 0; i < paras.n; i++) {
			paras.agents[i].max_vel = 99999;
			paras.agents[i].pos.x   = r_random_lcrc(10, 790);
			paras.agents[i].pos.y   = r_random_lcrc(10, 790);
		}
	}
}

void update(engine_t *eng, double delta)
{
	unsigned i;
	agent_t *other[100];
	for (i = 0; i < paras.n; i++) other[i] = &paras.agents[i];
	for (i = 0; i < paras.n; i++) {
		agent_steering_t steering;
		agent_steering_t s1 = agent_get_flock_steering(&paras.agents[i], other, paras.n, 50);
		agent_steering_t s2 = agent_get_seek_steering(&paras.agents[i], 50, eng->pointer);
		vec2_scale(s2.linear, s2.linear, 0.4);
		vec2_add(steering.linear, s1.linear, s2.linear);
		steering.angular = s1.angular + s2.angular;
		agent_update_state(&paras.agents[i], steering, delta);
	}
}

void render(engine_t *eng)
{
	memset(eng->data, 255, eng->width * eng->height * 4);

	/* draw agents */
	{
		unsigned i;
		for (i = 0; i < paras.n; i++) {
			plutovg_save(paras.vg);
			plutovg_translate(paras.vg, paras.agents[i].pos.x, paras.agents[i].pos.y);
			plutovg_circle(paras.vg, 0, 0, 10);
			plutovg_stroke(paras.vg);
			plutovg_rotate(paras.vg, paras.agents[i].orientation);
			plutovg_move_to(paras.vg, 0, 0);
			plutovg_line_to(paras.vg, 10, 0);
			plutovg_stroke(paras.vg);
			plutovg_restore(paras.vg);
		}
	}

	/* draw path */
	{
		unsigned i;
		plutovg_move_to(paras.vg, paras.path[0].x, paras.path[0].y);
		for (i = 1; i < 5; i++) plutovg_line_to(paras.vg, paras.path[i].x, paras.path[i].y);
		plutovg_stroke(paras.vg);
	}

	/* draw information */
	{
		char info[1024];
		sprintf(info, "pos: (%.0f, %.0f)", paras.a.pos.x, paras.a.pos.y);
		painter_text(paras.vg, info, 10, 10);
		plutovg_fill(paras.vg);
	}
}

void cleanup(engine_t *eng)
{
	plutovg_font_destroy(paras.font);
	plutovg_surface_destroy(paras.surface);
	plutovg_destroy(paras.vg);
}
