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
#ifndef INPUT_H
#define INPUT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "engine/engine.h"
#include "data_structure/array.h"

#define OPUS_ENGINE_KEYS_SIZE (348)

typedef struct opus_input opus_input;
typedef void (*opus_input_cb)(opus_input *input);

struct opus_input {
	void *context;

	opus_engine *engine; /* TODO: should we use hashmap to handle the case when multiple windows
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
	char keys_state[OPUS_ENGINE_KEYS_SIZE];

	opus_input_cb *on_pointer_down;
	opus_input_cb *on_pointer_move;
	opus_input_cb *on_pointer_up;
	opus_input_cb *on_drag;
	opus_input_cb *on_scroll;
	opus_input_cb *on_key_press;
	opus_input_cb *on_key_down;
	opus_input_cb *on_key_up;
};

opus_input *opus_input_init(opus_engine *engine);
void        opus_input_done(void);

void opus_input_on(opus_input_cb *callback, opus_input_cb cb);
void opus_input_emit(opus_input_cb *callback);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* INPUT_H */
