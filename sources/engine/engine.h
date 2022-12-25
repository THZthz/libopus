/**
 * @file render_engine.h
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
#ifndef ENGINE_H
#define ENGINE_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*#define __EMSCRIPTEN__*/

#ifdef __EMSCRIPTEN__
#include "core/external/emscripten/emscripten.h"
#include "core/external/emscripten/emscripten/html5.h"
#endif /* __EMSCRIPTEN__ */

#include "render/glad/glad.h"
#include "external/GLFW/glfw3.h"
#include "math/math.h"
#include "render/pluto/plutovg.h"
#include "utils/event.h"

/*
 * #define 	GLFW_KEY_UNKNOWN   -1
 * #define 	GLFW_KEY_SPACE   32
 * #define 	GLFW_KEY_MENU   348
 */
#define ENGINE_KEYS_SIZE (348)
#define ENGINE_KEYS_EVENT_SCANCODE_OFFSET GLFW_KEY_SPACE
#define PLUGIN_MAX_NAME_SIZE (64)
#define PLUGIN_MAX_DEPENDENCIES_SIZE (5)

enum {
	ENGINE_FLAG_IS_EMSCRIPTEN_EVENT = 1,
	ENGINE_FLAG_IS_MOUSE_EVENT      = 2,
	ENGINE_FLAG_IS_TOUCH_EVENT      = 4
};

enum {
	ENGINE_FLAG_POINTER_DOWN = 1,
	ENGINE_FLAG_POINTER_UP   = 2,
	ENGINE_FLAG_POINTER_MOVE = 4,
	ENGINE_FLAG_KEY_DOWN     = 8,
	ENGINE_FLAG_KEY_UP       = 16,
	ENGINE_FLAG_KEY_PRESS    = 32,
	ENGINE_FLAG_DRAG         = 64,
	ENGINE_FLAG_SCROLL       = 128
};

typedef struct engine       engine_t;
typedef struct engine_event engine_event_t;

typedef void (*engine_preload_t)(engine_t *);
typedef void (*engine_render_t)(engine_t *);
typedef void (*engine_update_t)(engine_t *, double);
typedef void (*engine_cleanup_t)(engine_t *);

typedef struct plugin plugin_t;
typedef void (*plugin_boot_cb)(engine_t *eng, plugin_t *plugin, ...);
typedef void (*plugin_shutdown_cb)(engine_t *eng, plugin_t *plugin);

struct plugin {
	char name[PLUGIN_MAX_NAME_SIZE];
	char dependencies[PLUGIN_MAX_NAME_SIZE * PLUGIN_MAX_DEPENDENCIES_SIZE];
	plugin_boot_cb boot_; /* called when creating engine */
	plugin_shutdown_cb shutdown_; /* called when releasing resources of the engine */
};

struct engine_event_drag {
	vec2 start_pos, last_pos, end_pos;
};
struct engine_event_pointer_down {
	vec2 pos;
};

struct engine_event {
	uint64_t event_type;
	uint64_t event_flag; /* is_mouse_event or is_touch_event */
#ifdef __EMSCRIPTEN__
	EmscriptenMouseEvent *mouse_event;
	EmscriptenTouchEvent *touch_event;
#endif /* __EMSCRIPTEN__ */
	struct engine_event_drag drag_data;
};

struct engine {
	GLFWwindow *window;
	int         width, height;

	GLsizei        data_width, data_height;
	unsigned char *data;
	GLuint         shader_program, VBO, VAO, EBO, texture;
	int      draw_data_to_current_frame;

	int is_running;
	size_t    frame_count;
	double    frame_interval;
	double    fps;
	double    start_time, last_time, current_time, elapsed_time;

	/* On desktop, it will be the position of the mouse.
	 * On mobile devices, it will be the first position of all touches */
	vec2 pointer;
	int  ctrl_key, shift_key, alt_key, meta_key; /* 1 if pressed down, otherwise 0 */
	real scroll_x, scroll_y;
	char keys_state[ENGINE_KEYS_SIZE];
	int key_pressed_count;

	plutovg_matrix_t camera; /* TODO: apply camera transform before every invocation to "render" */

	/* event */
	event_hub_t   *event_hub;
	engine_event_t event_data;

	/* plugin */
	plugin_t *plugins_;

	/* basic callback */
	engine_preload_t preload;
	engine_render_t  render;
	engine_update_t  update;
	engine_cleanup_t cleanup;
};

double engine_get_time();

void engine_set_preload(engine_t *eng, engine_render_t preload);
void engine_set_render(engine_t *eng, engine_render_t render);
void engine_set_update(engine_t *eng, engine_update_t update);
void engine_set_cleanup(engine_t *eng, engine_cleanup_t cleanup);

void engine_get_world_coord_from_screen_coord(engine_t *eng, real sx, real sy, real *wx, real *wy);

engine_t *engine_create(int win_width, int win_height, const char *title);
void      engine_destroy(engine_t *eng);

void engine_start(engine_t *eng);
void engine_stop(engine_t *eng);
void engine_preload(engine_t *eng);
void engine_render(engine_t *eng);
void engine_update(engine_t *eng);
void engine_cleanup(engine_t *eng);

extern const char *engine_event_before_update;
extern const char *engine_event_after_update;
extern const char *engine_event_before_render;
extern const char *engine_event_after_render;
extern const char *engine_event_on_cleanup;
extern const char *engine_event_on_destroy;

extern const char *engine_event_on_pointer_move;
extern const char *engine_event_on_pointer_down;
extern const char *engine_event_on_pointer_up;
extern const char *engine_event_on_drag;
extern const char *engine_event_on_key_press;
extern const char *engine_event_on_key_down;
extern const char *engine_event_on_key_up;
extern const char *engine_event_on_scroll;


#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* ENGINE_H */
