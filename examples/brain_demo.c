/**
 * @file brain_demo.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/10/19
 *
 * @example
 *
 * @development_log
 *
 */


#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "brain/ann.h"
#include "math/math.h"
#include "utils/utils.h"

#define MAX_SAMPLE_SIZE (500)
#define MAX_ITERATION (100000)

void run_test(const char *name, void (*func)())
{
	printf("\n=======================[%s]=======================\n", name);
	func();
}

int main()
{
	void xor ();
	void predefined_func();
	run_test("xor", xor);
	run_test("predefined_func", predefined_func);

	return 0;
}

/* use this function to produce input data */
static ann_real f(ann_real x1, ann_real x2, ann_real x3)
{
	return r_clamp(x1 * x2 + x3 / 2, -1, 1);
}

/* calculate the error between the desired output and the network output */
static ann_real calc_err(ann_real desired, ann_real actual)
{
	return fabs(actual - desired);
}

void predefined_func()
{
	ann_t   *net, *best = NULL;
	uint32_t nmap[4] = {3, 4, 3, 1};
	ann_real input[MAX_SAMPLE_SIZE][3], output[MAX_SAMPLE_SIZE];
	ann_real required_err = 0.01, cur_err = FLT_MAX, best_err = FLT_MAX, max_err, min_err;

	uint32_t i, j;

	/* generate sample data */
	for (i = 0; i < MAX_SAMPLE_SIZE; i++) {
		ann_real x1 = r_clamp((ann_real) rand() / RAND_MAX, -1, 1);
		ann_real x2 = r_clamp((ann_real) rand() / RAND_MAX, -1, 1);
		ann_real x3 = r_clamp((ann_real) rand() / RAND_MAX, -1, 1);
		ann_real y  = f(x1, x2, x3);

		input[i][0] = x1;
		input[i][1] = x2;
		input[i][2] = x3;
		output[i]   = y;
	}

	net = ann_create(4, nmap);
	ann_randomize(net);

	/* continue training when the network miss the target too much */
	j = 0;
	while (cur_err > required_err && j++ < MAX_ITERATION) {
		ann_real max = 0, min = FLT_MAX;

		/* feed all the data into the network */
		for (i = 0; i < MAX_SAMPLE_SIZE; i++)
			ann_learn(net, input[i], output + i, 2.5);

		/* calculate average error */
		cur_err = 0;
		for (i = 0; i < MAX_SAMPLE_SIZE; i++) {
			ann_real err = calc_err(output[i], *ann_predict(net, input[i]));
			cur_err += err;

			if (err > max) max = err;
			if (err < min) min = err;
		}
		cur_err /= MAX_SAMPLE_SIZE;

		if (cur_err < best_err) {
			if (best) ann_destroy(best);
			best     = ann_get_copy(net);
			best_err = cur_err;
			max_err  = max;
			min_err  = min;
		} else if (cur_err > best_err * 2.2) {
			/* if we get worse result */
			printf("randomize net at err %f\n", cur_err);
			ann_randomize(net);
		}
	}

	printf("\ntraining exit\n");
	printf("avg err %f, max %f, min %f, %" PRIu32 " iterations in total.\n",
	       best_err, max_err, min_err, j);

	printf("\tsome tests:\n");
	printf("\t\t%8s %8s %8s     %8s   %8s\n", "input1", "input2", "input3", "output", "prediction");
	for (i = 0; i < 5; i++)
		printf("\t\t%.6f %.6f %.6f --> %.6f : %.6f\n", input[i][0], input[i][1], input[i][2], output[i], *ann_predict(best, input[i]));

	printf("\nthe network information: \n");
	ann_console(net);

	ann_destroy(net);
	if (best) ann_destroy(best);
}

void xor () {
	ann_t   *net;
	uint32_t nmap[]      = {2, 3, 1};
	ann_real input[4][2] = {
	        {0, 1},
	        {1, 0},
	        {1, 1},
	        {0, 0}};
	ann_real output[4] = {1, 1, 0, 0};
	uint32_t i, j;


	net = ann_create(3, nmap);
	ann_randomize(net);

	for (i = 0; i < 900; i++)
		for (j = 0; j < 4; j++)
			ann_learn(net, input[j], output + j, 4);

	printf("\ntrained network %dx%d times in total\n", 4, 900);
	printf("final result:\n");
	printf("\t%8s  %8s  %8s  %8s\n", "input1", "input2", "output", "network_output");

	for (i = 0; i < 4; i++) {
		printf("\t%.6f  %.6f  %.6f  %.6f\n", input[i][0], input[i][1], output[i], *ann_predict(net, input[i]));
	}

	printf("\nthe network information: \n");
	ann_console(net);

	ann_destroy(net);
}
