/**
 * @file vg_input.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/12/27
 *
 * @example
 *
 * @development_log
 *
 */

#include "utils/utils.h"
#include "vg/vg_input.h"

static vg_input_t input;

void vg_input_emit(vg_input_cb *callback)
{
	size_t i, n ;
	if (!callback) return;
	for (i = 0, n = opus_arr_len(callback); i < n; i++) {
		if (callback[i] != NULL) callback[i](&input);
	}
}

void vg_input_on(vg_input_cb *callback, vg_input_cb cb)
{
	if (!callback) return;
	opus_arr_push(callback, &cb);
}

static void cursor_position_(GLFWwindow *window, double x_pos, double y_pos)
{
	opus_vec2_set(&input.pointer, x_pos, y_pos);
	vg_input_emit(input.on_pointer_move);
	if (input.is_pointer_down) {
		vg_input_emit(input.on_drag);
	}
}

static void key_(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_UNKNOWN) return;
	if (action == GLFW_PRESS) {
		input.keys_state[key] = 1;
		input.key_pressed++;
		vg_input_emit(input.on_key_down);
	} else if (action == GLFW_RELEASE) {
		input.keys_state[key] = 0;
		input.key_pressed--;
		vg_input_emit(input.on_key_up);
	}

	input.ctrl_key  = (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL);
	input.shift_key = (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT);
	input.meta_key  = 0; /* FIXME: what is meta_key? */
	input.alt_key   = (key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT);
}

static void mouse_button_(GLFWwindow *window, int button, int action, int mods)
{
	double x, y;
	glfwGetCursorPos(window, &x, &y);
	opus_vec2_set(&input.pointer, x, y);
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			input.is_pointer_down = 1;
			opus_vec2_copy(&input.drag_start, input.pointer);
		} else if (action == GLFW_RELEASE) {
			input.is_pointer_down = 0;
		}
	}
	input.mouse_left   = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	input.mouse_middle = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE);
	input.mouse_right  = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
	if (action == GLFW_PRESS) {
		vg_input_emit(input.on_pointer_down);
	} else if (action == GLFW_RELEASE) {
		vg_input_emit(input.on_pointer_up);
	}
}

static void scroll_(GLFWwindow *window, double x_offset, double y_offset)
{
	input.scroll_x = x_offset;
	input.scroll_y = y_offset;
	vg_input_emit(input.on_scroll);
}

vg_input_t *vg_input_init(vg_engine_t *engine)
{
	OPUS_RETURN_IF(NULL, !engine);

	input.engine = engine;

	memset(input.keys_state, 0, sizeof(char) * VG_ENGINE_KEYS_SIZE);
	input.key_pressed     = 0;
	input.mouse_left      = 0;
	input.mouse_middle    = 0;
	input.mouse_right     = 0;
	input.is_pointer_down = 0;

	glfwSetKeyCallback(engine->window, key_);
	glfwSetMouseButtonCallback(engine->window, mouse_button_);
	glfwSetCursorPosCallback(engine->window, cursor_position_);
	glfwSetScrollCallback(engine->window, scroll_);

	opus_arr_create(input.on_pointer_down, sizeof(vg_input_cb));
	opus_arr_create(input.on_pointer_move, sizeof(vg_input_cb));
	opus_arr_create(input.on_pointer_up, sizeof(vg_input_cb));
	opus_arr_create(input.on_drag, sizeof(vg_input_cb));
	opus_arr_create(input.on_scroll, sizeof(vg_input_cb));
	opus_arr_create(input.on_key_press, sizeof(vg_input_cb));
	opus_arr_create(input.on_key_down, sizeof(vg_input_cb));
	opus_arr_create(input.on_key_up, sizeof(vg_input_cb));

	return &input;
}

void vg_input_done()
{
	opus_arr_destroy(input.on_pointer_down);
	opus_arr_destroy(input.on_pointer_move);
	opus_arr_destroy(input.on_pointer_up);
	opus_arr_destroy(input.on_drag);
	opus_arr_destroy(input.on_scroll);
	opus_arr_destroy(input.on_key_press);
	opus_arr_destroy(input.on_key_down);
	opus_arr_destroy(input.on_key_up);
}
