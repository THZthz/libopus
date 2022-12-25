/**
 * @file delaunay_demo.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/10/20
 *
 * @example
 *
 * @development_log
 *
 */

#include <stdio.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include "external/emscripten/emscripten.h"
#include "external/emscripten/emscripten/html5.h"
#endif /* __EMSCRIPTEN__ */

#include "data_structure/array.h"
#include "engine/engine.h"
#include "math/delaunay.h"
#include "math/geometry.h"
#include "physics/physics.h"
#include "render/color.h"
#include "render/pluto/plutovg-private.h"
#include "render/pluto/plutovg.h"
#include "render/render_utils.h"
#include "utils/event.h"

struct parameters {
	plutovg_surface_t *surface;
	plutovg_t         *vg;
	plutovg_font_t    *font;

	vec2 points[100];
	int  n;
} paras = {NULL, NULL, NULL, 0, 0};

int on_pointer_down(event_hub_t *hub, event_t *e, void *args)
{
	engine_t *eng           = args;
	paras.points[paras.n].x = eng->pointer.x;
	paras.points[paras.n].y = eng->pointer.y;
	paras.n++;
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
	int       width = 600, height = 600;

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
	eng->draw_data_to_current_frame = 1; /* draw data to screen */

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

	paras.n = 0;
}

void update(engine_t *eng, double delta)
{
}

static INLINE void point_in_triangle_test()
{
	plutovg_t *vg = paras.vg;

	if (paras.n >= 3) {
		real x, y, r_sq;

		plutovg_move_to(vg, paras.points[0].x, paras.points[0].y);
		plutovg_line_to(vg, paras.points[1].x, paras.points[1].y);
		plutovg_line_to(vg, paras.points[2].x, paras.points[2].y);
		plutovg_close_path(vg);
		plutovg_stroke(vg);

		get_circum_center_of_triangle(paras.points[0].x, paras.points[0].y,
		                              paras.points[1].x, paras.points[1].y,
		                              paras.points[2].x, paras.points[2].y,
		                              &x, &y, &r_sq);
		plutovg_circle(vg, x, y, sqrt(r_sq));
		plutovg_stroke(vg);
		plutovg_circle(vg, x, y, 4);
		plutovg_fill(vg);
	}

	if (paras.n == 4) {
		plutovg_circle(vg, paras.points[3].x, paras.points[3].y, 4);
		plutovg_fill(vg);

		if (is_point_in_triangle(paras.points[3].x, paras.points[3].y,
		                         paras.points[0].x, paras.points[0].y,
		                         paras.points[1].x, paras.points[1].y,
		                         paras.points[2].x, paras.points[2].y)) {
			printf("in triangle\n");
		}
	}

	if (paras.n > 4) {
		paras.n = 0;
	}
}

static INLINE void delaunay_test()
{
	static int printed_data = 0;

	plutovg_t *vg = paras.vg;
	uint32_t   i, j;

	for (i = 0; i < paras.n; i++) {
		plutovg_circle(vg, paras.points[i].x, paras.points[i].y, 3);
	}
	plutovg_fill(vg);

	if (paras.n > 3) {
		delaunay_data_t data = {0};
		real           *flat = malloc(sizeof(real) * paras.n * 2);
		uint32_t        e;

		/* flatten the points array */
		j = 0;
		for (i = 0; i < paras.n; i++) {
			flat[j++] = paras.points[i].x;
			flat[j++] = paras.points[i].y;
		}

		delaunay_init(&data, flat, paras.n * 2);
		delaunay_triangulate(&data);

		for (e = 0; e < data.n_triangles_; e++) {
			if (e < data.half_edges_[e]) {
				vec2 p = paras.points[data.triangles_[e]];
				vec2 q = paras.points[data.triangles_[delaunay_next_half_edge(e)]];
				plutovg_move_to(vg, p.x, p.y);
				plutovg_line_to(vg, q.x, q.y);
				plutovg_stroke(vg);
			}
		}

		/*if (paras.n > 30 && !printed_data) {
		    printf("ve2 points[%d] = {", paras.n);
		    for (i = 0; i < paras.n; i++) printf("{%f, %f}, ", paras.points[i].x, paras.points[i].y);
		    printf("};\n");
		    printed_data = 1;
		}*/

		delaunay_done(&data);
	}
}

static INLINE void vertices_contain_test()
{
	plutovg_t *vg = paras.vg;

	if (paras.n >= 3) {
		uint32_t i;
		painter_path(vg, paras.points, paras.n);
		plutovg_close_path(vg);
		plutovg_set_source_rgb(vg, COLOR_BLACK);
		plutovg_stroke(vg);

		for (i = 0; i < paras.n; i++) {
			char text[512];
			snprintf(text, 512, "%u", i);
			plutovg_text(vg, text, paras.points[i].x, paras.points[i].y);
		}
		plutovg_fill(vg);
	}

	if (paras.n >= 5) {
		vertex_t *vertices = vertices_create(paras.points, paras.n - 1, NULL);
		vertices_sort_clockwise(vertices, array_len(vertices), sizeof(vertex_t));
		if (vertices_contains(vertices, array_len(vertices), paras.points[paras.n - 1], sizeof(vertex_t))) {
			printf("sda\n");
		}
	}
	if (paras.n >= 3) {
		uint32_t i;

		/* use vertices_sort_clockwise */
		vertex_t *vertices = vertices_create(paras.points, paras.n, NULL);
		vec2      center;
		vertices_center(vertices, paras.n, &center, sizeof(vertex_t));

		plutovg_circle(vg, center.x, center.y, 3);
		plutovg_fill(vg);

		vertices_sort_clockwise(vertices, paras.n, sizeof(vertex_t));
		for (i = 0; i < paras.n; i++) {
			char text[512];
			snprintf(text, 512, "%u", i);
			plutovg_text(vg, text, vertices[i].x + 10, vertices[i].y);
		}
		plutovg_set_source_rgb(vg, COLOR_RED);
		plutovg_fill(vg);
	}
}

void render(engine_t *eng)
{
	plutovg_t *vg = paras.vg;
	memset(eng->data, 255, eng->width * eng->height * 4);

	/*point_in_triangle_test();*/
	delaunay_test();
	/*vertices_contain_test();*/
}

void cleanup(engine_t *eng)
{
	plutovg_font_destroy(paras.font);
	plutovg_surface_destroy(paras.surface);
	plutovg_destroy(paras.vg);
}
