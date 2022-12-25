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
 * 		bresenham_line_init(xo, yo, xd, yd, &data);
 * 		do {
 * 			// do something with (xo, yo)
 * 			// ...
 * 		} while (!bresenham_line_step(&xo, &yo, &data));
 */
#ifndef BRESENHAM_H
#define BRESENHAM_H

/**
 *  @brief A struct used for computing a bresenham line.
 */
typedef struct bresenham_data {
	int step_x;
	int step_y;
	int e;
	int delta_x;
	int delta_y;
	int orig_x;
	int orig_y;
	int dest_x;
	int dest_y;
} bresenham_data_t;

void bresenham_line_init(int x_from, int y_from, int x_to, int y_to, bresenham_data_t* data);
int  bresenham_line_step(int* cur_x, int* cur_y, bresenham_data_t* data);

#endif /* BRESENHAM_H */
