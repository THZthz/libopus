#include <stdio.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include "external/emscripten/emscripten.h"
#include "external/emscripten/emscripten/html5.h"
#endif /* __EMSCRIPTEN__ */

#include "data_structure/array.h"
#include "engine/engine.h"
#include "math/geometry.h"
#include "physics/algorithm/bvh.h"
#include "physics/physics.h"
#include "render/color.h"
#include "render/pluto/plutovg-private.h"
#include "render/pluto/plutovg.h"
#include "render/render_utils.h"
#include "utils/utils.h"
#include "utils/event.h"


/* render context */
int                is_running      = 0;
plutovg_surface_t *vg_surface      = NULL;
plutovg_t         *vg              = NULL;
plutovg_font_t    *vg_consola_font = NULL;
physics_engine_t  *phy_eng         = NULL;
body_t           **g_stacks        = NULL;
body_t            *g_walls[4], *b1, *b2;
bvh_tree_t        *bvh_tree   = NULL;
char               input[512] = {0};
int                input_len  = 0;

void preload(engine_t *eng);
void update(engine_t *eng, double delta);
void render(engine_t *eng);
void cleanup(engine_t *eng);

collision_t *get_c(uint64_t a, uint64_t b)
{
	uint32_t i;
	for (i = 0; i < array_len(phy_eng->detector->collisions); i++) {
		collision_t *c = phy_eng->detector->collisions[i];
		if (c->body_a->id == a && c->body_b->id == b) return c;
	}
	return NULL;
}

int on_pointer_down(event_hub_t *hub, event_t *e, void *args)
{
	static body_t *selected_body         = NULL;
	static int     select_and_click_once = 0;

	engine_t *eng        = args;
	body_t  **all_bodies = composite_all_bodies(phy_eng->world);
	uint64_t  i;
	/*for (i = 0; i < array_len(all_bodies); i++) {
	    body_t *b = all_bodies[i];
	    if (b->id == 1) {
	        body_set_position(b, eng->pointer);
	    }
	}*/
	for (i = 0; i < array_len(all_bodies); i++) {
		body_t *b = all_bodies[i];
		if (selected_body != NULL && select_and_click_once == 0) {
			body_set_position(selected_body, eng->pointer);
			select_and_click_once = 1;
			selected_body         = NULL;
			return 0;
		} else if (vertices_contains(b->vertices, array_len(b->vertices), eng->pointer, sizeof(vertex_t))) {
			selected_body         = b;
			select_and_click_once = 0;
			return 0;
		}
	}

	selected_body = 0;

	return 0;
}

static int on_key_down(event_hub_t *hub, event_t *e, void *args)
{
	engine_t *eng = args;
	if (eng->keys_state[GLFW_KEY_SPACE]) {
		is_running = !is_running;
	} else if (eng->keys_state[GLFW_KEY_BACKSPACE] && input_len > 0) {
		input[input_len - 1] = 0;
		input_len--;
	} else if (eng->keys_state[GLFW_KEY_ENTER]) {
		int          a, b;
		collision_t *c;
		sscanf(input, "%d,%d", &a, &b);
		a = r_min(a, b);
		b = r_max(a, b);
		c = get_c(a, b);
		if (!c) printf("no collision\n");
		else
			printf("supports %d\n", c->n_supports);
	} else if (eng->keys_state[GLFW_KEY_Q]) {
		body_set_angle(b1, b1->angle + 0.2);
	}

	return 0;
}

void character_callback(GLFWwindow *window, unsigned int codepoint)
{
	if ((codepoint >= 'A' && codepoint <= 'Z') || (codepoint >= 'a' && codepoint <= 'z') ||
	    (codepoint >= '0' && codepoint <= '9') || codepoint == ' ' || codepoint == ',') {
		input[input_len++] = codepoint;
		input[input_len]   = '\0';
	}
}

int main()
{
	event_cb  l[2];
	engine_t *engine = NULL;
	int       width = 800, height = 800;

#ifdef __EMSCRIPTEN__
	printf("INFO::EMSCRIPTEN SPECIFIED WHEN COMPILING\n");
#endif
#ifdef __EMSCRIPTEN_ON_MOBILE__
	printf("INFO::EMSCRIPTEN_ON_MOBILE SPECIFIED WHEN COMPILING\n");
#endif

	/*int i = 0;
	preload(NULL);
	while (i++ < 100) physics_engine_tick(phy_eng);
	cleanup(NULL);*/

	engine = engine_create(width, height, "sandbox");
	engine_set_preload(engine, preload);
	engine_set_update(engine, update);
	engine_set_render(engine, render);
	engine_set_cleanup(engine, cleanup);
	l[0] = &on_pointer_down;
	event_hub_on(engine->event_hub, engine_event_on_pointer_down, event_create(l, 1, -1, engine));
	l[0] = &on_key_down;
	event_hub_on(engine->event_hub, engine_event_on_key_down, event_create(l, 1, -1, engine));
	glfwSetCharCallback(engine->window, character_callback);
	engine_start(engine);
	engine_destroy(engine);

	return 0;
}

void preload(engine_t *eng)
{
	if (eng != NULL) {
		/* load rendering context */
		eng->data_width                 = eng->width;
		eng->data_height                = eng->height;
		eng->draw_data_to_current_frame = 1; /* draw data to screen */

		vg_surface = plutovg_surface_create(eng->width, eng->height);
		vg         = plutovg_create(vg_surface);

		eng->data = vg_surface->data;

		/* load font */
		char *consola_font_file_path = "../assets/fonts/consola.ttf";
#ifdef __EMSCRIPTEN__
		consola_font_file_path = "consola.ttf";
#endif
		vg_consola_font = plutovg_font_load_from_file(consola_font_file_path, 18);
		plutovg_set_font(vg, vg_consola_font);
	}

	{
		uint64_t i;
		int      x, y;
		real     w = 30, gap = 5;
		body_t **all_bodies;

		phy_eng = physics_engine_create();

		array_create(g_stacks, sizeof(body_t *));
		array_resize(g_stacks, 5 * 5);
		for (y = 0; y < 4; y++) {
			for (x = 0; x < 4; x++) {
				body_t *r = common_rectangle(vec2_(x * (w + gap) + 100, y * (w + gap) + 100), w, w);
				array_push(g_stacks, &r);
				composite_add_body(phy_eng->world, r);
			}
		}

		b1            = body_create();
		b2            = body_create();
		point_t v1[4] = {{439, 109}, {400, 126}, {416, 192}, {501, 116}};
		point_t v2[5] = {{244, 161}, {327, 164}, {336, 270}, {261, 323}, {138, 245}};
		body_set_vertices(b1, vertices_create(v1, 4, b1));
		body_set_position(b1, vec2_(200, 100));
		body_set_vertices(b2, vertices_create(v2, 5, b2));
		body_set_position(b2, vec2_(100, 100));
		composite_add_body(phy_eng->world, b1);
		composite_add_body(phy_eng->world, b2);

		/* walls: down up left right */
		g_walls[0] = common_rectangle(vec2_(400, 780), 780, 20);
		g_walls[1] = common_rectangle(vec2_(400, 10), 780, 20);
		g_walls[2] = common_rectangle(vec2_(10, 400), 20, 780);
		g_walls[3] = common_rectangle(vec2_(780, 400), 20, 780);
		/*body_rotate(g_walls[0], 0.1, vec2_(0, 0), 0);*/
		for (y = 0; y < 4; y++) {
			body_set_static(g_walls[y], 1);
			composite_add_body(phy_eng->world, g_walls[y]);
		}

		phy_eng->gravity.y       = 1;
		phy_eng->gravity_scale   = 0.0001;
		phy_eng->enable_sleeping = 0;
		phy_eng->is_fixed        = 1;

		/*bvh_tree = bvh_tree_create();
		all_bodies = composite_all_bodies(phy_eng->world);
		for (i = 0; i < array_len(all_bodies); i++) {
		    bvh_tree_insert_leaf(bvh_tree, all_bodies[i], 0);
		}*/
	}
}

void update(engine_t *eng, double delta)
{
	uint64_t i;
	body_t **all_bodies;
	double   s;

	if (is_running) {
		physics_engine_tick(phy_eng);
	}


	/*s = engine_get_time();
	all_bodies= composite_all_bodies(phy_eng->world);
	bvh_tree_destroy(bvh_tree);
	bvh_tree = bvh_tree_create();
	for (i = 0; i < array_len(all_bodies); i++) {
	    bvh_tree_insert_leaf(bvh_tree, all_bodies[i], 0);
	}*/
	/*INFO("%f\n", engine_get_time() - s);*/
}

static void draw_info(engine_t *eng)
{
	vec2 p;
	char text[1024];

	engine_get_world_coord_from_screen_coord(eng, eng->pointer.x, eng->pointer.y, &p.x, &p.y);
	sprintf(text,
	        "FPS: %.2lf %.5lf\n"
	        "World Coordinate: (%.2lf, %.2lf)\n",
	        eng->fps, eng->elapsed_time, p.x, p.y);

	plutovg_set_font_size(vg, 18);
	painter_text_box(vg, text, -1, 450, 10, -1, -1, 0, 0, 1, 0);
	plutovg_set_source_rgb(vg, COLOR_BLACK);
	plutovg_fill(vg);
}

static void display_collisions_info(engine_t *eng, collision_t **collisions, real x, real y, real tile_len, real margin)
{
	real     ox = x + margin, oy = y + margin, w = tile_len;
	uint64_t i, max_id = body_max_id();

	/* background */
	plutovg_rect(vg, ox - margin, oy - margin, w * (real) max_id + 2 * margin, w * (real) max_id + 2 * margin);
	plutovg_set_source_rgb(vg, COLOR_BLACK);
	plutovg_fill(vg);

	/* border */
	plutovg_rect(vg, ox - margin, oy - margin, w * (real) max_id + 2 * margin, w * (real) max_id + 2 * margin);
	plutovg_set_source_rgb(vg, COLOR_WHITE);
	plutovg_stroke(vg);

	/* gray tile means there exists a collision between respondent bodies */
	for (i = 0; i < array_len(collisions); i++) {
		plutovg_rect(vg, (real) collisions[i]->body_a->id * w + ox - w, (real) collisions[i]->body_b->id * w + oy - w, w, w);
	}
	plutovg_set_source_rgb(vg, COLOR_GREEN2);
	plutovg_fill(vg);

	/* vertical separation line */
	for (i = 0; i < max_id; i++) {
		if (i % 5 == 0) {
			char text[16];
			snprintf(text, 16, "%" PRIu64, i + 1);
			plutovg_text(vg, text, ox + w * (real) i, oy - w);
		}

		plutovg_move_to(vg, ox + w * (real) i, oy);
		plutovg_line_to(vg, ox + w * (real) i, oy + (real) max_id * w);
		plutovg_set_source_rgba(vg, COLOR_GRAY81, i % 5 ? 0.5 : 1);
		plutovg_stroke(vg);
	}

	/* horizontal separation line */
	for (i = 0; i < body_max_id(); i++) {
		if (i % 5 == 0) {
			char text[16];
			snprintf(text, 16, "%" PRIu64, i + 1);
			plutovg_text(vg, text, ox - w, oy + w * (real) i + w);
		}

		plutovg_move_to(vg, ox, oy + w * (real) i);
		plutovg_line_to(vg, ox + (real) max_id * w, oy + w * (real) i);
		plutovg_set_source_rgba(vg, COLOR_GRAY81, i % 5 ? 0.5 : 1);
		plutovg_stroke(vg);
	}
}

static void draw_body_wireframe(body_t *body)
{
	size_t j;
	plutovg_move_to(vg, body->vertices[0].x, body->vertices[0].y);
	for (j = 1; j < array_len(body->vertices); j++) {
		plutovg_line_to(vg, body->vertices[j].x, body->vertices[j].y);
	}
	plutovg_close_path(vg);
	plutovg_stroke(vg);
}

void render(engine_t *eng)
{
	body_t **all_bodies = composite_all_bodies(phy_eng->world);
	uint64_t i, j;

	memset(eng->data, 255, eng->width * eng->height * 4);

	plutovg_set_source_rgb(vg, COLOR_BLACK);
	for (i = 0; i < array_len(all_bodies); i++) {
		body_t *b = all_bodies[i];
		char    text[128];

		draw_body_wireframe(b);

		/* draw vertex id */
		/*plutovg_set_font_size(vg, 13);
		for (j = 0; j < array_len(b->vertices); j++) {
		    sprintf(text, "%d", j);
		    plutovg_text(vg, text, b->vertices[j].x, b->vertices[j].y);
		}
		plutovg_fill(vg);*/

		/* draw body's ID */
		sprintf(text, "%s%" PRIu64 "", b->is_sleeping ? "s" : "", b->id);
		plutovg_set_font_size(vg, 10);
		plutovg_text(vg, text, b->position.x, b->position.y);
	}

	for (i = 0; i < array_len(phy_eng->detector->collisions); i++) {
		collision_t *c = phy_eng->detector->collisions[i];
		for (j = 0; j < c->n_supports; j++) {
			real sx = c->supports[j].x, sy = c->supports[j].y;
			real nx = c->normal.x, ny = c->normal.y;
			vec2 v;
			char t[512];
			vec2_set(v, nx, ny);
			vec2_set_length(v, 10);
			plutovg_circle(vg, sx, sy, 2.3);
			plutovg_set_source_rgb(vg, COLOR_RED);
			plutovg_fill(vg);
			/*plutovg_move_to(vg, sx, sy);
			plutovg_line_to(vg, sx + v.x, sy + v.y);
			plutovg_stroke(vg);

			sprintf(t, "id_a:%u", c->body_a->id);
			plutovg_text(vg, t, sx + 10, sy + 10);
			plutovg_fill(vg);*/
		}
	}

	if (!is_running) {
		extern vertex_t supports[2];
		collision_t    *c = collision_collides(b1, g_walls[0], NULL);
		vec2            v = {0};
		char            text[128];
		if (c) {
			vec2_set(v, c->normal.x, c->normal.y);
			vec2_set_length(v, 10);
			sprintf(text, "%d", c->body_a->id);
			plutovg_set_font_size(vg, 13);
			plutovg_text(vg, text, supports[0].x, supports[0].y);
			plutovg_set_source_rgb(vg, COLOR_BLACK);
			plutovg_fill(vg);
			collision_destroy(c);
		}

		plutovg_set_source_rgb(vg, COLOR_GREEN2);
		draw_body_wireframe(b1);
		plutovg_set_source_rgb(vg, COLOR_BLACK);
		draw_body_wireframe(g_walls[0]);

		plutovg_circle(vg, supports[0].x, supports[0].y, 3);
		//		plutovg_circle(vg, supports[1].x, supports[1].y, 3);
		plutovg_set_source_rgb(vg, COLOR_BLUE);
		plutovg_fill(vg);

		plutovg_move_to(vg, supports[0].x, supports[0].y);
		plutovg_line_to(vg, supports[0].x + v.x, supports[0].y + v.y);
		plutovg_stroke(vg);
	}

	/*bvh_tree_draw(vg, bvh_tree);*/

	/*display_collisions_info(eng, phy_eng->detector->collisions, 20, 20, 8, 20);*/

	draw_info(eng);

	plutovg_font_set_size(vg_consola_font, 25);
	plutovg_set_source_rgb(vg, COLOR_RED);
	plutovg_text(vg, input, 50, 50);
	plutovg_fill(vg);
}

void cleanup(engine_t *eng)
{
	if (eng != NULL) {
		plutovg_font_destroy(vg_consola_font);
		plutovg_surface_destroy(vg_surface);
		plutovg_destroy(vg);
	}

	/* release memory related to physics engine */
	{
		uint64_t i;
		body_t **all_bodies = composite_all_bodies(phy_eng->world);
		INFO("destroy %" PRIu64 " bodies\n", array_len(all_bodies));
		for (i = 0; i < array_len(all_bodies); i++) body_destroy(all_bodies[i]);
		array_destroy(g_stacks);
		physics_engine_destroy(phy_eng);

		/*bvh_tree_destroy(bvh_tree);*/
	}
}
