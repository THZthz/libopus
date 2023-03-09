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
#ifndef VG_ENGINE_H
#define VG_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "math/math.h"
#include "vg/vg.h"
#include "external/glad/glad.h"
#include "external/GLFW/glfw3.h"

typedef struct vg_engine vg_engine_t;
typedef void (*vg_engine_preload_cb)(vg_engine_t *);
typedef void (*vg_engine_render_cb)(vg_engine_t *);
typedef void (*vg_engine_update_cb)(vg_engine_t *, opus_real);
typedef void (*vg_engine_cleanup_cb)(vg_engine_t *);


struct vg_engine {
	int      width, height; /* size of the main GLFW window */
	char    *title;         /* title of the main GLFW window */
	opus_vg *vg;            /* vector graphics context bind to this engine, automatically created */
	int      is_running;

	/* main glfw window */
	GLFWwindow *window;

	/* context of this vector graphics engine */
	void *context_;

	vg_engine_preload_cb preload_;
	vg_engine_render_cb  render_;
	vg_engine_update_cb  update_;
	vg_engine_cleanup_cb cleanup_;

	/* measurement */
	unsigned  frame_count;
	opus_real start_time, last_time, elapsed_time, current_time;
	opus_real fps;
};

vg_engine_t *vg_engine_create(int width, int height, const char *title);
void         vg_engine_destroy(vg_engine_t *engine);

void vg_engine_set(vg_engine_t         *engine,
                   vg_engine_preload_cb preload,
                   vg_engine_update_cb  update,
                   vg_engine_render_cb  render,
                   vg_engine_cleanup_cb cleanup);

opus_real vg_engine_get_time();
void      vg_engine_start(vg_engine_t *engine);
void      vg_engine_stop(vg_engine_t *engine);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* VG_ENGINE_H */
