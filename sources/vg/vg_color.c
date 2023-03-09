/**
 * @file vg_color.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2023/2/23
 *
 * @example
 *
 * @development_log
 *
 */

#include "vg/vg_color.h"

/**
 * @brief transform RGB value to HSL value,
 * 		R, G and B input range = 0 ~ 1.0
 * 		H, S and L output range = 0 ~ 1.0
 */
void vg_color_RGB2HSL(opus_real r, opus_real g, opus_real b, opus_real *h, opus_real *s,
                      opus_real *l)
{
	opus_real min = r, max = r, delta;

	if (g < min) min = g;
	if (g > max) max = g;
	if (b < min) min = b;
	if (b > max) max = b;
	delta = max - min;

	*l = (max + min) / 2;

	if (delta == 0) {
		/* no chroma if the color is gray */
		*h = 0;
		*s = 0;
	} else {
		opus_real delta_r, delta_g, delta_b;

		if (*l < 0.5) *s = delta / (max + min);
		else
			*s = delta / (2 - max - min);

		delta_r = (((max - r) / 6) + (delta / 2)) / delta;
		delta_g = (((max - g) / 6) + (delta / 2)) / delta;
		delta_b = (((max - b) / 6) + (delta / 2)) / delta;

		if (r == max)
			*h = delta_b - delta_g;
		else if (g == max)
			*h = (1.0 / 3) + delta_r - delta_b;
		else if (b == max)
			*h = (2.0 / 3) + delta_g - delta_r;

		if (*h < 0) *h += 1;
		if (*h > 1) *h -= 1;
	}
}

static opus_real hue2rgb_(opus_real v1, opus_real v2, opus_real vh)
{
	if (vh < 0) vh += 1;
	if (vh > 1) vh -= 1;
	if ((6 * vh) < 1) return v1 + (v2 - v1) * 6 * vh;
	if ((2 * vh) < 1) return v2;
	if ((3 * vh) < 2) return v1 + (v2 - v1) * ((2.0 / 3) - vh) * 6;
	return v1;
}

/**
 * @brief transform HSL value to RGB value,
 * 		H, S and L output range = 0 ~ 1.0
 * 		R, G and B input range = 0 ~ 1.0
 * @param h
 * @param s
 * @param l
 * @param r
 * @param g
 * @param b
 */
void vg_color_HSL2RGB(opus_real h, opus_real s, opus_real l, opus_real *r, opus_real *g,
                      opus_real *b)
{
	if (s == 0) {
		*r = l;
		*g = l;
		*b = l;
	} else {
		opus_real v1, v2;

		if (l < 0.5) v2 = l * (1 + s);
		else
			v2 = (l + s) - (s * l);

		v1 = 2 * l - v2;

		*r = hue2rgb_(v1, v2, h + (1.0 / 3));
		*g = hue2rgb_(v1, v2, h);
		*b = hue2rgb_(v1, v2, h - (1.0 / 3));
	}
}
