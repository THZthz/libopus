
#include <stdio.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include "core/external/emscripten/emscripten.h"
#include "core/external/emscripten/emscripten/html5.h"
#endif /* __EMSCRIPTEN__ */

#include "core/data_structure/array.h"
#include "core/data_structure/heap.h"
#include "core/engine/engine.h"
#include "core/math/geometry.h"
#include "core/pathfinding/graph.h"
#include "core/pathfinding/pathfinder.h"
#include "core/render/color.h"
#include "core/render/pluto/plutovg-private.h"
#include "core/render/pluto/plutovg.h"
#include "core/render/render_utils.h"
#include "core/utils/event.h"

struct parameters {
	engine_t *engine;

	plutovg_surface_t *surface;
	plutovg_t         *vg;
	plutovg_font_t    *font;

	graph_t      *graph;
	pathfinder_t *finder;
} paras = {NULL, NULL, NULL};

int cmp(heap_t *h, void *a, void *b)
{
	return *(int *) a - *(int *) b;
}

void p(int *arr, int n)
{
	int i = 0;
	while (i < n) {
		printf("%d ", arr[i]);
		i++;
	}
	printf("\n");
}

int main()
{
	void run_engine();
	void test_bheap();

	/*test_bheap();*/
	run_engine();

	return 0;
}

void test_bheap()
{
	int mode = 1;

	if (mode == 0){
		heap_t h = {
		        NULL, NULL, 10, 0, sizeof(int), cmp};
		int *ptr;
		int  e[10]   = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
		int  arr[10] = {0};

		h.data_ = arr;

		heap_push(&h, &e[4]);
		heap_push(&h, &e[3]);
		heap_push(&h, &e[5]);
		heap_push(&h, &e[7]);
		heap_push(&h, &e[6]);
		p(arr, 10);

		ptr = heap_pop(&h);
		printf("%d\n", *ptr);
		p(arr, 10);

		heap_push(&h, &e[1]);
		heap_push(&h, &e[0]);
		heap_push(&h, &e[8]);
		heap_push(&h, &e[2]);
		heap_push(&h, &e[9]);
		p(arr, 10);

		printf("remove 2\n");
		heap_remove(&h, 2);
		p(arr, 10);

		printf("remove 4\n");
		heap_remove(&h, 4);
		p(arr, 10);
	} else if (mode == 1) {
		int  e[10]   = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
		int arr[10] = {0};/*{9, 10, 7, 4, 8, 6, 3, 5, 2, 1};*/
		heap_t h = {
		        arr, NULL, 10, 0, sizeof(int), cmp};

		/*p(arr, 10);*/
		/*heap_build(&h);*/
		/*p(arr, 10);*/
		heap_push(&h, &e[2]);
		heap_push(&h, &e[1]);
		heap_push(&h, &e[5]);
		heap_push(&h, &e[4]);
		heap_push(&h, &e[3]);
		heap_push(&h, &e[0]);
		p(arr, 10);

		heap_push(&h, &e[9]);
		heap_push(&h, &e[6]);
		heap_push(&h, &e[7]);
		heap_push(&h, &e[8]);
		p(arr, 10);

		printf("remove 2\n");
		heap_remove(&h, 2);
		p(arr, 10);
	}
}

void run_engine()
{
	void preload(engine_t * eng);
	void update(engine_t * eng, double delta);
	void render(engine_t * eng);
	void cleanup(engine_t * eng);

	int on_pointer_down(event_hub_t * hub, event_t * e, void *args);

	event_cb  l[2];
	engine_t *engine = NULL;
	int       width = 800, height = 800;

#ifdef __EMSCRIPTEN__
	printf("EMSCRIPTEN SPECIFIED WHEN COMPILING\n");
#endif
#ifdef __EMSCRIPTEN_ON_MOBILE__
	printf("EMSCRIPTENON_MOBILE SPECIFIED WHEN COMPILING\n");
#endif

	engine       = engine_create(width, height, "sandbox");
	paras.engine = engine;
	engine_set_preload(engine, preload);
	engine_set_update(engine, update);
	engine_set_render(engine, render);
	engine_set_cleanup(engine, cleanup);
	l[0] = &on_pointer_down;
	event_hub_on(engine->event_hub, engine_event_on_pointer_down, event_create(l, 1, -1, engine));
	engine_start(engine);
	engine_destroy(engine);
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

	/* load font */
	{
		char *consola_font_file_path = "../assets/fonts/consola.ttf";
#ifdef __EMSCRIPTEN__
		consola_font_file_path = "consola.ttf";
#endif
		paras.font = plutovg_font_load_from_file(consola_font_file_path, 18);
		plutovg_set_font(paras.vg, paras.font);
	}

	/* initialize pathfinding context */
	{
		char map[10][10] = {
		        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		        {0, 1, 0, 0, 0, 1, 0, 1, 0, 0},
		        {0, 1, 1, 0, 0, 1, 0, 1, 0, 0},
		        {0, 0, 1, 0, 0, 1, 0, 1, 0, 1},
		        {0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
		        {0, 1, 1, 1, 1, 1, 0, 1, 1, 0},
		        {0, 0, 1, 0, 0, 0, 0, 1, 0, 0},
		        {1, 1, 1, 0, 0, 0, 0, 1, 0, 1},
		        {0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
		        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		};
		paras.graph  = graph_grid_create(1, 0, 100, 100, 18, 18, 10, 10);
		paras.finder = pathfinder_d_star_lite_create(paras.graph, 10, 10);

		graph_grid_map(paras.graph, (char **) map);
		graph_grid_print_map01(paras.graph);
	}
}

void update(engine_t *eng, double delta)
{
}

void fill_rect(double x, double y, double w, double h, double r, double g, double b)
{
	plutovg_rect(paras.vg, x, y, w, h);
	plutovg_set_source_rgb(paras.vg, r, g, b);
	plutovg_fill(paras.vg);
}

void render(engine_t *eng)
{
	double ox = graph_grid_get_x(paras.graph);
	double oy = graph_grid_get_y(paras.graph);
	double tw = graph_grid_get_tile_width(paras.graph);
	double th = graph_grid_get_tile_height(paras.graph);

	memset(eng->data, 255, eng->width * eng->height * 4);

	/* render grid map */
	{
		graph_count_t ix, iy;

		for (iy = 0; iy < graph_grid_get_height(paras.graph); iy++) {
			for (ix = 0; ix < graph_grid_get_width(paras.graph); ix++) {
				graph_id_t id;
				char       w;
				graph_xy2id(ix, iy, graph_grid_get_width(paras.graph), &id);
				w = (char) paras.graph->get_arc_weight_by_id(paras.graph, id, 0);
				plutovg_rect(paras.vg, ox + ix * tw, oy + iy * th, tw, th);
				if (w == 1) {
					plutovg_set_source_rgb(paras.vg, COLOR_BLACK);
				} else if (w == 0) {
					plutovg_set_source_rgb(paras.vg, COLOR_GRAY81);
				} else {
					exit(114);
				}
				plutovg_fill(paras.vg);
			}
		}
	}

	/* render path */
	{
		graph_id_t   *path = NULL;
		graph_count_t n    = 0, i;

		pathfinder_search(paras.finder, 0, 99);

		/* start and end position */
		fill_rect(ox, oy, tw, th, COLOR_RED);
		fill_rect(ox + 9 * tw, oy + 9 * th, tw, th, COLOR_GREEN2);

		paras.finder->get_path(paras.finder, &path, &n);
		for (i = 0; i < n; i++) {
			graph_id_t x, y;
			graph_id2xy(path[i], graph_grid_get_width(paras.graph), &x, &y);
			fill_rect(ox + x * tw + tw / 4, oy + y * th + th / 4, tw / 2, th / 2, COLOR_ALICE_BLUE);
		}
		if (path != NULL) free(path);
	}
}

void cleanup(engine_t *eng)
{
	plutovg_font_destroy(paras.font);
	plutovg_surface_destroy(paras.surface);
	plutovg_destroy(paras.vg);

	graph_destroy(paras.graph);
	pathfinder_destroy(paras.finder);
}

int on_pointer_down(event_hub_t *hub, event_t *e, void *args)
{
	double ox = graph_grid_get_x(paras.graph);
	double oy = graph_grid_get_y(paras.graph);
	double tw = graph_grid_get_tile_width(paras.graph);
	double th = graph_grid_get_tile_height(paras.graph);

	graph_count_t w = graph_grid_get_width(paras.graph);
	graph_count_t h = graph_grid_get_height(paras.graph);

	double px = paras.engine->pointer.x;
	double py = paras.engine->pointer.y;

	graph_id_t x = floor((px - ox) / tw);
	graph_id_t y = floor((py - oy) / th);

	graph_id_t id;

	char c;

	if (px < ox || py < oy) return 0;
	if (x >= w || y >= h) return 0;
	graph_xy2id(x, y, graph_grid_get_width(paras.graph), &id);

	c = paras.graph->get_arc_weight_by_id(paras.graph, id, 0);
	paras.graph->set_arc_weight_by_id(paras.graph, id, 0, c ? 1 : 0);

	/*printf("changed:\n");
	graph_grid_print_map01(paras.graph);*/

	/*pathfinder_d_star_mark_dirty(paras.finder);*/

	return 0;
}
