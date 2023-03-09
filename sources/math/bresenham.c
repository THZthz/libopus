/**
 * @file bresenham.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/8/24
 *
 * @example
 *
 * @development_log
 *
 */

#include "math/bresenham.h"

/**
 *  @brief initialize a bresenham_bresenham_data_t struct.
 *
 *  	after calling this function you use opus_bresenham_step to iterate
 *  	over the individual points on the line.
 *
 *  @param x_from The starting x position.
 *  @param y_from The starting y position.
 *  @param x_to The ending x position.
 *  @param y_to The ending y position.
 *  @param data Pointer to a bresenham_bresenham_data_t struct.
 */
void opus_bresenham_init(int x_from, int y_from, int x_to, int y_to, opus_bresenham_data* data)
{
	data->orig_x  = x_from;
	data->orig_y  = y_from;
	data->dest_x  = x_to;
	data->dest_y  = y_to;
	data->delta_x = x_to - x_from;
	data->delta_y = y_to - y_from;
	if (data->delta_x > 0) {
		data->step_x = 1;
	} else if (data->delta_x < 0) {
		data->step_x = -1;
	} else
		data->step_x = 0;
	if (data->delta_y > 0) {
		data->step_y = 1;
	} else if (data->delta_y < 0) {
		data->step_y = -1;
	} else
		data->step_y = 0;
	if (data->step_x * data->delta_x > data->step_y * data->delta_y) {
		data->e = data->step_x * data->delta_x;
		data->delta_x *= 2;
		data->delta_y *= 2;
	} else {
		data->e = data->step_y * data->delta_y;
		data->delta_x *= 2;
		data->delta_y *= 2;
	}
}

/**
 *  @brief Get the next point on a line, returns true once the line has ended.
 *  	The starting point is excluded by this function.
 *  	After the ending point is reached, the next call will return true.
 *
 *  @param cur_x An int pointer to fill with the next x position.
 *  @param cur_y An int pointer to fill with the next y position.
 *  @param data Pointer to a initialized bresenham_bresenham_data_t struct.
 *  @return 1 after the ending point has been reached, 0 otherwise.
 */
int opus_bresenham_step(int* cur_x, int* cur_y, opus_bresenham_data* data)
{
	if (data->step_x * data->delta_x > data->step_y * data->delta_y) {
		if (data->orig_x == data->dest_x) return 1;
		data->orig_x += data->step_x;
		data->e -= data->step_y * data->delta_y;
		if (data->e < 0) {
			data->orig_y += data->step_y;
			data->e += data->step_x * data->delta_x;
		}
	} else {
		if (data->orig_y == data->dest_y) return 1;
		data->orig_y += data->step_y;
		data->e -= data->step_x * data->delta_x;
		if (data->e < 0) {
			data->orig_x += data->step_x;
			data->e += data->step_y * data->delta_y;
		}
	}
	*cur_x = data->orig_x;
	*cur_y = data->orig_y;
	return 0;
}
