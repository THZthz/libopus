#include "vg/vg_color.h"
#include "vg/vg_engine.h"
#include "vg/vg_input.h"
#include "vg/vg_utils.h"
#include "vg/vg_gl.h"
#include "vg/pluto/plutovg-private.h"
#include "vg/pluto/plutovg.h"
#include "physics/opus/physics.h"
#include "physics/opus/physics_private.h"
#include "math/polygon/polygon.h"
#include "math/geometry.h"

struct param {
	/********************  VG_ENGINE  **************************/
	vg_engine_t     *engine;
	vg_input_t      *input;
	vg_gl_program_t *program_gl, *program_pl;
	vg_gl_font_t    *font_gl;
	plutovg_t       *pl;

	/******************  PHYSICS_ENGINE  *********************/
	opus_polygon *pa, *pb;
	opus_circle  *ca;
	opus_real     rot_a, rot_b, rot_c;
	opus_vec2     pos_a, pos_b, pos_c;
	opus_real     pa_angle;

	opus_physics_world *world;

	/********************  INTERACTION  *********************/
	opus_real translation_delta, scale_delta;
	opus_vec2 translation;
	opus_real scale;

	int test_SAT;
	int test_world;

} g_param = {0};

plutovg_t *pl;
opus_vec2  pp;

void on_pointer_down(vg_input_t *input) {}

void on_pointer_move(vg_input_t *input)
{
	opus_vec2  p;
	opus_mat2d mat1, mat2, inv, m;

	char text[512];

	/* FIXME */
	opus_mat2d_scale(mat1, g_param.scale, g_param.scale);
	opus_mat2d_translate(mat2, g_param.translation.x, g_param.translation.y);
	opus_mat2d_mul(mat2, mat1);
	opus_mat2d_inv(inv, mat2);
	opus_mar2d_pre_mul_xy(&p.x, &p.y, inv, input->pointer.x, input->pointer.y);

	if (g_param.test_SAT) { g_param.pos_a = opus_vec2_add(p, opus_vec2_(-40, -40)); }

	pp = p;
}

void on_key_down(vg_input_t *input)
{
	if (g_param.test_SAT) {
		if (input->keys_state[GLFW_KEY_Q]) {
			g_param.pa_angle += 0.01;
			opus_rotate(g_param.pa->vertices, g_param.pa->n, opus_vec2_(0, 0), g_param.pa_angle);
		}
	}

	if (g_param.test_world) {
		if (input->keys_state[GLFW_KEY_ENTER]) {
			scanf("%f", &g_param.world->bias_factor);
		}
	}
}

void on_scroll(vg_input_t *input)
{
	if (input->scroll_y > 0) { g_param.scale *= 1 + g_param.scale_delta; }
	if (input->scroll_y < 0) { g_param.scale *= 1 - g_param.scale_delta; }

	if (g_param.test_SAT) { g_param.rot_a += input->scroll_y / 10; }
}

void preload(vg_engine_t *engine)
{
	opus_vec2 v1[4]         = {{439, 109}, {400, 126}, {416, 192}, {501, 116}};
	opus_vec2 v2[5]         = {{244, 161}, {327, 164}, {336, 270}, {261, 323}, {138, 245}};
	opus_vec2 rect_verts[4] = {{-200, -20}, {200, -20}, {200, 20}, {-200, 20}};

	plutovg_surface_t *surface = plutovg_surface_create(engine->width, engine->height);


	g_param.pl      = plutovg_create(surface);
	pl              = g_param.pl;
	g_param.font_gl = vg_gl_font_create("../assets/fonts/georgiaz.ttf");

	plutovg_set_font(g_param.pl, plutovg_font_load_from_file("../assets/fonts/consola.ttf", 18));

	if (g_param.test_SAT) {
		g_param.pa = opus_shape_polygon_create(v1, 4, opus_vec2_(0, 0));
		g_param.pb = opus_shape_polygon_create(v2, 5, opus_vec2_(0, 0));
		g_param.ca = opus_shape_circle_create(50);

		g_param.pos_a = opus_vec2_(400, 400);
		g_param.pos_b = opus_vec2_(450, 400);
		g_param.pos_c = opus_vec2_(450, 400);
	}

	if (g_param.test_world) {
		opus_body *b;
		g_param.world = opus_physics_world_create();
		//		 opus_physics_world_add_polygon(g_param.world, opus_vec2_(400, 400), v1, 4);
		//		opus_physics_world_add_polygon(g_param.world,opus_vec2_(300, 400), v2, 5);
		b       = opus_physics_world_add_rect(g_param.world, opus_vec2_(400, 700), 800, 20, 5);
		b->type = OPUS_BODY_STATIC;

		size_t    i, j;
		opus_real gap = 3, w = 70, x = 350, y = 350;
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 4; j++)
				opus_physics_world_add_rect(g_param.world, opus_vec2_(x + (float) i * (w + gap), y + (float) j * (w + gap)), w, w, 0);
		}
	}
}

void draw_shape(opus_shape *s, opus_real rot, opus_vec2 pos)
{
	opus_circle  *circle  = (void *) s;
	opus_polygon *polygon = (void *) s;

	opus_mat2d t;
	size_t     i;
	opus_vec2  p;
	opus_mat2d_rotate_about(t, rot, pos);
	switch (s->type_) {
		case PHYSICS_SHAPE_POLYGON: {
			for (i = 0; i < polygon->n; i++) {
				opus_mar2d_pre_mul_xy(&p.x, &p.y, t, polygon->vertices[i].x,
				                      polygon->vertices[i].y);
				if (i == 0) plutovg_move_to(g_param.pl, p.x, p.y);
				else
					plutovg_line_to(g_param.pl, p.x, p.y);
			}
			plutovg_close_path(g_param.pl);
			break;
		}
		case PHYSICS_SHAPE_CIRCLE: {
			p = opus_mat2d_pre_mul_vec(t, opus_vec2_(0, 0));
			plutovg_circle(g_param.pl, p.x, p.y, circle->radius);
			break;
		}
	}
}

void update(vg_engine_t *engine, opus_real delta)
{
	vg_input_t *input = g_param.input;

	if (input->keys_state[GLFW_KEY_W])
		g_param.translation.y += g_param.translation_delta / g_param.scale;
	if (input->keys_state[GLFW_KEY_A])
		g_param.translation.x += g_param.translation_delta / g_param.scale;
	if (input->keys_state[GLFW_KEY_S])
		g_param.translation.y -= g_param.translation_delta / g_param.scale;
	if (input->keys_state[GLFW_KEY_D])
		g_param.translation.x -= g_param.translation_delta / g_param.scale;

	if (g_param.test_world) {}
}

void test_SAT()
{
	char text[512];

	size_t i;

	opus_overlap_result dr;
	opus_clip_result    cr;

	opus_mat2d t, mat;
	/*


	    dr = physics_SAT((void *) g_param.pa, (void *) g_param.ca, g_param.tpa, g_param.tca);

	    plutovg_set_source_rgb(g_param.pl, COLOR_BLACK);
	    draw_shape((void *) g_param.pa, g_param.tpa);
	    draw_shape((void *) g_param.ca, g_param.tca);
	    plutovg_stroke(g_param.pl);

	    if (dr.is_overlap) {
	        vec2 c;
	        cr = physics_VCLIP(dr);

	        plutovg_set_source_rgb(g_param.pl, COLOR_BLACK);
	        mat2d_transform_vec(&c, g_param.tca, vec2_(0, 0));
	        vg_pl_arrow(pl, c, dr.normal, r_deg2rad(20), 10,- dr.separation);
	        plutovg_stroke(pl);

	        if (cr.A == (void *) g_param.tca)
	            plutovg_set_source_rgb(g_param.pl, COLOR_RED);
	        else
	            plutovg_set_source_rgb(g_param.pl, COLOR_BLUE);

	        plutovg_circle(pl, cr.supports[0][0].x, cr.supports[0][0].y, 5);
	        plutovg_fill(pl);

	        if (cr.B == (void *) g_param.tca)
	            plutovg_set_source_rgb(g_param.pl,COLOR_BLUE );
	        else
	            plutovg_set_source_rgb(g_param.pl, COLOR_RED);
	        plutovg_circle(pl, cr.supports[0][1].x, cr.supports[0][1].y, 5);
	        plutovg_fill(pl);

	    }
	*/

	opus_mat2d ta, tb;
	opus_mat2d_rotate_about(ta, g_param.rot_a, g_param.pos_a);
	opus_mat2d_rotate_about(tb, g_param.rot_b, g_param.pos_b);
	dr = opus_SAT((void *) g_param.pa, (void *) g_param.pb, ta, tb);

	plutovg_set_line_width(g_param.pl, 1);
	plutovg_set_source_rgba(g_param.pl, COLOR_BLACK, 1);
	draw_shape((void *) g_param.pa, g_param.rot_a, g_param.pos_a);
	draw_shape((void *) g_param.pb, g_param.rot_b, g_param.pos_b);
	plutovg_stroke(g_param.pl);

	if (dr.is_overlap) {
		cr = opus_VCLIP(dr);

		plutovg_set_line_width(g_param.pl, 2);
		plutovg_set_source_rgba(g_param.pl, COLOR_ORANGE, 1);

		sprintf(text, "%d", 2);
		plutovg_text(g_param.pl, text, 20, 20);
		plutovg_fill(g_param.pl);

		for (i = 0; i < 2; i++) {
			plutovg_set_source_rgba(g_param.pl, COLOR_RED, 1);
			plutovg_circle(g_param.pl, cr.supports[i][0].x, cr.supports[i][0].y, 4);
			plutovg_fill(g_param.pl);
			plutovg_set_source_rgba(g_param.pl, COLOR_BLUE, 1);
			plutovg_circle(g_param.pl, cr.supports[i][1].x, cr.supports[i][1].y, 4);
			plutovg_fill(g_param.pl);
		}

		plutovg_set_source_rgba(g_param.pl, COLOR_PURPLE, 1);
		vg_pl_arrow(pl, opus_vec2_scale(opus_vec2_add(cr.supports[0][0], cr.supports[1][0]), 0.5), dr.normal,
		            opus_deg2rad(20), 5, dr.separation);
		plutovg_stroke(pl);

		opus_vec2 p;
		plutovg_set_source_rgba(g_param.pl, COLOR_GRAY81, 1);
		if (dr.A == (void *) g_param.pa) {
			p = opus_vec2_add(g_param.pos_b, opus_vec2_scale(dr.normal, dr.separation));
			draw_shape((void *) g_param.pb, g_param.rot_b, p);
			plutovg_stroke(pl);
		} else {
			p = opus_vec2_add(g_param.pos_a, opus_vec2_scale(dr.normal, dr.separation));
			draw_shape((void *) g_param.pa, g_param.rot_a, p);
			plutovg_stroke(pl);
		}
	}

	/*vec2 a = {300, 300};
	vec2 b = {350, 320};
	vg_pl_line(g_param.pl, a.x, a.y, b.x, b.y);
	vg_pl_arrow(g_param.pl, a, vec2_perp(vec2_to(a, b)), 1, 5, 88);
	vg_pl_arrow(g_param.pl, b, vec2_perp(vec2_to(a, b)), 1, 5, 88);
	plutovg_stroke(pl);

	int v = geo_voronoi_region2(a, b, g_param.input->pointer);
	plutovg_set_source_rgb(pl, COLOR_BLACK);
	if (v == -1) plutovg_set_source_rgb(pl, COLOR_RED);
	if (v == 1) plutovg_set_source_rgb(pl, COLOR_BLUE);
	plutovg_circle(pl, g_param.input->pointer.x, g_param.input->pointer.y, 4);
	plutovg_fill(pl);*/

	/*vec2 a = {300, 300};
	vec2 b = {340, 320};
	vec2 c = {340, 220};
	vec2 sd;
	real ss, sss;

	vg_pl_line(g_param.pl, a.x, a.y, b.x, b.y);
	vg_pl_line(g_param.pl, c.x, c.y, g_param.input->pointer.x, g_param.input->pointer.y);
	plutovg_stroke(pl);
	geo_ll_intersect(a.x, a.y, b.x, b.y, c.x, c.y, g_param.input->pointer.x,
	g_param.input->pointer.y, &sd.x, &sd.y, &ss, &sss); plutovg_circle(pl, sd.x, sd.y, 4);
	plutovg_fill(pl);*/
}

void render(vg_engine_t *engine)
{
	char text[512];
	int  s_width  = g_param.pl->surface->width;
	int  s_height = g_param.pl->surface->height;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	memset(g_param.pl->surface->data, 255, s_width * s_height * 4);
	plutovg_set_source_rgba(g_param.pl, COLOR_ANTIQUE_WHITE3, 0.1);
	plutovg_rect(g_param.pl, 0, 0, s_width, s_height);
	plutovg_fill(g_param.pl);

	sprintf(text, "(%.3f, %.3f) %.3f", g_param.translation.x, g_param.translation.y, g_param.scale);
	plutovg_text(g_param.pl, text, 10, 30);
	plutovg_set_source_rgba(g_param.pl, COLOR_BLACK, 1);
	plutovg_set_line_width(g_param.pl, 1);
	plutovg_fill(g_param.pl);

	plutovg_save(g_param.pl);
	plutovg_scale(g_param.pl, g_param.scale, g_param.scale);
	plutovg_translate(g_param.pl, g_param.translation.x, g_param.translation.y);

	if (g_param.test_SAT) test_SAT();

	if (g_param.test_world) {
		g_param.world->velocity_iteration = 6;
		g_param.world->position_iteration = 3;
		g_param.world->bias_factor        = 0.09;
		g_param.world->position_slop      = 0.01;

		g_param.world->gravity = opus_vec2_(0, 1);
		opus_physics_world_step(g_param.world, 1);

		size_t i;
		for (i = 0; i < opus_arr_len(g_param.world->bodies); i++) {
			char       text[32];
			opus_body *b = g_param.world->bodies[i];
			sprintf(text, "%u", b->id);
			plutovg_text(pl, text, b->position.x, b->position.y);
			plutovg_set_source_rgba(pl, COLOR_BLACK, 1);
			plutovg_fill(pl);
			draw_shape((void *) b->shape, (float) b->rotation, b->position);
			plutovg_set_source_rgba(pl, COLOR_BLACK, 1);
			plutovg_stroke(pl);
			//			if (b->type != OPUS_BODY_STATIC) printf("%f %f\n", b->position.x, b->position.y);
		}
	}

	plutovg_restore(g_param.pl);

	/* render on screen */
	g_param.program_pl->use(g_param.program_pl, g_param.pl->surface->data, s_width, s_height);
}

void cleanup(vg_engine_t *engine)
{
	plutovg_destroy(g_param.pl);
	vg_gl_font_destroy(g_param.font_gl);

	if (g_param.test_SAT) {
		opus_shape_polygon_destroy(g_param.pa);
		opus_shape_polygon_destroy(g_param.pb);
	}
}

int main()
{
	g_param.test_SAT   = 0;
	g_param.test_world = 1;
	if (1) {
		g_param.translation_delta = 10;
		g_param.scale_delta       = 0.02;
		g_param.scale             = 1;

		g_param.translation = opus_vec2_(-155.36, -307.614);
		g_param.scale       = 1.536;

		/* engine to create glfw window and a combined "vg_t" instance */
		g_param.engine = vg_engine_create(800, 800, "vg");

		/* handle glfw window input */
		g_param.input = vg_input_init(g_param.engine);

		/* specially designed to draw [double, double, ...] like path(tessellate to triangle and
		 * draw with opengl) */
		g_param.program_gl = vg_gl_program_preset1();

		/* specially designed to render pluto_vg surface buffer */
		g_param.program_pl = vg_gl_program_preset2();

		vg_engine_set(g_param.engine, preload, update, render, cleanup);
		vg_input_on(g_param.input->on_pointer_down, on_pointer_down);
		vg_input_on(g_param.input->on_key_down, on_key_down);
		vg_input_on(g_param.input->on_scroll, on_scroll);
		vg_input_on(g_param.input->on_pointer_move, on_pointer_move);
		vg_engine_start(g_param.engine);

		/* release resources */
		vg_gl_program_destroy(g_param.program_gl);
		vg_gl_program_destroy(g_param.program_pl);
		vg_engine_destroy(g_param.engine);
		vg_input_done();
	} else {
	}


	return 0;
}
