/**
 * @file engine_gui.h
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/10/15
 *
 * @example
 *
 * @development_log
 *
 */
#ifndef GUI_H
#define GUI_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "engine/engine.h"

#define GUI_MAX_TEXT_SIZE (512)

typedef struct gui_button_style gui_button_style_t;

struct gui_button_style {
	char text[GUI_MAX_TEXT_SIZE];
	real x, y; /* top-left position of the button*/
	real w, h;
	real margin_left, margin_right, margin_top, margin_bottom; /* control the region of the text */
	real bw;                                                   /* border line width */
	                                                           real fs; /* font size */
	real br, bg, bb;                                           /* background color */
	real or, og, ob;                                           /* outline color */
	real tr, tg, tb;                                           /*text color */
};

gui_button_style_t gui_button_style(char *text, real x, real y, real w, real h,
                                    real bw, real fs,
                                    real ml, real mr, real mt, real mb,
                                    real br, real bg, real bb,
                                    real or, real og, real ob,
                                    real tr, real tg, real tb);
int          gui_button(plutovg_t *vg, gui_button_style_t style);


#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* GUI_H */
