/**
 * @file vg_utils.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/12/29
 *
 * @example
 *
 * @development_log
 *
 */

#include "utils/utils.h"
#include "vg/vg_utils.h"
#include "vg/pluto/plutovg-private.h"

void vg_pl_line(plutovg_t *vg, opus_real sx, opus_real sy, opus_real ex, opus_real ey)
{
	plutovg_move_to(vg, sx, sy);
	plutovg_line_to(vg, ex, ey);
}

void vg_pl_line_vec(plutovg_t *vg, opus_vec2 s, opus_vec2 e)
{
	vg_pl_line(vg, s.x, s.y, e.x, e.y);
}

void vg_pl_path(plutovg_t *vg, opus_vec2 *path, size_t n)
{
	uint64_t i;
	plutovg_move_to(vg, path[0].x, path[0].y);
	for (i = 1; i < n; i++) plutovg_line_to(vg, path[i].x, path[i].y);
}

void vg_pl_arrow(plutovg_t *vg, opus_vec2 origin, opus_vec2 dir, opus_real deflection,
                 opus_real len_head, opus_real len_body)
{
	opus_vec2 p, tmp;
	dir = opus_vec2_norm(dir);
	p.x = dir.x * len_head;
	p.y = dir.y * len_head;
	dir.x *= len_body;
	dir.y *= len_body;
	plutovg_move_to(vg, origin.x, origin.y);
	plutovg_line_to(vg, origin.x + dir.x, origin.y + dir.y);
	tmp = opus_vec2_rotate(p, OPUS_PI - deflection);
	plutovg_line_to(vg, origin.x + dir.x + tmp.x, origin.y + dir.y + tmp.y);
	plutovg_move_to(vg, origin.x + dir.x, origin.y + dir.y);
	tmp = opus_vec2_rotate(p, -OPUS_PI + deflection);
	plutovg_line_to(vg, origin.x + dir.x + tmp.x, origin.y + dir.y + tmp.y);
}

/**
 * @brief
 * @param vg
 * @param text
 * @param len -1 if you want to display all of it
 * @param x
 * @param y
 */
void vg_pl_text_n(plutovg_t *vg, const char *text, int len, opus_real x, opus_real y)
{
	double font_size;
	OPUS_NOT_NULL(vg->state->font);
	font_size = plutovg_font_get_size(vg->state->font);
	plutovg_textn(vg, text, len, x, y + plutovg_font_get_ascent(vg->state->font));
}

void vg_pl_text(plutovg_t *vg, const char *text, opus_real x, opus_real y)
{
	OPUS_NOT_NULL(vg->state->font);
	plutovg_text(vg, text, x, y + plutovg_font_get_ascent(vg->state->font));
}

/**
 * @brief Define a text box with a width of "max_width", and a height of "max_height" to display
 * text. Text will, however, be displayed in a smaller box decided by "margin_x" and "margin_y".
 * @param vg
 * @param text
 * @param len the max length of text you want to display, -1 means displaying whole text
 * @param x the top-left coordinate X of the text box
 * @param y the top-left coordinate Y of the text box
 * @param max_width -1 means no limit
 * @param max_height -1 means no limit
 * @param margin_x
 * @param margin_y
 * @param split_word 1 if you want to display a '-' after the English word are cut
 * @param line_spacing space for each line
 */
void vg_pl_text_box(plutovg_t *vg, const char *text, int len, opus_real x, opus_real y,
                    opus_real max_width, opus_real max_height, opus_real margin_x,
                    opus_real margin_y, int split_word, opus_real line_spacing)
{
	double font_size, advance, offset_y;
	char  *last_start = (char *) text;
	char  *end        = (char *) text + (len == -1 ? strlen(text) + 1 : len);
	char  *ptr        = (char *) text;
	int    row        = 0;

	OPUS_NOT_NULL(vg->state->font);
	font_size = plutovg_font_get_size(vg->state->font);

	/* draw the outline of the text box */
	/*plutovg_rect(vg, x, y, max_width, max_height);
	plutovg_stroke(vg);*/

	x += margin_x;
	y += margin_y;
	while (ptr < end && *ptr != '\0') {
		int add_connect_char = 0; /* 1 to add '-' */

		/* find the end of each row */
		advance = 0;
		while (*ptr != '\0' && *ptr != '\n') {

			advance += plutovg_font_get_char_advance(vg->state->font, (int) *ptr);

			if (!(max_width < 0 || advance <= max_width - 2 * margin_x)) {
				/* append char '-' if you pass 1 to "split_word" */
				char *prev = ptr - 1;
				if (split_word == 1 &&
				    ((*prev >= 'A' && *prev <= 'Z') || (*prev >= 'a' && *prev <= 'z'))) {
					ptr -= 1;
					add_connect_char = 1;
				}
				break;
			}

			ptr++;
		}

		/* test if the text to draw reach the bottom of text box */
		offset_y = row * (font_size + line_spacing);
		if (max_height >= 0 && offset_y + font_size + 2 * margin_y > max_height) break;

		/* draw text path */
		vg_pl_text_n(vg, last_start, (int) (ptr - last_start), x, y + offset_y);
		if (add_connect_char) {
			double last2_char_advance =
			        plutovg_font_get_char_advance(vg->state->font, (int) *(ptr - 1)) +
			        plutovg_font_get_char_advance(vg->state->font, (int) *(ptr - 2));
			vg_pl_text(vg, "-", x + advance - last2_char_advance,
			           y + offset_y + plutovg_font_get_descent(vg->state->font));
		}

		/* proceed to next row of text */
		row++;
		if (*ptr == '\n') ptr++;
		last_start = ptr;
	}
}
