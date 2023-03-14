/**
 * @file brain_demo.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/10/19
 *
 * @example
 *
 * @development_log
 *
 */


#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "brain/lstm.h"
#include "brain/ann.h"
#include "math/math.h"
#include "utils/utils.h"
#include "vg/vg_color.h"
#include "engine/engine.h"
#include "engine/input.h"
#include "vg/vg_utils.h"
#include "vg/vg_gl.h"
#include "vg/pluto/plutovg-private.h"
#include "vg/pluto/plutovg.h"

opus_engine     *g_engine  = NULL;
opus_input      *g_input   = NULL;
opus_gl_program *g_program = NULL;
opus_font       *g_font    = NULL;
opus_gl_program *g_pl_r    = NULL;

plutovg_surface_t *g_pl_s = NULL;
plutovg_t         *g_pl   = NULL;

#define MAX_SAMPLE_SIZE (500)
#define MAX_ITERATION (100000)
#define N 100


void on_pointer_down(opus_input *input)
{
}

void on_key_down(opus_input *input)
{
}

void preload(opus_engine *engine)
{
	g_pl_s = plutovg_surface_create(engine->width, engine->height);
	g_pl   = plutovg_create(g_pl_s);
	g_font = vg_gl_font_create("../assets/fonts/georgiaz.ttf");

	/* handle glfw window input */
	g_input = opus_input_init(g_engine);
	opus_input_on(g_input->on_pointer_down, on_pointer_down);
	opus_input_on(g_input->on_key_down, on_key_down);
}

void update(opus_engine *engine, opus_real delta)
{
}

double get_output(size_t i)
{
	return cos(2.0 * 3.14 / N * 5.0 * (double) i);
}

double get_input(size_t i)
{
	return sin(2.0 * 3.14 / N * 5.0 * (double) i);
}

void render(opus_engine *engine)
{
	static int     should_run_lstm = 1;
	static opus_lstm_unit *u;

	opus_vg *vg = engine->vg;
	size_t i;

	glClearColor(COLOR_WHITE, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glEnable(GL_MULTISAMPLE);

	double input[N], output[N] = {0}, err;

	for (i = 0; i < N; i++) {
		input[i] = get_input(i);
	}

	if (should_run_lstm) {
		u = opus_lstm_unit_create(N);
		for (i = 0; i < N; i++) {
			u->x[i] = input[i];
			u->hat_h[i] = get_output(i);
		}

		for (i = 0; i < 4000; i++) {
			opus_lstm_unit_run(u);
			opus_lstm_unit_train(u, 0.01);
		}

		opus_lstm_unit_save(u, "LSTM.net");

		should_run_lstm = 0;
	}

	{
		opus_real w = 500, h = 100, sx = 100, sy = 500, x;
		opus_real gap = w / N;

		opus_real max_output = 1;

		opus_lstm_unit_predict(u, input, output);

		x = sx;
		opus_vg_begin(vg);
		for (i = 0; i < N; i++) {
			if (i == 0) opus_vg_move_to(vg, x, sy - get_output(i) / max_output * h);
			else
				opus_vg_line_to(vg, x, sy - get_output(i) / max_output * h);
			x += gap;
		}
		opus_vg_end_stroke_path(vg);
		vg_gl_p1_render_stroke(g_program, vg, COLOR_BLACK, 1);

		x = sx;
		opus_vg_begin(vg);
		for (i = 0; i < N; i++) {
			if (i == 0) opus_vg_move_to(vg, x, sy - output[i] / max_output * h);
			else
				opus_vg_line_to(vg, x, sy - output[i] / max_output * h);
			x += gap;
		}
		opus_vg_end_stroke_path(vg);
		vg_gl_p1_render_stroke(g_program, vg, COLOR_CYAN, 1);
	}
}

void cleanup(opus_engine *engine)
{
	plutovg_surface_destroy(g_pl_s);
	plutovg_destroy(g_pl);
	vg_gl_font_destroy(g_font);

	opus_input_done();
}

void run_test(const char *name, void (*func)())
{
	printf("\n=======================[%s]=======================\n", name);
	func();
}

void create_engine()
{
	/* engine to create glfw window and a combined "vg_t" instance */
	g_engine = opus_engine_create(800, 800, "vg");

	/* specially designed to draw [double, double, ...] like path(tessellate to triangle and draw with opengl) */
	g_program = vg_gl_program_preset1();

	/* specially designed to render pluto_vg surface buffer */
	g_pl_r = vg_gl_program_preset2();

	opus_engine_set_callback(g_engine, preload, update, render, cleanup);
}

void destroy_engine()
{
	/* release resources */
	vg_gl_program_destroy(g_program);
	vg_gl_program_destroy(g_pl_r);
	opus_engine_destroy(g_engine);
}

int main()
{
	void xor ();
	void predefined_func();
	void lstm_test();

	create_engine();

	opus_engine_start(g_engine);
	/*lstm_test();*/

	/*run_test("xor", xor);
	run_test("predefined_func", predefined_func);*/

	destroy_engine();

	return 0;
}

/* use this function to produce input data */
static opus_real f(opus_real x1, opus_real x2, opus_real x3)
{
	return opus_clamp(x1 * x2 + x3 / 2, -1, 1);
}

/* calculate the error between the desired output and the network output */
static opus_real calc_err(opus_real desired, opus_real actual)
{
	return fabs(actual - desired);
}

void predefined_func()
{
	opus_ann   *net, *best = NULL;
	uint32_t nmap[4] = {3, 4, 3, 1};
	opus_real   input[MAX_SAMPLE_SIZE][3], output[MAX_SAMPLE_SIZE];
	opus_real   required_err = 0.01, cur_err = FLT_MAX, best_err = FLT_MAX, max_err, min_err;

	uint32_t i, j;

	/* generate sample data */
	for (i = 0; i < MAX_SAMPLE_SIZE; i++) {
		opus_real x1 = opus_clamp((opus_real) rand() / RAND_MAX, -1, 1);
		opus_real x2 = opus_clamp((opus_real) rand() / RAND_MAX, -1, 1);
		opus_real x3 = opus_clamp((opus_real) rand() / RAND_MAX, -1, 1);
		opus_real y  = f(x1, x2, x3);

		input[i][0] = x1;
		input[i][1] = x2;
		input[i][2] = x3;
		output[i]   = y;
	}

	net = opus_ann_create(4, nmap);
	opus_ann_randomize(net);

	/* continue training when the network miss the target too much */
	j = 0;
	while (cur_err > required_err && j++ < MAX_ITERATION) {
		opus_real max = 0, min = FLT_MAX;

		/* feed all the data into the network */
		for (i = 0; i < MAX_SAMPLE_SIZE; i++)
			opus_ann_learn(net, input[i], output + i, 2.5);

		/* calculate average error */
		cur_err = 0;
		for (i = 0; i < MAX_SAMPLE_SIZE; i++) {
			opus_real err = calc_err(output[i], *opus_ann_predict(net, input[i]));
			cur_err += err;

			if (err > max) max = err;
			if (err < min) min = err;
		}
		cur_err /= MAX_SAMPLE_SIZE;

		if (cur_err < best_err) {
			if (best) opus_ann_destroy(best);
			best     = opus_ann_get_copy(net);
			best_err = cur_err;
			max_err  = max;
			min_err  = min;
		} else if (cur_err > best_err * 2.2) {
			/* if we get worse result */
			printf("randomize net at err %f\n", cur_err);
			opus_ann_randomize(net);
		}
	}

	printf("\ntraining exit\n");
	printf("avg err %f, max %f, min %f, %" PRIu32 " iterations in total.\n",
	       best_err, max_err, min_err, j);

	printf("\tsome tests:\n");
	printf("\t\t%8s %8s %8s     %8s   %8s\n", "input1", "input2", "input3", "output", "prediction");
	for (i = 0; i < 5; i++)
		printf("\t\t%.6f %.6f %.6f --> %.6f : %.6f\n", input[i][0], input[i][1], input[i][2], output[i], *opus_ann_predict(best, input[i]));

	printf("\nthe network information: \n");
	opus_ann_console(net);

	opus_ann_destroy(net);
	if (best) opus_ann_destroy(best);
}

void lstm_test(void)
{
}

void xor () {
	opus_ann   *net;
	uint32_t nmap[]      = {2, 3, 1};
	opus_real   input[4][2] = {
	        {0, 1},
	        {1, 0},
	        {1, 1},
	        {0, 0}};
	opus_real output[4] = {1, 1, 0, 0};
	uint32_t i, j;

	net = opus_ann_create(3, nmap);
	opus_ann_randomize(net);

	for (i = 0; i < 900; i++)
		for (j = 0; j < 4; j++)
			opus_ann_learn(net, input[j], output + j, 4);

	printf("\ntrained network %dx%d times in total\n", 4, 900);
	printf("final result:\n");
	printf("\t%8s  %8s  %8s  %8s\n", "input1", "input2", "output", "network_output");

	for (i = 0; i < 4; i++) {
		printf("\t%.6f  %.6f  %.6f  %.6f\n", input[i][0], input[i][1], output[i], *opus_ann_predict(net, input[i]));
	}

	printf("\nthe network information: \n");
	opus_ann_console(net);

	opus_ann_destroy(net);
}
