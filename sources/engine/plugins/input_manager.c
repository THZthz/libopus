/**
 * @file input.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/11/13
 *
 * @example
 *
 * @development_log
 *
 */

#include "engine/engine.h"

static engine_t *g_engine = NULL;
static int g_is_pointer_down = 0;

#ifdef __EMSCRIPTEN__
static const char *engine_emscripten_event_type_to_string(int event_type)
{
	const char *events[] = {"(invalid)", "(none)", "keypress", "keydown", "keyup", "click", "mousedown", "mouseup", "dblclick", "mousemove", "wheel", "resize",
	                        "scroll", "blur", "focus", "focusin", "focusout", "deviceorientation", "devicemotion", "orientationchange", "fullscreenchange", "pointerlockchange",
	                        "visibilitychange", "touchstart", "touchend", "touchmove", "touchcancel", "gamepadconnected", "gamepaddisconnected", "beforeunload",
	                        "batterychargingchange", "batterylevelchange", "webglcontextlost", "webglcontextrestored", "(invalid)"};
	++event_type;
	if (event_type < 0) event_type = 0;
	if (event_type >= sizeof(events) / sizeof(events[0])) event_type = sizeof(events) / sizeof(events[0]) - 1;
	return events[event_type];
}

#define GENERAL_MOUSE_EVENT_ACTION                                                                   \
	g_engine->pointer.x              = e->clientX;                                                   \
	g_engine->pointer.y              = e->clientY;                                                   \
	g_engine->event_data.event_flag  = ENGINE_FLAG_IS_MOUSE_EVENT | ENGINE_FLAG_IS_EMSCRIPTEN_EVENT; \
	g_engine->event_data.mouse_event = (EmscriptenMouseEvent *) e;

EM_BOOL engine_emsc_mouse_move_callback(int event_type, const EmscriptenMouseEvent *e, void *user_data)
{
	GENERAL_MOUSE_EVENT_ACTION
	g_engine->event_data.event_type = ENGINE_FLAG_POINTER_MOVE;
	event_hub_emit(g_engine->hub, engine_event_on_pointer_move, g_engine);
	return EM_TRUE;
}

EM_BOOL engine_emsc_mouse_down_callback(int event_type, const EmscriptenMouseEvent *e, void *user_data)
{
	GENERAL_MOUSE_EVENT_ACTION
	g_engine->event_data.event_type = ENGINE_FLAG_POINTER_DOWN;
	event_hub_emit(g_engine->hub, engine_event_on_pointer_down, g_engine);
	return EM_TRUE;
}

EM_BOOL engine_emsc_mouse_up_callback(int event_type, const EmscriptenMouseEvent *e, void *user_data)
{
	GENERAL_MOUSE_EVENT_ACTION
	g_engine->event_data.event_type = ENGINE_FLAG_POINTER_UP;
	event_hub_emit(g_engine->hub, engine_event_on_pointer_up, g_engine);
	return EM_TRUE;
}

#define GENERAL_TOUCH_EVENT_ACTION                                                                   \
	g_engine->pointer.x              = e->touches[0].clientX;                                        \
	g_engine->pointer.y              = e->touches[0].clientY;                                        \
	g_engine->event_data.event_flag  = ENGINE_FLAG_IS_TOUCH_EVENT | ENGINE_FLAG_IS_EMSCRIPTEN_EVENT; \
	g_engine->event_data.touch_event = (EmscriptenTouchEvent *) e;

EM_BOOL engine_emsc_touch_move_callback(int event_type, const EmscriptenTouchEvent *e, void *user_data)
{
	GENERAL_TOUCH_EVENT_ACTION
	g_engine->event_data.event_type = ENGINE_FLAG_POINTER_MOVE;
	event_hub_emit(g_engine->hub, engine_event_on_pointer_move, g_engine);
	return EM_TRUE;
}

EM_BOOL engine_emsc_touch_start_callback(int event_type, const EmscriptenTouchEvent *e, void *user_data)
{
	GENERAL_TOUCH_EVENT_ACTION
	g_engine->event_data.event_type = ENGINE_FLAG_POINTER_DOWN;
	event_hub_emit(g_engine->hub, engine_event_on_pointer_down, g_engine);
	return EM_TRUE;
}

EM_BOOL engine_emsc_touch_end_callback(int event_type, const EmscriptenTouchEvent *e, void *user_data)
{
	GENERAL_TOUCH_EVENT_ACTION
	g_engine->event_data.event_type = ENGINE_FLAG_POINTER_UP;
	event_hub_emit(g_engine->hub, engine_event_on_pointer_up, g_engine);
	return EM_TRUE;
}
#endif /* __EMSCRIPTEN__ */

static void glfw_cursor_position_callback_(GLFWwindow *window, double x_pos, double y_pos)
{
#ifndef __EMSCRIPTEN_ON_MOBILE__
	vec2_set(g_engine->pointer, x_pos, y_pos);
	g_engine->event_data.event_flag = ENGINE_FLAG_IS_MOUSE_EVENT;
	g_engine->event_data.event_type = ENGINE_FLAG_POINTER_MOVE;
	event_hub_emit(g_engine->event_hub, engine_event_on_pointer_move, g_engine);
#endif /* __EMSCRIPTEN_ON_MOBILE__ */
}

static void glfw_key_callback_(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_UNKNOWN) return;
	if (action == GLFW_PRESS) {
		g_engine->event_data.event_type = ENGINE_FLAG_KEY_DOWN;
		g_engine->keys_state[key] = 1;
		g_engine->key_pressed_count++;
		event_hub_emit(g_engine->event_hub, engine_event_on_key_down, g_engine);
	} else if (action == GLFW_RELEASE) {
		g_engine->event_data.event_type = ENGINE_FLAG_KEY_UP;
		g_engine->keys_state[key] = 0;
		g_engine->key_pressed_count--;
		event_hub_emit(g_engine->event_hub, engine_event_on_key_up, g_engine);
	}
#ifndef __EMSCRIPTEN__
	/* specially for desktop users to quit the window */
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		g_engine->is_running = 0;
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
#endif /* __EMSCRIPTEN__ */

	g_engine->ctrl_key  = (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL);
	g_engine->shift_key = (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT);
	g_engine->meta_key  = 0; /* FIXME: what is meta_key? */
	g_engine->alt_key   = (key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT);
}

static void glfw_mouse_button_callback_(GLFWwindow *window, int button, int action, int mods)
{
#ifndef __EMSCRIPTEN_ON_MOBILE__
	int    stat = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	double x, y;
	glfwGetCursorPos(window, &x, &y);
	vec2_set(g_engine->pointer, x, y);
	g_engine->event_data.event_flag = ENGINE_FLAG_IS_MOUSE_EVENT;
	if (stat == GLFW_PRESS) {
		g_engine->event_data.event_type = ENGINE_FLAG_POINTER_DOWN;
		event_hub_emit(g_engine->event_hub, engine_event_on_pointer_down, g_engine);
	} else if (stat == GLFW_RELEASE) {
		g_engine->event_data.event_type = ENGINE_FLAG_POINTER_UP;
		event_hub_emit(g_engine->event_hub, engine_event_on_pointer_up, g_engine);
	}
#endif /* __EMSCRIPTEN_ON_MOBILE__ */
}

static void glfw_scroll_callback_(GLFWwindow *window, double x_offset, double y_offset)
{
	g_engine->scroll_x = x_offset;
	g_engine->scroll_y = y_offset;

	g_engine->event_data.event_type = ENGINE_FLAG_SCROLL;
	event_hub_emit(g_engine->event_hub, engine_event_on_scroll, g_engine);
}

static int drag_start_listener(event_hub_t *hub, event_t *e, void *user_data)
{
	engine_t *eng = user_data;
	g_is_pointer_down = 1;
	vec2_copy(eng->event_data.drag_data.start_pos, eng->pointer);
	return 0;
}

static int drag_move_listener(event_hub_t *hub, event_t *e, void *user_data)
{
	engine_t *eng = user_data;
	if (g_is_pointer_down) {
		vec2_copy(eng->event_data.drag_data.last_pos, eng->pointer);
		eng->event_data.event_type = ENGINE_FLAG_DRAG;
		event_hub_emit(eng->event_hub, engine_event_on_drag, eng);
	}
	return 0;
}

static int drag_end_listener(event_hub_t *hub, event_t *e, void *user_data)
{
	engine_t *eng = user_data;
	if (g_is_pointer_down) {
		vec2_copy(eng->event_data.drag_data.end_pos, eng->pointer);
		g_is_pointer_down = 0;
	}
	return 0;
}

/* called in "engine.c" using "extern" declaration */
extern void input_manager_boot_plugin(engine_t *eng, plugin_t *plugin,...)
{
	GLFWwindow      *window = eng->window;
	g_engine                = eng;

#ifndef __EMSCRIPTEN_ON_MOBILE__
	glfwSetKeyCallback(window, glfw_key_callback_);
	glfwSetMouseButtonCallback(window, glfw_mouse_button_callback_);
	glfwSetCursorPosCallback(window, glfw_cursor_position_callback_);
	glfwSetScrollCallback(window, glfw_scroll_callback_);
#endif /* __EMSCRIPTEN_ON_MOBILE__ */

#ifdef __EMSCRIPTEN__
	emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, eng, 1, engine_emsc_mouse_move_callback);
	emscripten_set_mousedown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, eng, 1, engine_emsc_mouse_down_callback);
	emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, eng, 1, engine_emsc_mouse_up_callback);

	emscripten_set_touchmove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, eng, 1, engine_emsc_touch_move_callback);
	emscripten_set_touchstart_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, eng, 1, engine_emsc_touch_start_callback);
	emscripten_set_touchend_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, eng, 1, engine_emsc_touch_end_callback);
#endif /* __EMSCRIPTEN__ */

	/* listen and generate drag event */
	{
		event_cb callbacks[5];
		callbacks[0] = &drag_start_listener;
		event_hub_on(g_engine->event_hub, engine_event_on_pointer_down, event_create(callbacks, 1, -1, g_engine));
		callbacks[0] = &drag_move_listener;
		event_hub_on(g_engine->event_hub, engine_event_on_pointer_move, event_create(callbacks, 1, -1, g_engine));
		callbacks[0] = &drag_end_listener;
		event_hub_on(g_engine->event_hub, engine_event_on_pointer_up, event_create(callbacks, 1, -1, g_engine));
	}
}
