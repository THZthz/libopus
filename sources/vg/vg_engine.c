/**
 * @file vg_engine.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/12/27
 *
 */

#include <string.h>

#include "vg/vg_color.h"
#include "external/sokol_time.h"
#include "utils/utils.h"
#include "vg/vg_engine.h"

/* glfw: whenever the window size changed (by OS or user resize) this callback function executes */
void framebuffer_size_(GLFWwindow* window, int width, int height)
{
	/* make sure the viewport matches the new window dimensions; note that width and */
	/* height will be significantly larger than specified on retina displays */
	glViewport(0, 0, width, height);
}

static void create_glfw_window_(vg_engine_t *engine)
{
	GLFWwindow *window;

	/* create glfw window */
	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_SAMPLES, 4);
	window = glfwCreateWindow(engine->width, engine->height, engine->title, NULL, NULL);
	if (window == NULL) {
		OPUS_ERROR("Could not create glfw window\n");
		glfwTerminate();
		exit(-1);
	}
	engine->window = window;

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_);
	OPUS_INFO("window with size %dx%d has been created\n", engine->width, engine->height);

	/* load openGL functions */
	if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
		OPUS_ERROR("Could not load GL functions\n");
		glfwDestroyWindow(window);
		glfwTerminate();
		exit(-1);
	}

	/* set error callback about glfw */
	void error_(int error, const char *description);
	glfwSetErrorCallback(error_);

	glfwSwapInterval(0);
	glfwSetTime(0);
}

static void init_glfw_(vg_engine_t *engine)
{
	int major, minor, revision;

	/* init glfw */
	if (!glfwInit()) {
		OPUS_ERROR("Failed to initialize GLFW library\n");
		exit(-1);
	}

	engine->window = NULL;

	/* give version hint */
	glfwGetVersion(&major, &minor, &revision);
	OPUS_INFO("engine initialized with GLFW %i.%i.%i\n", major, minor, revision);
}

vg_engine_t *vg_engine_create(int width, int height, const char *title)
{
	vg_engine_t *engine;

	engine = (vg_engine_t *) malloc(sizeof(vg_engine_t));
	OPUS_RETURN_IF(NULL, !engine);
	engine->vg = opus_vg_create();
	if (!engine->vg) {
		free(engine);
		return NULL;
	}

	engine->width  = width;
	engine->height = height;
	engine->title  = strdup(title);

	engine->is_running = 0;

	engine->context_ = NULL;
	engine->preload_ = NULL;
	engine->update_  = NULL;
	engine->render_  = NULL;
	engine->cleanup_ = NULL;

	/* init glfw context */
	engine->window = NULL;
	init_glfw_(engine);
	create_glfw_window_(engine);
	if (engine->window) glfwHideWindow(engine->window);

	/* setup time measurement */
	stm_setup();

	return engine;
}

void vg_engine_destroy(vg_engine_t *engine)
{
	OPUS_RETURN_IF(, !engine);
	free(engine->title);
	opus_vg_destroy(engine->vg);
	free(engine);
}

void vg_engine_set(vg_engine_t *engine,
                   vg_engine_preload_cb preload,
                   vg_engine_update_cb update,
                   vg_engine_render_cb render,
                   vg_engine_cleanup_cb cleanup)
{
	engine->preload_ = preload;
	engine->update_ = update;
	engine->render_ = render;
	engine->cleanup_ = cleanup;
}

void vg_engine_stop(vg_engine_t *engine)
{
	engine->is_running = 0;
}

void iter(vg_engine_t *engine)
{
	static float avg_duration = 0.f;
	static float alpha        = 1.f / 100.f; /* sampling_count=100 */
	GLenum       err;

	for (; (err = glGetError()) != GL_NO_ERROR;)
		OPUS_ERROR("OpenGL Error Code %d appeared, please check for further information\n", err);

	glViewport(0, 0, engine->width, engine->height);

	engine->current_time = vg_engine_get_time();
	engine->frame_count++;
	engine->elapsed_time = engine->current_time - engine->last_time;

	/* calculate FPS */
	if (1 == engine->frame_count)
		avg_duration = (float) engine->elapsed_time;
	else
		avg_duration = avg_duration * (1 - alpha) + (float) engine->elapsed_time * alpha;
	engine->fps = 1. / avg_duration;

	if (engine->update_) engine->update_(engine, engine->elapsed_time);
	if (engine->render_) engine->render_(engine);

	/* swap the screen buffers */
	glfwSwapBuffers(engine->window);
	/* poll for and process events */
	glfwPollEvents();

	engine->last_time = engine->current_time;
}

opus_real vg_engine_get_time()
{
	return stm_ms(stm_now());
}

void vg_engine_start(vg_engine_t *engine)
{
	OPUS_INFO("engine is now running\n");

	if (!engine->window) create_glfw_window_(engine);
	if (engine->window) glfwShowWindow(engine->window);

	glfwMakeContextCurrent(engine->window);


	/* load resources and prepare to enter the main loop */
	if (engine->preload_) engine->preload_(engine);

	engine->is_running   = 1;
	engine->frame_count  = 0;
	engine->last_time    = vg_engine_get_time();
	engine->start_time   = engine->last_time;
	engine->elapsed_time = 0.0;
	engine->fps          = 0.0;
	while (!glfwWindowShouldClose(engine->window) && engine->is_running)
		iter(engine);

	/* cleanup resources and exit */
	if (engine->cleanup_) engine->cleanup_(engine);
	OPUS_INFO("engine preformed a cleanup and terminated\n");
}


/* glfw internal error callback */
void error_(int error, const char *description)
{
	OPUS_ERROR("GLFW_INTERNAL_ERROR::%s\n", description);
}
