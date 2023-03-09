#include "vg/vg_color.h"
#include "vg/vg_engine.h"
#include "vg/vg_input.h"
#include "vg/vg_utils.h"
#include "vg/vg_gl.h"
#include "vg/pluto/plutovg-private.h"
#include "vg/pluto/plutovg.h"

vg_engine_t     *g_engine  = NULL;
vg_input_t      *g_input   = NULL;
vg_gl_program_t *g_program = NULL;
vg_gl_font_t    *g_font    = NULL;
vg_gl_program_t *g_pl_r    = NULL;

plutovg_surface_t *g_pl_s = NULL;
plutovg_t         *g_pl   = NULL;

void on_pointer_down(vg_input_t *input)
{
}

void on_key_down(vg_input_t *input)
{
}

void preload(vg_engine_t *engine)
{
	g_pl_s = plutovg_surface_create(engine->width, engine->height);
	g_pl   = plutovg_create(g_pl_s);
	g_font = vg_gl_font_create("../assets/fonts/georgiaz.ttf");
}

void update(vg_engine_t *engine, real delta)
{

}

void render(vg_engine_t *engine)
{
	vg_t *vg = engine->vg;

	glClearColor(COLOR_WHITE, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void cleanup(vg_engine_t *engine)
{
	plutovg_surface_destroy(g_pl_s);
	plutovg_destroy(g_pl);
	vg_gl_font_destroy(g_font);
}

int main()
{
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


	return 0;
}
