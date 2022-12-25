/**
 * @file render_engine.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/6/12
 *
 * @example
 *
 * @development_log
 *
 */



#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

#include "engine/engine.h"
#include "data_structure/array.h"
#include "render/color.h"
#include "utils/utils.h"

const char *engine_event_before_update = "engine_event_before_update";
const char *engine_event_after_update  = "engine_event_after_update";
const char *engine_event_before_render = "engine_event_before_render";
const char *engine_event_after_render  = "engine_event_after_render";
const char *engine_event_on_cleanup    = "engine_event_on_cleanup";
const char *engine_event_on_destroy    = "engine_event_on_destroy";

const char *engine_event_on_pointer_move = "engine_event_on_pointer_move";
const char *engine_event_on_pointer_down = "engine_event_on_pointer_down";
const char *engine_event_on_pointer_up   = "engine_event_on_pointer_up";
const char *engine_event_on_drag         = "engine_event_on_drag";
const char *engine_event_on_key_press    = "engine_event_on_key_press";
const char *engine_event_on_key_down     = "engine_event_on_key_down";
const char *engine_event_on_key_up       = "engine_event_on_key_up";
const char *engine_event_on_scroll       = "engine_event_on_scroll";

const char *engine_font_consola_path = "../../../assets/fonts/consola.ttf";

#ifdef WIN32
#include <windows.h>
double engine_get_time()
{
	LARGE_INTEGER t, f;
	QueryPerformanceCounter(&t);
	QueryPerformanceFrequency(&f);
	return (double) t.QuadPart / (double) f.QuadPart;
}
#else


#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>


double engine_get_time()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec + t.tv_usec * 1e-6;
}

#endif

void engine_glfw_error_callback_(int error, const char *description)
{
	ERROR_("%s\n", description);
}

static INLINE void engine_init_glfw_context(engine_t *eng, int width, int height, const char *title)
{
	GLFWwindow *window;
	int         major, minor, revision;

	/* init glfw */
	if (!glfwInit()) {
		ERROR_("Failed to initialize GLFW library\n");
		exit(-1);
	}

	/* create glfw window */
	/* create glfw window */
	glfwDefaultWindowHints();
	window      = glfwCreateWindow(width, height, title, NULL, NULL);
	eng->window = window;
	if (window == NULL) {
		ERROR_("Could not create glfw window\n");
		glfwTerminate();
		exit(-1);
	}

	glfwMakeContextCurrent(window);

	/* load openGL functions */
	if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
		ERROR_("Could not load GL functions\n");
		glfwDestroyWindow(window);
		glfwTerminate();
		exit(-1);
	}

	/* set error callback about glfw */
	glfwSetErrorCallback(engine_glfw_error_callback_);

	glfwSwapInterval(0);
	glfwSetTime(0);

	/* give version hint */
	glfwGetVersion(&major, &minor, &revision);
	INFO("Engine initialized! Running against GLFW %i.%i.%i\n", major, minor, revision);
	INFO("Create window of size %dx%d\n", width, height);
}

static plugin_t plugin_(char *name, char *dependencies, plugin_boot_cb boot, plugin_shutdown_cb shutdown)
{
	plugin_t plugin;
	strncpy(plugin.name, name, PLUGIN_MAX_NAME_SIZE);
	if (dependencies != NULL)
		strncpy(plugin.dependencies, dependencies, PLUGIN_MAX_DEPENDENCIES_SIZE * PLUGIN_MAX_NAME_SIZE);
	plugin.boot_     = boot;
	plugin.shutdown_ = shutdown;
	return plugin;
}

engine_t *engine_create(int win_width, int win_height, const char *title)
{
	static plugin_t plugins_registry[2];

	engine_t       *eng = (void *) malloc(sizeof(engine_t));

	engine_init_glfw_context(eng, win_width, win_height, title);

	eng->width          = win_width;
	eng->height         = win_height;
	eng->frame_interval = 1.0 / 80.0;
	eng->is_running     = 0;
	eng->preload        = NULL;
	eng->render         = NULL;
	eng->update         = NULL;
	eng->cleanup        = NULL;

	vec2_set(eng->pointer, 0, 0);

	eng->ctrl_key  = 0;
	eng->alt_key   = 0;
	eng->meta_key  = 0;
	eng->shift_key = 0;

	plutovg_matrix_init_identity(&eng->camera);

	eng->event_hub             = event_hub_create();
	eng->event_hub->context    = eng;
	eng->event_data.event_flag = 0;
	eng->event_data.event_type = 0;
#ifdef __EMSCRIPTEN__
	eng->event_data.mouse_event = NULL;
	eng->event_data.touch_event = NULL;
#endif /* __EMSCRIPTEN__ */
	memset(eng->keys_state, 0, sizeof(char) * ENGINE_KEYS_SIZE);
	eng->key_pressed_count = 0;

	/* pre-register plugins */
	{
		extern void vector_graphics_boot_plugin(engine_t *, plugin_t *, ...);
		extern void vector_graphics_shutdown_plugin(engine_t *, plugin_t *);
		extern void input_manager_boot_plugin(engine_t *, plugin_t *, ...);
		plugins_registry[0] = plugin_("vector_graphics", NULL, &vector_graphics_boot_plugin, &vector_graphics_shutdown_plugin);
		plugins_registry[1] = plugin_("input_manager", NULL, &input_manager_boot_plugin, NULL);

		array_create(eng->plugins_, sizeof(plugin_t));
		array_resize(eng->plugins_, 2);
		memcpy(eng->plugins_, plugins_registry, sizeof(plugin_t) * 2);
	}

	/* loop through all the plugins and boot them all */
	if (array_len(eng->plugins_) > 0) {
		uint64_t i, n = array_len(eng->plugins_);
		/* TODO: account for dependency relations */
		for (i = 0; i < n; ++i)
			if (eng->plugins_[i].boot_) eng->plugins_[i].boot_(eng, &eng->plugins_[i]);
	}

	return eng;
}

void engine_destroy(engine_t *eng)
{
	uint64_t i;

	/* final emission of the engine */
	event_hub_emit(eng->event_hub, engine_event_on_destroy, eng);

	/* shutdown plugins */
	for (i = 0; i < array_len(eng->plugins_); i++)
		if (eng->plugins_[i].shutdown_) eng->plugins_[i].shutdown_(eng, &eng->plugins_[i]);
	array_destroy(eng->plugins_);

	/* destroy windows */
	glfwDestroyWindow(eng->window);
	glfwTerminate();

	/* destroy event hub */
	event_hub_destroy(eng->event_hub);

	free(eng);
}

void engine_set_preload(engine_t *eng, engine_render_t preload)
{
	eng->preload = preload;
}

void engine_set_render(engine_t *eng, engine_render_t render)
{
	eng->render = render;
}

void engine_set_update(engine_t *eng, engine_update_t update)
{
	eng->update = update;
}

void engine_set_cleanup(engine_t *eng, engine_cleanup_t cleanup)
{
	eng->cleanup = cleanup;
}

void engine_preload(engine_t *eng)
{
	if (eng->preload) eng->preload(eng);
}

void engine_render(engine_t *eng)
{
	if (eng->render) eng->render(eng);
}

void engine_update(engine_t *eng)
{
	if (eng->update) eng->update(eng, eng->elapsed_time);
}

void engine_cleanup(engine_t *eng)
{
	event_hub_emit(eng->event_hub, engine_event_on_cleanup, eng);
	if (eng->cleanup) eng->cleanup(eng);
	INFO("Exit Engine\n");
}

void engine_stop(engine_t *eng)
{
	eng->is_running = 0;
}

static engine_t *engine_iter_eng_ref = NULL;
static void      engine_iter()
{
	engine_t     *eng = engine_iter_eng_ref;
	static double next_frame;
	static float  avg_duration = 0.f;
	static float  alpha        = 1.f / 100.f; /* sampling_count=100 */
	GLenum        err;

	for (; (err = glGetError()) != GL_NO_ERROR;)
		ERROR_("OpenGL Error Code %d appeared, please check for further information\n", err);

	/* define the viewport dimensions */
	glViewport(0, 0, eng->width, eng->height);

	glUseProgram(eng->shader_program);
	glClearColor(COLOR_ANTIQUE_WHITE, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	eng->current_time = engine_get_time();

	/* TODO: ensure constant frame interval */

	/* calculate FPS */
	{
		eng->frame_count++;
		eng->elapsed_time = eng->current_time - eng->last_time;

		if (1 == eng->frame_count)
			avg_duration = (float) eng->elapsed_time;
		else
			avg_duration = avg_duration * (1 - alpha) + (float) eng->elapsed_time * alpha;
		eng->fps = 1. / avg_duration;
	}
	if (eng->key_pressed_count > 0)
		event_hub_emit(eng->event_hub, engine_event_on_key_press, eng);

	event_hub_emit(eng->event_hub, engine_event_before_update, eng);
	engine_update(eng);
	event_hub_emit(eng->event_hub, engine_event_after_update, eng);

	event_hub_emit(eng->event_hub, engine_event_before_render, eng);
	engine_render(eng); /* render */
	if (eng->draw_data_to_current_frame) {
		GLint t = glGetUniformLocation(eng->shader_program, "texture1");
		glBindTexture(GL_TEXTURE_2D, eng->texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, eng->data_width, eng->data_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, eng->data);
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(t, 0);
		glBindVertexArray(eng->VAO);
		/*glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);*/ /* FIXME: why this cannot work? */
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}
	event_hub_emit(eng->event_hub, engine_event_after_render, eng);

	/* swap the screen buffers */
	glfwSwapBuffers(eng->window);
	/* poll for and process events */
	glfwPollEvents();
	eng->last_time = eng->current_time;
}

void engine_start(engine_t *eng)
{
	INFO("Start engine now\n");
	engine_preload(eng);

	eng->is_running     = 1;
	eng->frame_count    = 0;
	eng->last_time      = engine_get_time();
	eng->start_time     = eng->last_time;
	eng->elapsed_time   = 0.0;
	eng->fps            = 0.0;
	engine_iter_eng_ref = eng;
#ifndef __EMSCRIPTEN__
	while (!glfwWindowShouldClose(eng->window) && eng->is_running)
		engine_iter();
#else
	emscripten_set_main_loop(engine_iter, 0, 1);
#endif /* __EMSCRIPTEN__ */
	engine_cleanup(eng);
}

void engine_get_world_coord_from_screen_coord(engine_t *eng, real sx, real sy, real *wx, real *wy)
{
	plutovg_matrix_t inv;
	double           rx, ry;
	memcpy(&inv, &eng->camera, sizeof(plutovg_matrix_t));
	plutovg_matrix_invert(&inv);
	plutovg_matrix_map(&inv, sx, sy, &rx, &ry);
	*wx = (real) rx;
	*wy = (real) ry;
}
