#include "opus.h"

struct param {
	/********************  VG_ENGINE  **************************/
	opus_engine     *engine;
	opus_input      *input;
	opus_gl_program *program_gl, *program_pl;
	opus_font       *font_gl;
	plutovg_t       *pl;

	/********************  INTERACTION  *********************/
	opus_real translation_delta, scale_delta;
	opus_vec2 translation;
	opus_real scale;
} g_param = {0};

plutovg_t *pl;
opus_vec2  pp;

void on_pointer_down(opus_input *input) {}

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

	pp = p;
}

void on_scroll(opus_input *input)
{
	if (input->scroll_y > 0) { g_param.scale *= 1 + g_param.scale_delta; }
	if (input->scroll_y < 0) { g_param.scale *= 1 - g_param.scale_delta; }
}

void preload(opus_engine *engine)
{
	plutovg_surface_t *surface = plutovg_surface_create(engine->width, engine->height);

	g_param.pl      = plutovg_create(surface);
	pl              = g_param.pl;
	g_param.font_gl = vg_gl_font_create("../assets/fonts/georgiaz.ttf");

	plutovg_set_font(g_param.pl, plutovg_font_load_from_file("../assets/fonts/consola.ttf", 18));
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

	plutovg_save(g_param.pl);
	plutovg_scale(g_param.pl, g_param.scale, g_param.scale);
	plutovg_translate(g_param.pl, g_param.translation.x, g_param.translation.y);



	plutovg_restore(g_param.pl);

	/* render on screen */
	g_param.program_pl->use(g_param.program_pl, g_param.pl->surface->data, s_width, s_height);
}

void cleanup(opus_engine *engine)
{
	plutovg_destroy(g_param.pl);
	vg_gl_font_destroy(g_param.font_gl);
}

void on_key_down(opus_input *input)
{
}

int main()
{
	if (1) {
		g_param.translation_delta = 10;
		g_param.scale_delta       = 0.02;
		g_param.scale             = 1;

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
