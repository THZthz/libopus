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

#include <time.h>

#include "vg/vg_color.h"
#include "vg/vg_engine.h"
#include "vg/vg_input.h"
#include "vg/vg_gl.h"
#include "vg/vg_utils.h"
#include "vg/pluto/plutovg-private.h"
#include "vg/pluto/plutovg.h"
#include "physics/physics.h"
#include "math/geometry.h"
#include "math/polygon/polygon.h"
#include "math/polygon/delaunay.h"
#include "external/sokol_time.h"

vg_engine_t     *g_engine  = NULL;
vg_input_t      *g_input   = NULL;
vg_gl_program_t *g_program = NULL;
vg_gl_font_t    *g_font    = NULL;
vg_gl_program_t *g_pl_r    = NULL;

plutovg_surface_t *g_pl_s = NULL;
plutovg_t         *g_pl   = NULL;

struct paras {
	size_t n;
	opus_vec2 points[400];

	int s;
} paras = {0};

void on_pointer_down(vg_input_t *input)
{
	if (paras.n == 400) return;
	opus_vec2_copy(paras.points[paras.n], input->pointer);
	paras.n++;
}

void on_key_down(vg_input_t *input)
{
	if (input->keys_state[GLFW_KEY_SPACE]) {
		size_t i;
		printf("vec2 points[%d] = {", paras.n);
		for (i = 0; i < paras.n; i++) printf("{%.0f, %.0f}, ", paras.points[i].x, paras.points[i].y);
		printf("};\n");
	} else if (input->keys_state[GLFW_KEY_S]) {
		paras.s = !paras.s;
	}
}

void preload(vg_engine_t *engine)
{
	plutovg_font_t *font;
	font   = plutovg_font_load_from_file("../assets/fonts/georgiaz.ttf", 20);
	g_pl_s = plutovg_surface_create(engine->width, engine->height);
	g_pl   = plutovg_create(g_pl_s);
	g_font = vg_gl_font_create("../assets/fonts/georgiaz.ttf");
	plutovg_set_font(g_pl, font);
}

void update(vg_engine_t *engine, opus_real delta)
{
}

static void point_in_triangle_test()
{
	if (paras.n >= 3) {
		opus_real x, y, r_sq;

		plutovg_move_to(g_pl, paras.points[0].x, paras.points[0].y);
		plutovg_line_to(g_pl, paras.points[1].x, paras.points[1].y);
		plutovg_line_to(g_pl, paras.points[2].x, paras.points[2].y);
		plutovg_close_path(g_pl);
		plutovg_stroke(g_pl);

		opus_triangle_cricumcenter(paras.points[0].x, paras.points[0].y, paras.points[1].x,
		                           paras.points[1].y, paras.points[2].x, paras.points[2].y, &x, &y,
		                           &r_sq);
		plutovg_circle(g_pl, x, y, sqrt(r_sq));
		plutovg_stroke(g_pl);
		plutovg_circle(g_pl, x, y, 4);
		plutovg_fill(g_pl);
	}

	if (paras.n == 4) {
		plutovg_circle(g_pl, paras.points[3].x, paras.points[3].y, 4);
		plutovg_fill(g_pl);

		if (opus_is_point_in_triangle(paras.points[3].x, paras.points[3].y, paras.points[0].x,
		                              paras.points[0].y, paras.points[1].x, paras.points[1].y,
		                              paras.points[2].x, paras.points[2].y)) {
			printf("in triangle\n");
		}
	}

	if (paras.n > 4) {
		paras.n = 0;
	}
}

static void delaunay_test()
{
	uint32_t i, j;

	for (i = 0; i < paras.n; i++) {
		plutovg_circle(g_pl, paras.points[i].x, paras.points[i].y, 3);
	}
	plutovg_fill(g_pl);

	if (paras.n > 3) {
		opus_delaunay_data data = {0};
		opus_real      *flat = malloc(sizeof(opus_real) * paras.n * 2);
		uint32_t        e;

		/* flatten the points array */
		j = 0;
		for (i = 0; i < paras.n; i++) {
			flat[j++] = paras.points[i].x;
			flat[j++] = paras.points[i].y;
		}

		opus_delaunay_init(&data, flat, paras.n * 2);
		opus_delaunay_triangulate(&data);

		for (e = 0; e < data.n_triangles_; e++) {
			if (e < data.half_edges_[e]) {
				opus_vec2 p = paras.points[data.triangles_[e]];
				opus_vec2 q = paras.points[data.triangles_[opus_delaunay_next_half_edge(e)]];
				plutovg_move_to(g_pl, p.x, p.y);
				plutovg_line_to(g_pl, q.x, q.y);
				plutovg_stroke(g_pl);
			}
		}

		opus_delaunay_done(&data);
	}
}

static void gpc_test()
{
	opus_vec2 p1_points[16] = {
	        {219, 216},
	        {401, 121},
	        {510, 143},
	        {571, 275},
	        {590, 453},
	        {485, 689},
	        {460, 724},
	        {279, 728},
	        {178, 670},
	        {166, 590},
	        {286, 547},
	        {346, 548},
	        {432, 498},
	        {483, 362},
	        {422, 302},
	        {296, 296},
	};
	opus_vec2 p2_points[4] = {
	        {161, 178},
	        {538, 169},
	        {535, 507},
	        {51, 472},
	};
	opus_vec2 points[4] = {
	        {254, 614},
	        {419, 626},
	        {213, 406},
	        {484, 407},
	};

	opus_poly_verts      p1_v = {16, p1_points};
	opus_poly_verts      p2_v = {4, points};
	opus_poly            p1   = {1, NULL, &p1_v};
	opus_poly            p2   = {1, NULL, &p2_v};
	opus_poly            p3;

	opus_poly_clip(OPUS_POLY_UNION, &p1, &p2, &p3);

	plutovg_set_line_width(g_pl, 4);

	srand(time(NULL));
	if (paras.s) {
		size_t i;
		for (i = 0; i < p3.num_contours; i++) {
			plutovg_set_source_rgb(g_pl, r_random_01(), r_random_01(), r_random_01());
			vg_pl_path(g_pl, (opus_vec2 *) p3.contour[i].vertex, p3.contour[i].num_vertices);
			plutovg_close_path(g_pl);
			plutovg_stroke(g_pl);
		}
	} else {
		plutovg_set_source_rgb(g_pl, COLOR_BLUE);
		vg_pl_path(g_pl, (opus_vec2 *) p1.contour[0].vertex, p1.contour[0].num_vertices);
		plutovg_close_path(g_pl);
		plutovg_stroke(g_pl);

		plutovg_set_source_rgb(g_pl, COLOR_GREEN);
		vg_pl_path(g_pl, (opus_vec2 *) p2.contour[0].vertex, p2.contour[0].num_vertices);
		plutovg_close_path(g_pl);
		plutovg_stroke(g_pl);
	}


	opus_poly_free(&p3);
}

static void tessellate_polygon_test()
{
	opus_vec2 p1_points[16] = {
	        {219, 216},
	        {401, 121},
	        {510, 143},
	        {571, 275},
	        {590, 453},
	        {485, 689},
	        {460, 724},
	        {279, 728},
	        {178, 670},
	        {166, 590},
	        {286, 547},
	        {346, 548},
	        {432, 498},
	        {483, 362},
	        {422, 302},
	        {296, 296},
	};
	opus_vec2    hole_points[3] = {{193, 96}, {200, 130}, {255, 100}};
	opus_vec2   *hole;
	opus_vec2  **holes;
	opus_vec2   *coords;
	size_t *triangles = NULL, i;
	opus_real    s, e;
	char    text[512];

	/* create an array to contain the contour of the polygon and scale it to nice shape */
	opus_arr_create(coords, sizeof(opus_vec2));
	opus_arr_resize(coords, 16);
	memcpy(coords, p1_points, sizeof(opus_vec2) * 16);
	opus_arr_reverse(coords);
	for (i = 0; i < 16; i++) opus_vec2_scale(coords[i], coords[i], 0.5);

	/* create an array to store the hole */
	opus_arr_create(hole, sizeof(opus_vec2));
	opus_arr_resize(hole, 3);
	memcpy(hole, hole_points, sizeof(opus_vec2) * 3);
	opus_arr_reverse(hole);

	opus_arr_create(holes, sizeof(opus_vec2 *));
	opus_arr_resize(holes, 1);
	holes[0] = hole;

	/* tessellate polygon */
	s         = stm_ms(stm_now());
	triangles = opus_tessellate(&coords, holes);
	e         = stm_ms(stm_now());

	vg_pl_path(g_pl, coords, 16);
	plutovg_close_path(g_pl);
	plutovg_stroke(g_pl);

	plutovg_set_source_rgb(g_pl, COLOR_RED);
	plutovg_circle(g_pl, coords[0].x, coords[0].y, 5);
	plutovg_fill(g_pl);

	plutovg_set_source_rgb(g_pl, COLOR_BLUE);
	plutovg_circle(g_pl, coords[1].x, coords[1].y, 5);
	plutovg_fill(g_pl);

	plutovg_set_source_rgb(g_pl, COLOR_BLACK);


	if (triangles != NULL) {
		plutovg_rect_t rect;
		snprintf(text, sizeof(text), "tessellation completed in %.3fms with %lu triangles in total", e - s, opus_arr_len(triangles) / 3);
		plutovg_text(g_pl, text, 20, 20);
		snprintf(text, sizeof(text), "pointer at (%.0f, %.0f)",  g_input->pointer.x, g_input->pointer.y);
		plutovg_font_get_text_extents(g_pl->state->font, text, &rect);
		plutovg_text(g_pl, text, 20, 20 + rect.h * 1.1);
		plutovg_fill(g_pl);

		for (i = 0; i < opus_arr_len(triangles); i += 3) {
			opus_vec2 t1 = coords[triangles[i]];
			opus_vec2 t2 = coords[triangles[i + 1]];
			opus_vec2 t3 = coords[triangles[i + 2]];
			plutovg_move_to(g_pl, t1.x, t1.y);
			plutovg_line_to(g_pl, t2.x, t2.y);
			plutovg_line_to(g_pl, t3.x, t3.y);
			plutovg_line_to(g_pl, t1.x, t1.y);
		}

		plutovg_stroke_preserve(g_pl);
		plutovg_set_source_rgba(g_pl, COLOR_RED, 0.5);
		plutovg_fill(g_pl);
	}

	opus_arr_destroy(triangles);
	opus_arr_destroy(coords);
	opus_arr_destroy(hole);
	opus_arr_destroy(holes);
}

void render(vg_engine_t *engine)
{
	glClearColor(COLOR_WHITE, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	memset(g_pl_s->data, 255, g_pl_s->width * g_pl_s->height * 4);

	/*point_in_triangle_test();*/
	/*delaunay_test();*/
	/*gpc_test();*/
	tessellate_polygon_test();
	/*vertices_contain_test();*/

	g_pl_r->use(g_pl_r, g_pl_s->data, g_pl_s->width, g_pl_s->height);
}

void cleanup(vg_engine_t *engine)
{
	plutovg_surface_destroy(g_pl_s);
	plutovg_destroy(g_pl);
	vg_gl_font_destroy(g_font);
}

int main()
{
	if (1) {
		/* engine to create glfw window and a combined "vg_t" instance */
		g_engine = vg_engine_create(800, 800, "vg");

		/* handle glfw window input */
		g_input = vg_input_init(g_engine);

		/* specially designed to draw [double, double, ...] like path(tessellate to triangle and draw with opengl) */
		g_program = vg_gl_program_preset1();

		/* specially designed to render pluto_vg surface buffer */
		g_pl_r = vg_gl_program_preset2();

		vg_engine_set(g_engine, preload, update, render, cleanup);
		vg_input_on(g_input->on_pointer_down, on_pointer_down);
		vg_input_on(g_input->on_key_down, on_key_down);
		vg_engine_start(g_engine); /* start engine main loop */

		/* release resources */
		vg_gl_program_destroy(g_program);
		vg_gl_program_destroy(g_pl_r);
		vg_engine_destroy(g_engine);
		vg_input_done();
	} else {
		/* tessellation data test do not mind */
		opus_vec2 p1_points[16] = {
		        {219, 216},
		        {401, 121},
		        {510, 143},
		        {571, 275},
		        {590, 453},
		        {485, 689},
		        {460, 724},
		        {279, 728},
		        {178, 670},
		        {166, 590},
		        {286, 547},
		        {346, 548},
		        {432, 498},
		        {483, 362},
		        {422, 302},
		        {296, 296},
		};
		opus_vec2    hole_points[3] = {{193, 96}, {200, 130}, {255, 100}};
		opus_vec2   *hole;
		opus_vec2  **holes;
		opus_vec2   *coords;
		size_t *triangles, i;


		opus_arr_create(coords, sizeof(opus_vec2));
		opus_arr_resize(coords, 16);
		memcpy(coords, p1_points, sizeof(opus_vec2) * 16);
		opus_arr_reverse(coords);
		for (i = 0; i < 16; i++) opus_vec2_scale(coords[i], coords[i], 0.5);

		opus_arr_create(hole, sizeof(opus_vec2));
		opus_arr_resize(hole, 3);
		memcpy(hole, hole_points, sizeof(opus_vec2) * 3);
		opus_arr_reverse(hole);

		opus_arr_create(holes, sizeof(opus_vec2 *));
		opus_arr_resize(holes, 1);
		holes[0] = hole;

		triangles = NULL;
		triangles = opus_tessellate(&coords, holes);

		opus_arr_destroy(triangles);
		opus_arr_destroy(coords);
		opus_arr_destroy(hole);
		opus_arr_destroy(holes);
	}


	return 0;
}
