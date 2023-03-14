#include "vg/vg_color.h"
#include "engine/engine.h"
#include "engine/input.h"
#include "vg/vg_utils.h"
#include "vg/vg_gl.h"
#include "vg/pluto/plutovg-private.h"
#include "vg/pluto/plutovg.h"
#include "math/polygon/polygon.h"
#include "physics/matter/algorithm/bvh.h"
#include "physics/matter/physics.h"

struct param {
	/********************  VG_ENGINE  **************************/
	opus_engine     *engine;
	opus_input      *input;
	opus_gl_program *program_gl, *program_pl;
	opus_font       *font_gl;
	plutovg_t       *pl;

	/******************  PHYSICS_ENGINE  *********************/
	physics_engine_t *phy_engine;

	int         enable_physics, enable_sleeping;
	body_t    **stacks, *walls[4], *b1, *b2;
	opus_bvh   *bvh;

	constraint_t *constraint;

	/********************  INTERACTION  *********************/
	opus_real    translation_delta, scale_delta;
	opus_vec2 translation;
	opus_real    scale;
	body_t *selected_body;
	int     select_and_click_once;

} g_param = {0};

void on_pointer_down(opus_input *input)
{
	body_t **all_bodies = composite_all_bodies(g_param.phy_engine->world);
	uint64_t i;
	opus_vec2 p, m = input->pointer;
	opus_mat2d mat1, mat2, inv;

	/* FIXME */
	opus_mat2d_scale(mat1, g_param.scale, g_param.scale);
	opus_mat2d_translate(mat2, g_param.translation.x, g_param.translation.y);
	opus_mat2d_mul(mat2, mat1);
	opus_mat2d_inv(inv, mat2);
	opus_mar2d_pre_mul_xy(&p.x, &p.y, inv, m.x, m.y);

	opus_set_polygon_offset(0, sizeof(body_vertex_t));
	for (i = 0; i < opus_arr_len(all_bodies); i++) {
		body_t *b = all_bodies[i];

		if (g_param.selected_body != NULL && g_param.select_and_click_once == 0) {
			body_set_position(g_param.selected_body, p);
			sleeping_set(g_param.selected_body, 0);
			g_param.select_and_click_once = 1;
			g_param.selected_body         = NULL;
			return;
		} else if (opus_contains_(b->vertices, opus_arr_len(b->vertices), p)) {
			g_param.selected_body         = b;
			g_param.select_and_click_once = 0;
			return;
		}
	}

	g_param.selected_body = 0;
}

void on_pointer_move(opus_input *input)
{
	if (input->is_pointer_down) {}
}

void on_key_down(opus_input *input)
{
	if (input->keys_state[GLFW_KEY_Q]) {
		body_set_angle(g_param.b1, g_param.b1->angle + 0.2);
		body_set_position(g_param.stacks[0], opus_vec2_(100, 100));
		body_set_position(g_param.stacks[1], opus_vec2_(100, 100));
	} else if (input->keys_state[GLFW_KEY_SPACE]) {
		g_param.enable_physics = !g_param.enable_physics;
	}
}

void on_scroll(opus_input *input)
{
	if (input->scroll_y > 0) { g_param.scale *= 1 + g_param.scale_delta; }
	if (input->scroll_y < 0) { g_param.scale *= 1 - g_param.scale_delta; }
}

void preload(opus_engine *engine)
{
	plutovg_surface_t *surface = plutovg_surface_create(engine->width, engine->height);
	g_param.pl                 = plutovg_create(surface);
	g_param.font_gl            = vg_gl_font_create("../assets/fonts/georgiaz.ttf");

	plutovg_set_font(g_param.pl, plutovg_font_load_from_file("../assets/fonts/consola.ttf", 18));

	{
		uint64_t i, n = 5;
		opus_real     x = 100, y = 590;
		int      ix, iy;
		opus_real     w = 30, gap = 1;
		body_t **all_bodies;

		g_param.phy_engine = physics_engine_create();

		opus_arr_create(g_param.stacks, sizeof(body_t *));
		for (iy = 0; iy < n; iy++) {
			for (ix = 0; ix < n; ix++) {
				body_t *r = common_rectangle(opus_vec2_(ix * (w + gap) + x, iy * (w + gap) + y), w, w);
				opus_arr_push(g_param.stacks, &r);
				r->density = 0.09;
				composite_add_body(g_param.phy_engine->world, r);
			}
		}

		g_param.constraint = constraint_create(
		        constraint_(g_param.stacks[0], g_param.stacks[1], opus_vec2_(0, 0), opus_vec2_(0, 0), 100));
		/*composite_add_constraint(g_param.phy_engine->world, g_param.constraint);*/

		OPUS_INFO("%d - %d\n", g_param.stacks[0]->id, g_param.stacks[1]->id);

		g_param.b1 = body_create();
		g_param.b2 = body_create();
		opus_vec2 v1[4] = {{439, 109}, {400, 126}, {416, 192}, {501, 116}};
		opus_vec2 v2[5] = {{244, 161}, {327, 164}, {336, 270}, {261, 323}, {138, 245}};
		body_set_vertices(g_param.b1, vertices_create(v1, 4, g_param.b1));
		body_set_position(g_param.b1, opus_vec2_(600, 200));
		body_set_vertices(g_param.b2, vertices_create(v2, 5, g_param.b2));
		body_set_position(g_param.b2, opus_vec2_(500, 200));
		composite_add_body(g_param.phy_engine->world, g_param.b1);
		composite_add_body(g_param.phy_engine->world, g_param.b2);

		/* walls: down up left right */
		g_param.walls[0] = common_rectangle(opus_vec2_(400, 780), 780, 20);
		g_param.walls[1] = common_rectangle(opus_vec2_(400, 10), 780, 20);
		g_param.walls[2] = common_rectangle(opus_vec2_(10, 400), 20, 780);
		g_param.walls[3] = common_rectangle(opus_vec2_(780, 400), 20, 780);
		/*body_rotate(g_param.walls[0], 0.1, vec2_(0, 0), 0);*/
		for (iy = 0; iy < 4; iy++) {
			body_set_static(g_param.walls[iy], 1);
			composite_add_body(g_param.phy_engine->world, g_param.walls[iy]);
		}

		g_param.phy_engine->gravity.y       = 1;
		g_param.phy_engine->gravity_scale   = 0.00006;
		g_param.phy_engine->enable_sleeping = g_param.enable_sleeping;
		g_param.phy_engine->is_fixed        = 1;

		/*g_param.bvh = bvh_tree_create();
		all_bodies = composite_all_bodies(g_param.phy_engine->world);
		for (i = 0; i < array_len(all_bodies); i++) {
		    bvh_tree_insert(g_param.bvh, all_bodies[i], 0);
		}*/
	}
}

void update(opus_engine *engine, opus_real delta)
{
	opus_input *input = g_param.input;
	if (g_param.enable_physics) { physics_engine_update(g_param.phy_engine); }

	if (g_param.bvh) {
		size_t   i;
		opus_real s          = opus_engine_get_time();
		body_t **all_bodies = composite_all_bodies(g_param.phy_engine->world);
		opus_bvh_destroy(g_param.bvh);
		g_param.bvh = opus_bvh_create();
		for (i = 0; i < opus_arr_len(all_bodies); i++) {
			bvh_tree_insert(g_param.bvh, all_bodies[i], 0);
		}
		OPUS_INFO("%f\n", opus_engine_get_time() - s);
	}

	if (input->keys_state[GLFW_KEY_W])
		g_param.translation.y += g_param.translation_delta / g_param.scale;
	if (input->keys_state[GLFW_KEY_A])
		g_param.translation.x += g_param.translation_delta / g_param.scale;
	if (input->keys_state[GLFW_KEY_S])
		g_param.translation.y -= g_param.translation_delta / g_param.scale;
	if (input->keys_state[GLFW_KEY_D])
		g_param.translation.x -= g_param.translation_delta / g_param.scale;
}

static void draw_info()
{
	opus_vec2 p;
	char text[1024];

	opus_vec2_copy(&p, g_param.input->pointer);
	sprintf(text,
	        "FPS: %.2lf %.5lf\n"
	        "World Coordinate: (%.2lf, %.2lf)\n"
	        "translation: (%.3f, %.3f)\n"
	        "scale: %.3f\n",
	        g_param.engine->fps * 1000, g_param.engine->elapsed_time, p.x, p.y,
	        g_param.translation.x, g_param.translation.y, g_param.scale);

	plutovg_set_font_size(g_param.pl, 18);
	opus_pl_text_box(g_param.pl, text, -1, 400, 10, -1, -1, 0, 0, 1, 0);
	plutovg_set_source_rgb(g_param.pl, COLOR_BLACK);
	plutovg_fill(g_param.pl);
}

static void display_collisions_info(collision_t **collisions, opus_real x, opus_real y,
                                    opus_real tile_len, opus_real margin)
{
	char text[16];

	opus_real ox = x + margin, oy = y + margin, w = tile_len;
	uint64_t i, max_id = body_max_id();

	/* background */
	plutovg_rect(g_param.pl, ox - margin, oy - margin, w * (opus_real) max_id + 2 * margin,
	             w * (opus_real) max_id + 2 * margin);
	plutovg_set_source_rgb(g_param.pl, COLOR_BLACK);
	plutovg_fill(g_param.pl);

	/* border */
	plutovg_rect(g_param.pl, ox - margin, oy - margin, w * (opus_real) max_id + 2 * margin,
	             w * (opus_real) max_id + 2 * margin);
	plutovg_set_source_rgb(g_param.pl, COLOR_WHITE);
	plutovg_stroke(g_param.pl);

	/* gray tile means there exists a collision between respondent bodies */
	for (i = 0; i < opus_arr_len(collisions); i++) {
		plutovg_rect(g_param.pl, (opus_real) collisions[i]->body_a->id * w + ox - w,
		             (opus_real) collisions[i]->body_b->id * w + oy - w, w, w);
	}
	plutovg_set_source_rgb(g_param.pl, COLOR_GREEN2);
	plutovg_fill(g_param.pl);

	/* vertical separation line */
	for (i = 0; i < max_id; i++) {
		if (i % 5 == 0) {
			snprintf(text, 16, "%" PRIu64, i + 1);
			plutovg_text(g_param.pl, text, ox + w * (opus_real) i, oy - w);
		}

		plutovg_move_to(g_param.pl, ox + w * (opus_real) i, oy);
		plutovg_line_to(g_param.pl, ox + w * (opus_real) i, oy + (opus_real) max_id * w);
		plutovg_set_source_rgba(g_param.pl, COLOR_GRAY81, i % 5 ? 0.5 : 1);
		plutovg_stroke(g_param.pl);
	}

	/* horizontal separation line */
	for (i = 0; i < body_max_id(); i++) {
		if (i % 5 == 0) {
			snprintf(text, 16, "%" PRIu64, i + 1);
			plutovg_text(g_param.pl, text, ox - w, oy + w * (opus_real) i + w);
		}

		plutovg_move_to(g_param.pl, ox, oy + w * (opus_real) i);
		plutovg_line_to(g_param.pl, ox + (opus_real) max_id * w, oy + w * (opus_real) i);
		plutovg_set_source_rgba(g_param.pl, COLOR_GRAY81, i % 5 ? 0.5 : 1);
		plutovg_stroke(g_param.pl);
	}
}

static void draw_body(body_t *body)
{
	size_t j;
	char   text[128];

	plutovg_move_to(g_param.pl, body->vertices[0].x, body->vertices[0].y);
	for (j = 1; j < opus_arr_len(body->vertices); j++) {
		plutovg_line_to(g_param.pl, body->vertices[j].x, body->vertices[j].y);
	}
	plutovg_close_path(g_param.pl);
	plutovg_set_source_rgba(g_param.pl, COLOR_BLACK, 0.6);
	plutovg_stroke_preserve(g_param.pl);
	plutovg_set_source_rgba(g_param.pl, COLOR_GRAY81, 0.6);
	plutovg_fill(g_param.pl);

	/* draw body's ID */
	sprintf(text, "%s%" PRIu64 "", body->is_sleeping ? "s" : "", body->id);
	plutovg_set_font_size(g_param.pl, 10);
	plutovg_set_source_rgb(g_param.pl, COLOR_BLACK);
	plutovg_text(g_param.pl, text, body->position.x, body->position.y);
	plutovg_fill(g_param.pl);

	/*plutovg_circle(g_param.pl, body->position.x, body->position.y, 2);
	plutovg_fill(g_param.pl);*/
}

void render(opus_engine *engine)
{
	body_t       **all_bodies;
	constraint_t **all_constraints;
	uint64_t       i, j;

	int s_width  = g_param.pl->surface->width;
	int s_height = g_param.pl->surface->height;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	memset(g_param.pl->surface->data, 255, s_width * s_height * 4);
	plutovg_set_source_rgba(g_param.pl, COLOR_ANTIQUE_WHITE3, 0.1);
	plutovg_rect(g_param.pl, 0, 0, s_width, s_height);
	plutovg_fill(g_param.pl);

	plutovg_set_line_width(g_param.pl, 1);

	plutovg_save(g_param.pl);
	plutovg_scale(g_param.pl, g_param.scale, g_param.scale);
	plutovg_translate(g_param.pl, g_param.translation.x, g_param.translation.y);

	/* draw bodies */
	all_bodies = composite_all_bodies(g_param.phy_engine->world);
	for (i = 0; i < opus_arr_len(all_bodies); i++) draw_body(all_bodies[i]);

	/* draw constraints */
	all_constraints = composite_all_constraints(g_param.phy_engine->world);
	for (i = 0; i < opus_arr_len(all_constraints); i++) {
		constraint_t *c = all_constraints[i];

		opus_vec2 pa = c->point_a, pb = c->point_b;
		if (c->body_a) pa = opus_vec2_add(pa, c->body_a->position);
		if (c->body_b) pb = opus_vec2_add(pb, c->body_b->position);

		opus_pl_line(g_param.pl, pa.x, pa.y, pb.x, pb.y);
		plutovg_set_source_rgb(g_param.pl, COLOR_BLACK);
		plutovg_stroke(g_param.pl);
	}

	/* draw collision contact */
	for (i = 0; i < opus_arr_len(g_param.phy_engine->detector->collisions); i++) {
		collision_t *col = g_param.phy_engine->detector->collisions[i];
		for (j = 0; j < col->n_supports; j++) {
			opus_real sx = col->supports[j].x, sy = col->supports[j].y;
			opus_real nx = col->normal.x, ny = col->normal.y;
			opus_vec2 v;
			opus_vec2_set(&v, nx, ny);
			opus_vec2_set_length(&v, 8);
			plutovg_circle(g_param.pl, sx, sy, 2.3);
			plutovg_set_source_rgb(g_param.pl, COLOR_RED);
			plutovg_fill(g_param.pl);

			plutovg_move_to(g_param.pl, sx, sy);
			plutovg_line_to(g_param.pl, sx + v.x, sy + v.y);
			plutovg_stroke(g_param.pl);
		}
	}

	/* draw bounding volume hierarchy */
	if (g_param.bvh) opus_bvh_render(g_param.pl, g_param.bvh);

	plutovg_restore(g_param.pl);

	/* draw information */
	/*display_collisions_info(g_param.phy_engine->detector->collisions, 20, 20, 8, 20);*/
	draw_info();

	/* render on screen */
	g_param.program_pl->use(g_param.program_pl, g_param.pl->surface->data, s_width, s_height);
}

void cleanup(opus_engine *engine)
{
	plutovg_destroy(g_param.pl);
	vg_gl_font_destroy(g_param.font_gl);

	/* release memory related to physics engine */
	{
		uint64_t i;
		body_t **all_bodies = composite_all_bodies(g_param.phy_engine->world);
		OPUS_INFO("destroy %" PRIu64 " bodies\n", opus_arr_len(all_bodies));
		for (i = 0; i < opus_arr_len(all_bodies); i++) body_destroy(all_bodies[i]);
		opus_arr_destroy(g_param.stacks);
		physics_engine_destroy(g_param.phy_engine);

		if (g_param.bvh) opus_bvh_destroy(g_param.bvh);

		constraint_destroy(g_param.constraint);
	}
}

void lup(opus_real *mat, uint16_t n, opus_real *LU, uint16_t *P)
{
	uint16_t i, j, k;

	for (k = 0; k < n; k++) P[k] = k;
	memcpy(LU, mat, n * n * sizeof(opus_real));

	for (i = 0; i < n; i++) {}
}

int main()
{
	if (1) {
		g_param.translation_delta = 10;
		g_param.scale_delta       = 0.02;
		g_param.translation       = opus_vec2_(-46, -510);
		g_param.scale             = 2.972;
		g_param.enable_physics    = 1;
		g_param.enable_sleeping   = 0;

		/* engine to create glfw window and a combined "vg_t" instance */
		g_param.engine = opus_engine_create(800, 800, "vg");

		/* handle glfw window input */
		g_param.input = opus_input_init(g_param.engine);

		/* specially designed to draw [double, double, ...] like path(tessellate to triangle and
		 * draw with opengl) */
		g_param.program_gl = vg_gl_program_preset1();

		/* specially designed to render pluto_vg surface buffer */
		g_param.program_pl = vg_gl_program_preset2();

		opus_engine_set_callback(g_param.engine, preload, update, render, cleanup);
		opus_input_on(g_param.input->on_pointer_down, on_pointer_down);
		opus_input_on(g_param.input->on_key_down, on_key_down);
		opus_input_on(g_param.input->on_scroll, on_scroll);
		opus_input_on(g_param.input->on_pointer_move, on_pointer_move /* start engine main loop */);
		opus_engine_start(g_param.engine);

		/* release resources */
		vg_gl_program_destroy(g_param.program_gl);
		vg_gl_program_destroy(g_param.program_pl);
		opus_engine_destroy(g_param.engine);
		opus_input_done();
	} else {
		int i, j;

		opus_real A[9] = {-5, 3, 4, 10, -8, -9, 15, 1, 2};
	}


	return 0;
}
