/**
 * @file vg_engine.h
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/12/27
 *
 * @brief An interface which integrates GLFW and "vg_t", please notice that if you want to render the content of
 * 		"vg_t", you should create "vg_gl_program_t" to render it using openGL.
 *
 */
#ifndef ENGINE_H
#define ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "math/math.h"
#include "vg/vg.h"
#include "external/glad/glad.h"
#include "external/GLFW/glfw3.h"

typedef struct opus_engine opus_engine;
typedef void (*opus_engine_preload_cb)(opus_engine *);
typedef void (*opus_engine_render_cb)(opus_engine *);
typedef void (*opus_engine_update_cb)(opus_engine *, opus_real);
typedef void (*opus_engine_cleanup_cb)(opus_engine *);


struct opus_engine {
	int      width, height; /* size of the main GLFW window */
	char    *title;         /* title of the main GLFW window */
	opus_vg *vg;            /* vector graphics context bind to this engine, automatically created */
	int      is_running;

	/* main glfw window */
	GLFWwindow *window;

	/* context of this vector graphics engine */
	void *context_;

	opus_engine_preload_cb preload_;
	opus_engine_render_cb  render_;
	opus_engine_update_cb  update_;
	opus_engine_cleanup_cb cleanup_;

	/* measurement */
	unsigned  frame_count;
	opus_real start_time, last_time, elapsed_time, current_time;
	opus_real fps;
};

opus_engine *opus_engine_create(int width, int height, const char *title);
void         opus_engine_destroy(opus_engine *engine);

void opus_engine_set_callback(opus_engine           *engine,
                              opus_engine_preload_cb preload,
                              opus_engine_update_cb  update,
                              opus_engine_render_cb  render,
                              opus_engine_cleanup_cb cleanup);

opus_real opus_engine_get_time(void);
void      opus_engine_start(opus_engine *engine);
void      opus_engine_stop(opus_engine *engine);
void      opus_engine_end(opus_engine *engine);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* ENGINE_H */
