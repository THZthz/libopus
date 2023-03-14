#include "opus.h"

struct param {
	/********************  VG_ENGINE  **************************/
	opus_engine     *engine;
	opus_input      *input;
	opus_gl_program *program_gl, *program_pl;
	opus_font       *font_gl;
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

	opus_vec2 pointer;

	opus_body *selected;

	int test_SAT;
	int test_world;

} g_param = {0};

plutovg_t *pl;

void on_pointer_down(opus_input *input)
{
	uint64_t i;

	int clicked_a_body = 0;

	opus_vec2     local;
	opus_body    *body;
	opus_polygon *poly;

	if (g_param.selected) {
		opus_body_set_position(g_param.selected, g_param.pointer);
		g_param.selected = NULL;
		return;
	}

	for (i = 0; i < opus_arr_len(g_param.world->bodies); i++) {
		body = g_param.world->bodies[i];
		poly = (void *) body->shape;
		OPUS_ASSERT(body->shape->type_ == OPUS_SHAPE_POLYGON);
		local = opus_body_w2l(body, g_param.pointer);
		if (opus_contains(poly->vertices, poly->n, local)) {
			g_param.selected = body;
			clicked_a_body   = 1;
		}
	}

	if (!clicked_a_body) {
		g_param.selected = NULL;
	}
}

void on_pointer_move(opus_input *input)
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

	g_param.pointer = p;
}

void on_scroll(opus_input *input)
{
	if (input->scroll_y > 0) { g_param.scale *= 1 + g_param.scale_delta; }
	if (input->scroll_y < 0) { g_param.scale *= 1 - g_param.scale_delta; }

	if (g_param.test_SAT) { g_param.rot_a += input->scroll_y / 10; }
}

void preload(opus_engine *engine)
{
	opus_vec2 v1[4]         = {{439, 109}, {400, 126}, {416, 192}, {501, 116}};
	opus_vec2 v2[5]         = {{244, 161}, {327, 164}, {336, 270}, {261, 323}, {138, 245}};
	opus_vec2 rect_verts[4] = {{-200, -20}, {200, -20}, {200, 20}, {-200, 20}};

	plutovg_surface_t *surface = plutovg_surface_create(engine->width, engine->height);


	g_param.pl = plutovg_create(surface);
	pl         = g_param.pl;
#ifdef __EMSCRIPTEN__
	g_param.font_gl = vg_gl_font_create("georgiaz.ttf");
	plutovg_set_font(g_param.pl, plutovg_font_load_from_file("consola.ttf", 18));
#else
	g_param.font_gl = vg_gl_font_create("../assets/fonts/georgiaz.ttf");
	plutovg_set_font(g_param.pl, plutovg_font_load_from_file("../assets/fonts/consola.ttf", 18));
#endif /* __EMSCRIPTEN__ */

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

		if (1) {
			//					 opus_physics_world_add_polygon(g_param.world, opus_vec2_(400, 400), v1, 4);
			//					opus_physics_world_add_polygon(g_param.world,opus_vec2_(300, 400), v2, 5);
			b       = opus_physics_world_add_rect(g_param.world, opus_vec2_(400, 700), 2200, 40, 5);
			b->type = OPUS_BODY_STATIC;

			size_t    i, j;
			opus_real gap = 3, w = 40, h = 40, x = 350, y = -10;
			for (i = 0; i < 10; i++) {
				for (j = 0; j < 10; j++) {
					opus_physics_world_add_rect(g_param.world, opus_vec2_(x + (float) i * (w + gap), y + (float) j * (h + gap)), w, h, 0);
				}
			}

			opus_body           *cur, *prev;
			opus_joint_revolute *joint;
			cur  = NULL;
			prev = NULL;
			x    = -300;
			y    = 0;
			for (i = 0; i < 8; i++) {
				cur = opus_physics_world_add_rect(g_param.world, opus_vec2_(x + (float) i * (w + gap), y + (float) 0 * (h + gap)), w, h, 0);
				if (i == 0) {
					opus_physics_world_add_distance_joint(g_param.world, cur,
					                                      opus_vec2_(-w / 2, 0),
					                                      opus_vec2_add(cur->position, opus_vec2_(-w / 2 - 2 * gap, 0)), 0, 10);
				} else if (i == 7) {
					opus_physics_world_add_distance_joint(g_param.world, cur,
					                                      opus_vec2_(w / 2, 0),
					                                      opus_vec2_add(cur->position, opus_vec2_(w / 2 + 2 * gap, 0)), 0, 10);
				}
				if (prev) {
					joint            = (void *) opus_physics_world_add_revolute_joint(g_param.world, cur, prev, opus_vec2_(-w / 2 - 2 * gap, 0), opus_vec2_(w / 2 + 2 * gap, 0));
					joint->stiffness = 1;
				}

				prev = cur;
			}
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
	opus_mat2d_rotate_about(t, (float) rot, pos);
	switch (s->type_) {
		case OPUS_SHAPE_POLYGON: {
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
		case OPUS_SHAPE_CIRCLE: {
			p = opus_mat2d_pre_mul_vec(t, opus_vec2_(0, 0));
			plutovg_circle(g_param.pl, p.x, p.y, circle->radius);
			break;
		}
	}
}

void update(opus_engine *engine, opus_real delta)
{
	opus_input *input = g_param.input;

	if (input->keys_state[GLFW_KEY_W])
		g_param.translation.y += g_param.translation_delta / g_param.scale;
	if (input->keys_state[GLFW_KEY_A])
		g_param.translation.x += g_param.translation_delta / g_param.scale;
	if (input->keys_state[GLFW_KEY_S])
		g_param.translation.y -= g_param.translation_delta / g_param.scale;
	if (input->keys_state[GLFW_KEY_D])
		g_param.translation.x -= g_param.translation_delta / g_param.scale;

	if (g_param.test_world) {
	}
}

void test_SAT(void)
{
	char text[512];

	size_t i;

	opus_overlap_result dr;
	opus_clip_result    cr;

	opus_mat2d t, mat, tp, tc;

	opus_mat2d_rotate_about(tp, g_param.rot_a, g_param.pos_a);
	opus_mat2d_rotate_about(tc, g_param.rot_c, g_param.pos_c);

	dr = opus_SAT((void *) g_param.pa, (void *) g_param.ca, tp, tc);

	plutovg_set_source_rgb(g_param.pl, COLOR_BLACK);
	draw_shape((void *) g_param.pa, g_param.rot_a, g_param.pos_a);
	draw_shape((void *) g_param.ca, g_param.rot_b, g_param.pos_b);
	plutovg_stroke(g_param.pl);

	plutovg_set_line_width(pl, 1);
	if (dr.is_overlap) {
		opus_vec2 c;
		cr = opus_VCLIP(dr);

		plutovg_set_source_rgb(g_param.pl, COLOR_BLACK);
		if (dr.A == (void *) g_param.ca) {
			c = opus_mat2d_pre_mul_vec(dr.transform_a, opus_vec2_(0, 0));
		} else {
			c = (cr.supports[0][0]);
		}
		opus_pl_arrow(pl, c, dr.normal, opus_deg2rad(20), 10, dr.separation);
		plutovg_stroke(pl);

		for (i = 0; i < cr.n_support; i++) {
			plutovg_circle(pl, cr.supports[i][0].x, cr.supports[i][0].y, 3);
			plutovg_set_source_rgb(pl, COLOR_RED);
			plutovg_fill(pl);

			plutovg_circle(pl, cr.supports[i][1].x, cr.supports[i][1].y, 3);
			plutovg_set_source_rgb(pl, COLOR_BLUE);
			plutovg_fill(pl);
		}
	}
	//
	//	opus_mat2d ta, tb;
	//	opus_mat2d_rotate_about(ta, g_param.rot_a, g_param.pos_a);
	//	opus_mat2d_rotate_about(tb, g_param.rot_b, g_param.pos_b);
	//	dr = opus_SAT((void *) g_param.pa, (void *) g_param.pb, ta, tb);
	//
	//	plutovg_set_line_width(g_param.pl, 1);
	//	plutovg_set_source_rgba(g_param.pl, COLOR_BLACK, 1);
	//	draw_shape((void *) g_param.pa, g_param.rot_a, g_param.pos_a);
	//	draw_shape((void *) g_param.pb, g_param.rot_b, g_param.pos_b);
	//	plutovg_stroke(g_param.pl);
	//
	//	if (dr.is_overlap) {
	//		cr = opus_VCLIP(dr);
	//
	//		plutovg_set_line_width(g_param.pl, 2);
	//		plutovg_set_source_rgba(g_param.pl, COLOR_ORANGE, 1);
	//
	//		sprintf(text, "%d", 2);
	//		plutovg_text(g_param.pl, text, 20, 20);
	//		plutovg_fill(g_param.pl);
	//
	//		for (i = 0; i < 2; i++) {
	//			plutovg_set_source_rgba(g_param.pl, COLOR_RED, 1);
	//			plutovg_circle(g_param.pl, cr.supports[i][0].x, cr.supports[i][0].y, 4);
	//			plutovg_fill(g_param.pl);
	//			plutovg_set_source_rgba(g_param.pl, COLOR_BLUE, 1);
	//			plutovg_circle(g_param.pl, cr.supports[i][1].x, cr.supports[i][1].y, 4);
	//			plutovg_fill(g_param.pl);
	//		}
	//
	//		plutovg_set_source_rgba(g_param.pl, COLOR_PURPLE, 1);
	//		opus_pl_arrow(pl, opus_vec2_scale(opus_vec2_add(cr.supports[0][0], cr.supports[1][0]), 0.5), dr.normal,
	//		              opus_deg2rad(20), 5, dr.separation);
	//		plutovg_stroke(pl);
	//
	//		opus_vec2 p;
	//		plutovg_set_source_rgba(g_param.pl, COLOR_GRAY81, 1);
	//		if (dr.A == (void *) g_param.pa) {
	//			p = opus_vec2_add(g_param.pos_b, opus_vec2_scale(dr.normal, dr.separation));
	//			draw_shape((void *) g_param.pb, g_param.rot_b, p);
	//			plutovg_stroke(pl);
	//		} else {
	//			p = opus_vec2_add(g_param.pos_a, opus_vec2_scale(dr.normal, dr.separation));
	//			draw_shape((void *) g_param.pa, g_param.rot_a, p);
	//			plutovg_stroke(pl);
	//		}
	//	}

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

void render(opus_engine *engine)
{
	char text[512];
	int  s_width  = g_param.pl->surface->width;
	int  s_height = g_param.pl->surface->height;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	memset(g_param.pl->surface->data, 255, s_width * s_height * 4);
	plutovg_set_source_rgba(g_param.pl, COLOR_ANTIQUE_WHITE3, 0.1);
	plutovg_rect(g_param.pl, 0, 0, s_width, s_height);
	plutovg_fill(g_param.pl);

	if (g_param.test_world) {
		sprintf(text, "FPS: %.3f | MOUSE: (%.3f, %.3f) | DELTA: %.5fms", engine->fps, g_param.pointer.x, g_param.pointer.y, engine->elapsed_time);
		plutovg_text(g_param.pl, text, 10, 30);
		plutovg_set_source_rgba(g_param.pl, COLOR_BLACK, 1);
		plutovg_set_line_width(g_param.pl, 1);
		plutovg_fill(g_param.pl);
	}

	plutovg_save(g_param.pl);
	plutovg_scale(g_param.pl, g_param.scale, g_param.scale);
	plutovg_translate(g_param.pl, g_param.translation.x, g_param.translation.y);

	if (g_param.test_SAT) test_SAT();

	if (g_param.test_world) {
		g_param.world->draw_contacts = 1;
		g_param.world->enable_sleeping = 1;
		g_param.world->gravity = opus_vec2_(0, 0.2);

		opus_physics_world_step(g_param.world, engine->elapsed_time);

		size_t i;
		for (i = 0; i < opus_arr_len(g_param.world->bodies); i++) {
			opus_body *b = g_param.world->bodies[i];

			/*sprintf(text, "%zu", b->id);
			plutovg_text(pl, text, b->position.x, b->position.y);
			plutovg_set_source_rgba(pl, COLOR_BLACK, 1);
			plutovg_fill(pl);*/

			draw_shape((void *) b->shape, (float) b->rotation, b->position);
			plutovg_set_line_width(pl, 0.3);
			plutovg_set_source_rgba(pl, COLOR_BLACK, 1);
			if (g_param.selected == b) {
				plutovg_set_line_width(pl, 1);
			}
			plutovg_stroke(pl);

			if (b->is_sleeping) {
				draw_shape((void *) b->shape, (float) b->rotation, b->position);
				plutovg_set_source_rgba(pl, COLOR_SLATE_GRAY1, 0.5);
				plutovg_fill(pl);
			}
		}
		for (i = 0; i < opus_arr_len(g_param.world->joints); i++) {
			switch (g_param.world->joints[i]->type) {
				case OPUS_JOINT_REVOLUTE: {
					opus_joint_revolute *joint = (void *) g_param.world->joints[i];
					opus_vec2            pa, pb;
					pa = opus_body_l2w(joint->A, joint->local_a);
					pb = opus_body_l2w(joint->B, joint->local_b);
					opus_pl_line_vec(pl, joint->A->position, pa);
					opus_pl_line_vec(pl, joint->B->position, pb);
					plutovg_set_line_width(pl, 1.4);
					plutovg_set_source_rgba(pl, COLOR_SLATE_GRAY1, 1);
					plutovg_stroke(pl);
					break;
				}
				case OPUS_JOINT_DISTANCE: {
					opus_joint_distance *joint = (void *) g_param.world->joints[i];
					plutovg_circle(pl, joint->anchor.x, joint->anchor.y, 3);
					plutovg_set_source_rgba(pl, COLOR_SLATE_GRAY1, 1);
					plutovg_fill(pl);
					break;
				}
			}
		}

		plutovg_circle(g_param.pl, g_param.pointer.x, g_param.pointer.y, 3);
		plutovg_set_source_rgba(pl, COLOR_RED, 1);
		plutovg_fill(g_param.pl);
	}

	plutovg_restore(g_param.pl);

	/* render on screen */
	g_param.program_pl->use(g_param.program_pl, g_param.pl->surface->data, s_width, s_height);
}

void cleanup(opus_engine *engine)
{
	plutovg_destroy(g_param.pl);
	vg_gl_font_destroy(g_param.font_gl);

	if (g_param.test_SAT) {
		opus_shape_polygon_destroy(g_param.pa);
		opus_shape_polygon_destroy(g_param.pb);
	}

	if (g_param.test_world) {
		opus_physics_world_destroy(g_param.world);
	}
}

void on_key_down(opus_input *input)
{
	if (g_param.test_SAT) {
		if (input->keys_state[GLFW_KEY_Q]) {
			g_param.pa_angle += 0.01;
			opus_rotate(g_param.pa->vertices, g_param.pa->n, opus_vec2_(0, 0), g_param.pa_angle);
		}
	}

	if (g_param.test_world) {
		if (input->keys_state[GLFW_KEY_ENTER]) {
			scanf("%lf", &g_param.world->position_bias);
		}
		if (input->keys_state[GLFW_KEY_R]) {
			opus_engine_stop(g_param.engine);
			opus_engine_end(g_param.engine);
			opus_engine_start(g_param.engine);
		}
		if (input->keys_state[GLFW_KEY_Q]) {
			opus_physics_world_add_rect(g_param.world, g_param.pointer, 60, 60, 0);
		}
		if (input->keys_state[GLFW_KEY_E]) {
			if (g_param.selected) {
				opus_physics_world_remove_body(g_param.world, g_param.selected);
				g_param.selected = NULL;
			}
		}
	}
}

int main(void)
{
	g_param.test_SAT   = 0;
	g_param.test_world = 10;
	if (1) {
		g_param.translation_delta = 10;
		g_param.scale_delta       = 0.02;
		g_param.scale             = 1;
		//
		//				g_param.translation = opus_vec2_(-155.36, -307.614);
		//				g_param.scale       = 1.3;

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
		opus_input_on(g_param.input->on_pointer_move, on_pointer_move);
		opus_engine_start(g_param.engine);
		opus_engine_end(g_param.engine);

		/* release resources */
		vg_gl_program_destroy(g_param.program_gl);
		vg_gl_program_destroy(g_param.program_pl);
		opus_engine_destroy(g_param.engine);
		opus_input_done();
	} else {
	}


	return 0;
}
