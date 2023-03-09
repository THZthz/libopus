/**
 * @file vg_input.h
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
#ifndef VG_INPUT_H
#define VG_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "vg/vg_engine.h"
#include "data_structure/array.h"

#define VG_ENGINE_KEYS_SIZE (348)

typedef struct vg_input vg_input_t;
typedef void (*vg_input_cb)(vg_input_t *input);

struct vg_input {
	void *context;

	vg_engine_t *engine; /* TODO: should we use hashmap to handle the case when multiple windows
	                        coexist? */

	opus_vec2 pointer;
	opus_real scroll_x, scroll_y;

	opus_vec2 drag_start;
	int  is_pointer_down; /* on window, this means left mouse button is down */

	int ctrl_key;
	int meta_key;
	int shift_key;
	int alt_key;

	int mouse_left;
	int mouse_middle;
	int mouse_right;

	int  key_pressed;
	char keys_state[VG_ENGINE_KEYS_SIZE];

	vg_input_cb *on_pointer_down;
	vg_input_cb *on_pointer_move;
	vg_input_cb *on_pointer_up;
	vg_input_cb *on_drag;
	vg_input_cb *on_scroll;
	vg_input_cb *on_key_press;
	vg_input_cb *on_key_down;
	vg_input_cb *on_key_up;
};

vg_input_t *vg_input_init(vg_engine_t *engine);
void        vg_input_done();

void vg_input_on(vg_input_cb *callback, vg_input_cb cb);
void vg_input_emit(vg_input_cb *callback);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* VG_INPUT_H */
