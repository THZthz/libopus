/*
 * Created by amias on 2022/2/10.
 */
/**
 * @file bresenham.h
 * @brief Draw a line in discrete space.
 *
 * @example to use this library, simply follow a template:
 * 		// (xo, yo): origin, (xd, yd): destination
 * 		bresenham_data_t data;
 * 		opus_bresenham_init(xo, yo, xd, yd, &data);
 * 		do {
 * 			// do something with (xo, yo)
 * 			// ...
 * 		} while (!opus_bresenham_step(&xo, &yo, &data));
 */
#ifndef BRESENHAM_H
#define BRESENHAM_H

/**
 *  @brief A struct used for computing a bresenham line.
 */
typedef struct opus_bresenham {
	int step_x;
	int step_y;
	int e;
	int delta_x;
	int delta_y;
	int orig_x;
	int orig_y;
	int dest_x;
	int dest_y;
} opus_bresenham_data;

void opus_bresenham_init(int x_from, int y_from, int x_to, int y_to, opus_bresenham_data* data);
int  opus_bresenham_step(int* cur_x, int* cur_y, opus_bresenham_data* data);

#endif /* BRESENHAM_H */
