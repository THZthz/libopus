#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#include "utils/utils.h"
#include "math/math.h"
#include "lstm.h"

opus_lstm_unit *opus_lstm_unit_create(int length)
{
#define LSTM_ALLOC(_dst, _l)                           \
	do {                                               \
		mem = (double *) calloc((_l), sizeof(double)); \
		if (!mem) goto no_memory;                      \
		(_dst) = mem;                                  \
	} while (0)
#define LSTM_FREE(_src) \
	if (_src) free(_src)

	opus_lstm_unit *u;
	double	       *mem;

	OPUS_RETURN_IF(NULL, length < 1);

	u = (opus_lstm_unit *) calloc(1, sizeof(opus_lstm_unit));
	OPUS_RETURN_IF(NULL, !u);

	u->error_no  = 0;
	u->error_msg = "\0";
	u->length    = length;
	LSTM_ALLOC(u->x, length);
	LSTM_ALLOC(u->h, length);
	LSTM_ALLOC(u->f, length);
	LSTM_ALLOC(u->i, length);
	LSTM_ALLOC(u->tilde_C, length);
	LSTM_ALLOC(u->C, length);
	LSTM_ALLOC(u->o, length);
	LSTM_ALLOC(u->hat_h, length);
	opus_lstm_unit_random_params(u, -1, 1);
	return u;
no_memory:
	LSTM_FREE(u->o);
	LSTM_FREE(u->C);
	LSTM_FREE(u->tilde_C);
	LSTM_FREE(u->i);
	LSTM_FREE(u->f);
	LSTM_FREE(u->h);
	LSTM_FREE(u->x);
	free(u);
	return NULL;
}

void opus_lstm_unit_destroy(opus_lstm_unit *u)
{
	LSTM_FREE(u->o);
	LSTM_FREE(u->C);
	LSTM_FREE(u->tilde_C);
	LSTM_FREE(u->i);
	LSTM_FREE(u->f);
	LSTM_FREE(u->h);
	LSTM_FREE(u->x);
	free(u);
}

char opus_lstm_unit_random_params(opus_lstm_unit *u, double min, double max)
{
	int    i;
	double diff;
	if (NULL == u) {
		return 0;
	}
	if (max < min) {
		return 0;
	}
	diff = max - min;
	i    = u->length - 1;
	do {
		u->x[i]       = 0.0;
		u->h[i]       = 0.0;
		u->f[i]       = opus_rand_01() * diff + min;
		u->i[i]       = opus_rand_01() * diff + min;
		u->tilde_C[i] = opus_rand_01() * diff + min;
		u->C[i]       = opus_rand_01() * diff + min;
		u->o[i]       = opus_rand_01() * diff + min;
		u->hat_h[i]   = opus_rand_01() * diff + min;
	} while (i--);
	u->W_fh = opus_rand_01() * diff + min;
	u->W_fx = opus_rand_01() * diff + min;
	u->b_f  = opus_rand_01() * diff + min;
	u->W_ih = opus_rand_01() * diff + min;
	u->W_ix = opus_rand_01() * diff + min;
	u->b_i  = opus_rand_01() * diff + min;
	u->W_Ch = opus_rand_01() * diff + min;
	u->W_Cx = opus_rand_01() * diff + min;
	u->b_C  = opus_rand_01() * diff + min;
	u->W_oh = opus_rand_01() * diff + min;
	u->W_ox = opus_rand_01() * diff + min;
	u->b_o  = opus_rand_01() * diff + min;
	return 1;
}

char opus_lstm_unit_predict(opus_lstm_unit *u, double *input, double *output)
{
	double *input_back;
	double *output_back;
	if (NULL == u) {
		return 0;
	}
	if (NULL == input) {
		return 0;
	}
	if (NULL == output) {
		return 0;
	}
	input_back  = u->x;
	output_back = u->h;
	u->x        = input;
	u->h        = output;
	opus_lstm_unit_run(u);
	u->x = input_back;
	u->h = output_back;
	return 1;
}

char opus_lstm_unit_run(opus_lstm_unit *u)
{
	int    i, length;
	double s;
	if (NULL == u) {
		return 0;
	}
	length = u->length;
	for (i = 0; i < length; i++) {
		if (i == 0) {
			u->f[i] = 1.0 / (1.0 + exp(-1.0 * u->b_f));
			u->i[i] = 1.0 / (1.0 + exp(-1.0 * u->b_i));
			u->C[i] = u->i[i] * u->tilde_C[i];
			u->o[i] = 1.0 / (1.0 + exp(-1.0 * u->b_o));
			u->h[i] = u->o[i] * tanh(u->C[i]);

			u->tilde_C[i] = tanh(u->b_C);
		} else {
			s       = u->W_fh * u->h[i - 1] + u->W_fx * u->x[i - 1] + u->b_f;
			u->f[i] = 1.0 / (1.0 + exp(-1.0 * s));

			s       = u->W_ih * u->h[i - 1] + u->W_ix * u->x[i - 1] + u->b_i;
			u->i[i] = 1.0 / (1.0 + exp(-1.0 * s));

			s             = u->W_Ch * u->h[i - 1] + u->W_Cx * u->x[i - 1] + u->b_C;
			u->tilde_C[i] = tanh(s);

			u->C[i] = u->f[i] * u->C[i - 1] + u->i[i] * u->tilde_C[i];

			s       = u->W_oh * u->h[i - 1] + u->W_ox * u->x[i - 1] + u->b_o;
			u->o[i] = 1.0 / (1.0 + exp(-1.0 * s));

			u->h[i] = u->o[i] * tanh(u->C[i]);
		}
	}
	return 1;
}

double opus_lstm_unit_get_mse(opus_lstm_unit *u)
{
	int    i;
	double s, sum;
	if (NULL == u) {
		return 0.0;
	}
	sum = 0.0;
	i   = u->length - 1;
	do {
		s = u->h[i] - u->hat_h[i];
		sum += (s * s);
	} while (i--);
	return sum / u->length;
}

/**
 * BPTT 过程
 */
char opus_lstm_unit_train(opus_lstm_unit *u, double lr)
{
	int    i, length;
	double temp1, temp2, temp3, temp4;
	double d_h_W_fh = 0.0, d_h_W_fx = 0.0, d_h_b_f = 0.0;
	double d_h_W_ih = 0.0, d_h_W_ix = 0.0, d_h_b_i = 0.0;
	double d_h_W_Ch = 0.0, d_h_W_Cx = 0.0, d_h_b_C = 0.0;
	double d_h_W_oh = 0.0, d_h_W_ox = 0.0, d_h_b_o = 0.0;
	double d_h_W_fh_last = 0.0, d_h_W_fx_last = 0.0, d_h_b_f_last = 0.0;
	double d_h_W_ih_last = 0.0, d_h_W_ix_last = 0.0, d_h_b_i_last = 0.0;
	double d_h_W_Ch_last = 0.0, d_h_W_Cx_last = 0.0, d_h_b_C_last = 0.0;
	double d_h_W_oh_last = 0.0, d_h_W_ox_last = 0.0, d_h_b_o_last = 0.0;
	double d_C_W_fh = 0.0, d_C_W_fx = 0.0, d_C_b_f = 0.0;
	double d_C_W_ih = 0.0, d_C_W_ix = 0.0, d_C_b_i = 0.0;
	double d_C_W_Ch = 0.0, d_C_W_Cx = 0.0, d_C_b_C = 0.0;
	double d_C_W_oh = 0.0, d_C_W_ox = 0.0, d_C_b_o = 0.0;
	double d_C_W_fh_last = 0.0, d_C_W_fx_last = 0.0, d_C_b_f_last = 0.0;
	double d_C_W_ih_last = 0.0, d_C_W_ix_last = 0.0, d_C_b_i_last = 0.0;
	double d_C_W_Ch_last = 0.0, d_C_W_Cx_last = 0.0, d_C_b_C_last = 0.0;
	double d_C_W_oh_last = 0.0, d_C_W_ox_last = 0.0, d_C_b_o_last = 0.0;
	double d_E_W_fh = 0.0, d_E_W_fx = 0.0, d_E_b_f = 0.0;
	double d_E_W_ih = 0.0, d_E_W_ix = 0.0, d_E_b_i = 0.0;
	double d_E_W_Ch = 0.0, d_E_W_Cx = 0.0, d_E_b_C = 0.0;
	double d_E_W_oh = 0.0, d_E_W_ox = 0.0, d_E_b_o = 0.0;
	if (NULL == u) {
		return 0;
	}
	length = u->length;
	for (i = 0; i < length; i++) {
		if (0 == i) {
			d_h_b_f  = 0.0;
			d_h_W_fh = 0.0;
			d_h_W_fx = 0.0;
			temp1    = tanh(u->C[i]);
			temp2    = 1.0 / (1.0 + exp(-(u->W_ix * u->x[i] + u->b_i)));
			d_h_b_i  = u->o[i] * (1.0 - temp1 * temp1) * u->tilde_C[i] * (1.0 - temp2) * temp2;
			d_h_W_ih = 0.0;
			d_h_W_ix = d_h_b_i * u->x[i];
			temp2    = tanh(u->W_Cx * u->x[i] + u->b_C);
			d_h_b_C  = u->o[i] * (1.0 - temp1 * temp1) * u->i[i] * (1.0 - temp2 * temp2);
			d_h_W_Ch = 0.0;
			d_h_W_Cx = d_h_b_C * u->x[i];
			temp2    = 1.0 / (1.0 + exp(-(u->W_ox * u->x[i] + u->b_o)));
			d_h_b_o  = temp1 * (1.0 - temp2) * temp2;
			d_h_W_oh = 0.0;
			d_h_W_ox = d_h_b_o * u->x[i];
		} else {
			// 这里计算当 t>1 时的情况，需要算出的变量是所有的 d_h_xxx，也就是一共 12 个参数分成 12 个部分
			// Part 1
			temp1    = u->W_oh * u->h[i - 1] + u->W_ox * u->x[i] + u->b_o;
			temp1    = 1.0 / (1.0 + exp(-temp1));
			temp1    = temp1 * (1.0 - temp1) * u->W_oh * d_h_W_fh_last;
			temp2    = u->W_fh * u->h[i - 1] + u->W_fx * u->x[i] + u->b_f;
			temp2    = 1.0 / (1.0 + exp(-temp2));
			temp2    = temp2 * (1.0 - temp2) * (u->h[i - 1] + u->W_fh * d_h_W_fh_last);
			temp3    = u->W_ih * u->h[i - 1] + u->W_ix * u->x[i] + u->b_i;
			temp3    = 1.0 / (1.0 + exp(-temp3));
			temp3    = temp3 * (1.0 - temp3) * u->W_ih * d_h_W_fh_last;
			temp4    = u->W_Ch * u->h[i - 1] + u->W_Cx * u->x[i] + u->b_C;
			temp4    = tanh(temp4);
			temp4    = (1.0 - temp4 * temp4) * u->W_Ch * d_h_W_fh_last;
			d_C_W_fh = temp2 * u->C[i - 1] + u->f[i] * d_C_W_fh_last + temp3 * u->tilde_C[i] + u->i[i] * temp4;
			d_h_W_fh = temp1 * tanh(u->C[i]) + u->o[i] * (1 - tanh(u->C[i]) * tanh(u->C[i])) * d_C_W_fh;
			// Part 2
			temp1    = u->W_oh * u->h[i - 1] + u->W_ox * u->x[i] + u->b_o;
			temp1    = 1.0 / (1.0 + exp(-temp1));
			temp1    = temp1 * (1.0 - temp1) * u->W_oh * d_h_W_fx_last;
			temp2    = u->W_fh * u->h[i - 1] + u->W_fx * u->x[i] + u->b_f;
			temp2    = 1.0 / (1.0 + exp(-temp2));
			temp2    = temp2 * (1.0 - temp2) * (u->W_fh * d_h_W_fx_last + u->x[i]);
			temp3    = u->W_ih * u->h[i - 1] + u->W_ix * u->x[i] + u->b_i;
			temp3    = 1.0 / (1.0 + exp(-temp3));
			temp3    = temp3 * (1.0 - temp3) * u->W_ih * d_h_W_fx_last;
			temp4    = u->W_Ch * u->h[i - 1] + u->W_Cx * u->x[i] + u->b_C;
			temp4    = tanh(temp4);
			temp4    = (1.0 - temp4 * temp4) * u->W_Ch * d_h_W_fx_last;
			d_C_W_fx = temp2 * u->C[i - 1] + u->f[i] * d_C_W_fx_last + temp3 * u->tilde_C[i] + u->i[i] * temp4;
			d_h_W_fx = temp1 * tanh(u->C[i]) + u->o[i] * (1.0 - tanh(u->C[i]) * tanh(u->C[i])) * d_C_W_fx;
			// Part 3
			temp1   = u->W_oh * u->h[i - 1] + u->W_ox * u->x[i] + u->b_o;
			temp1   = 1.0 / (1.0 + exp(-temp1));
			temp1   = temp1 * (1.0 - temp1) * u->W_oh * d_h_b_f_last;
			temp2   = u->W_fh * u->h[i - 1] + u->W_fx * u->x[i] + u->b_f;
			temp2   = 1.0 / (1.0 + exp(-temp2));
			temp2   = temp2 * (1.0 - temp2) * (u->W_fh * d_h_b_f_last + 1.0);
			temp3   = u->W_ih * u->h[i - 1] + u->W_ix * u->x[i] + u->b_i;
			temp3   = 1.0 / (1.0 + exp(-temp3));
			temp3   = temp3 * (1.0 - temp3) * u->W_ih * d_h_b_f_last;
			temp4   = u->W_Ch * u->h[i - 1] + u->W_Cx * u->x[i] + u->b_C;
			temp4   = tanh(temp4);
			temp4   = (1.0 - temp4 * temp4) * u->W_Ch * d_h_b_f_last;
			d_C_b_f = temp2 * u->C[i - 1] + u->f[i] * d_C_b_f_last + temp3 * u->tilde_C[i] + u->i[i] * temp4;
			d_h_b_f = temp1 * tanh(u->C[i]) + u->o[i] * (1.0 - tanh(u->C[i]) * tanh(u->C[i])) * d_C_b_f;
			// Part 4
			temp1    = u->W_oh * u->h[i - 1] + u->W_ox * u->x[i] + u->b_o;
			temp1    = 1.0 / (1.0 + exp(-temp1));
			temp1    = temp1 * (1.0 - temp1) * u->W_oh * d_h_W_ih_last;
			temp2    = u->W_fh * u->h[i - 1] + u->W_fx * u->x[i] + u->b_f;
			temp2    = 1.0 / (1.0 + exp(-temp2));
			temp2    = temp2 * (1.0 - temp2) * u->W_fh * d_h_W_ih_last;
			temp3    = u->W_ih * u->h[i - 1] + u->W_ix * u->x[i] + u->b_i;
			temp3    = 1.0 / (1.0 + exp(-temp3));
			temp3    = temp3 * (1.0 - temp3) * (u->h[i - 1] + u->W_ih * d_h_W_ih_last);
			temp4    = u->W_Ch * u->h[i - 1] + u->W_Cx * u->x[i] + u->b_C;
			temp4    = tanh(temp4);
			temp4    = (1.0 - temp4 * temp4) * u->W_Ch * d_h_W_ih_last;
			d_C_W_ih = temp2 * u->C[i - 1] + u->f[i] * d_C_W_ih_last + temp3 * u->tilde_C[i] + u->i[i] * temp4;
			d_h_W_ih = temp1 * tanh(u->C[i]) + u->o[i] * (1.0 - tanh(u->C[i]) * tanh(u->C[i])) * d_C_W_ih;
			// Part 5
			temp1    = u->W_oh * u->h[i - 1] + u->W_ox * u->x[i] + u->b_o;
			temp1    = 1.0 / (1.0 + exp(-temp1));
			temp1    = temp1 * (1.0 - temp1) * u->W_oh * d_h_W_ix_last;
			temp2    = u->W_fh * u->h[i - 1] + u->W_fx * u->x[i] + u->b_f;
			temp2    = 1.0 / (1.0 + exp(-temp2));
			temp2    = temp2 * (1.0 - temp2) * u->W_fh * d_h_W_ix_last;
			temp3    = u->W_ih * u->h[i - 1] + u->W_ix * u->x[i] + u->b_i;
			temp3    = 1.0 / (1.0 + exp(-temp3));
			temp3    = temp3 * (1.0 - temp3) * (u->W_ih * d_h_W_ix_last + u->x[i]);
			temp4    = u->W_Ch * u->h[i - 1] + u->W_Cx * u->x[i] + u->b_C;
			temp4    = tanh(temp4);
			temp4    = (1.0 - temp4 * temp4) * u->W_Ch * d_h_W_ix_last;
			d_C_W_ix = temp2 * u->C[i - 1] + u->f[i] * d_C_W_ix_last + temp3 * u->tilde_C[i] + u->i[i] * temp4;
			d_h_W_ix = temp1 * tanh(u->C[i]) + u->o[i] * (1.0 - tanh(u->C[i]) * tanh(u->C[i])) * d_C_W_ix;
			// Part 6
			temp1   = u->W_oh * u->h[i - 1] + u->W_ox * u->x[i] + u->b_o;
			temp1   = 1.0 / (1.0 + exp(-temp1));
			temp1   = temp1 * (1.0 - temp1) * u->W_oh * d_h_b_i_last;
			temp2   = u->W_fh * u->h[i - 1] + u->W_fx * u->x[i] + u->b_f;
			temp2   = 1.0 / (1.0 + exp(-temp2));
			temp2   = temp2 * (1.0 - temp2) * u->W_fh * d_h_b_i_last;
			temp3   = u->W_ih * u->h[i - 1] + u->W_ix * u->x[i] + u->b_i;
			temp3   = 1.0 / (1.0 + exp(-temp3));
			temp3   = temp3 * (1.0 - temp3) * (u->W_ih * d_h_b_i_last + 1.0);
			temp4   = u->W_Ch * u->h[i - 1] + u->W_Cx * u->x[i] + u->b_C;
			temp4   = tanh(temp4);
			temp4   = (1.0 - temp4 * temp4) * u->W_Ch * d_h_b_i_last;
			d_C_b_i = temp2 * u->C[i - 1] + u->f[i] * d_C_b_i_last + temp3 * u->tilde_C[i] + u->i[i] * temp4;
			d_h_b_i = temp1 * tanh(u->C[i]) + u->o[i] * (1.0 - tanh(u->C[i]) * tanh(u->C[i])) * d_C_b_i;
			// Part 7
			temp1    = u->W_oh * u->h[i - 1] + u->W_ox * u->x[i] + u->b_o;
			temp1    = 1.0 / (1.0 + exp(-temp1));
			temp1    = temp1 * (1.0 - temp1) * u->W_oh * d_h_W_Ch_last;
			temp2    = u->W_fh * u->h[i - 1] + u->W_fx * u->x[i] + u->b_f;
			temp2    = 1.0 / (1.0 + exp(-temp2));
			temp2    = temp2 * (1.0 - temp2) * u->W_fh * d_h_W_Ch_last;
			temp3    = u->W_ih * u->h[i - 1] + u->W_ix * u->x[i] + u->b_i;
			temp3    = 1.0 / (1.0 + exp(-temp3));
			temp3    = temp3 * (1.0 - temp3) * u->W_ih * d_h_W_Ch_last;
			temp4    = u->W_Ch * u->h[i - 1] + u->W_Cx * u->x[i] + u->b_C;
			temp4    = tanh(temp4);
			temp4    = (1.0 - temp4 * temp4) * (u->h[i - 1] + u->W_Ch * d_h_W_Ch_last);
			d_C_W_Ch = temp2 * u->C[i - 1] + u->f[i] * d_C_W_Ch_last + temp3 * u->tilde_C[i] + u->i[i] * temp4;
			d_h_W_Ch = temp1 * tanh(u->C[i]) + u->o[i] * (1.0 - tanh(u->C[i]) * tanh(u->C[i])) * d_C_W_Ch;
			// Part 8
			temp1    = u->W_oh * u->h[i - 1] + u->W_ox * u->x[i] + u->b_o;
			temp1    = 1.0 / (1.0 + exp(-temp1));
			temp1    = temp1 * (1.0 - temp1) * u->W_oh * d_h_W_Cx_last;
			temp2    = u->W_fh * u->h[i - 1] + u->W_fx * u->x[i] + u->b_f;
			temp2    = 1.0 / (1.0 + exp(-temp2));
			temp2    = temp2 * (1.0 - temp2) * u->W_fh * d_h_W_Cx_last;
			temp3    = u->W_ih * u->h[i - 1] + u->W_ix * u->x[i] + u->b_i;
			temp3    = 1.0 / (1.0 + exp(-temp3));
			temp3    = temp3 * (1.0 - temp3) * u->W_ih * d_h_W_Cx_last;
			temp4    = u->W_Ch * u->h[i - 1] + u->W_Cx * u->x[i] + u->b_C;
			temp4    = tanh(temp4);
			temp4    = (1.0 - temp4 * temp4) * (u->W_Ch * d_h_W_Cx_last + u->x[i]);
			d_C_W_Cx = temp2 * u->C[i - 1] + u->f[i] * d_C_W_Cx_last + temp3 * u->tilde_C[i] + u->i[i] * temp4;
			d_h_W_Cx = temp1 * tanh(u->C[i]) + u->o[i] * (1.0 - tanh(u->C[i]) * tanh(u->C[i])) * d_C_W_Cx;
			// Part 9
			temp1   = u->W_oh * u->h[i - 1] + u->W_ox * u->x[i] + u->b_o;
			temp1   = 1.0 / (1.0 + exp(-temp1));
			temp1   = temp1 * (1.0 - temp1) * u->W_oh * d_h_b_C_last;
			temp2   = u->W_fh * u->h[i - 1] + u->W_fx * u->x[i] + u->b_f;
			temp2   = 1.0 / (1.0 + exp(-temp2));
			temp2   = temp2 * (1.0 - temp2) * u->W_fh * d_h_b_C_last;
			temp3   = u->W_ih * u->h[i - 1] + u->W_ix * u->x[i] + u->b_i;
			temp3   = 1.0 / (1.0 + exp(-temp3));
			temp3   = temp3 * (1.0 - temp3) * u->W_ih * d_h_b_C_last;
			temp4   = u->W_Ch * u->h[i - 1] + u->W_Cx * u->x[i] + u->b_C;
			temp4   = tanh(temp4);
			temp4   = (1.0 - temp4 * temp4) * (u->W_Ch * d_h_b_C_last + 1.0);
			d_C_b_C = temp2 * u->C[i - 1] + u->f[i] * d_C_b_C_last + temp3 * u->tilde_C[i] + u->i[i] * temp4;
			d_h_b_C = temp1 * tanh(u->C[i]) + u->o[i] * (1.0 - tanh(u->C[i]) * tanh(u->C[i])) * d_C_b_C;
			// Part 10
			temp1    = u->W_oh * u->h[i - 1] + u->W_ox * u->x[i] + u->b_o;
			temp1    = 1.0 / (1.0 + exp(-temp1));
			temp1    = temp1 * (1.0 - temp1) * (u->h[i - 1] + u->W_oh * d_h_W_oh_last);
			temp2    = u->W_fh * u->h[i - 1] + u->W_fx * u->x[i] + u->b_f;
			temp2    = 1.0 / (1.0 + exp(-temp2));
			temp2    = temp2 * (1.0 - temp2) * u->W_fh * d_h_W_oh_last;
			temp3    = u->W_ih * u->h[i - 1] + u->W_ix * u->x[i] + u->b_i;
			temp3    = 1.0 / (1.0 + exp(-temp3));
			temp3    = temp3 * (1.0 - temp3) * u->W_ih * d_h_W_oh_last;
			temp4    = u->W_Ch * u->h[i - 1] + u->W_Cx * u->x[i] + u->b_C;
			temp4    = tanh(temp4);
			temp4    = (1.0 - temp4 * temp4) * u->W_Ch * d_h_W_oh_last;
			d_C_W_oh = temp2 * u->C[i - 1] + u->f[i] * d_C_W_oh_last + temp3 * u->tilde_C[i] + u->i[i] * temp4;
			d_h_W_oh = temp1 * tanh(u->C[i]) + u->o[i] * (1.0 - tanh(u->C[i]) * tanh(u->C[i])) * d_C_W_oh;
			// Part 11
			temp1    = u->W_oh * u->h[i - 1] + u->W_ox * u->x[i] + u->b_o;
			temp1    = 1.0 / (1.0 + exp(-temp1));
			temp1    = temp1 * (1.0 - temp1) * (u->W_oh * d_h_W_ox_last + u->x[i]);
			temp2    = u->W_fh * u->h[i - 1] + u->W_fx * u->x[i] + u->b_f;
			temp2    = 1.0 / (1.0 + exp(-temp2));
			temp2    = temp2 * (1.0 - temp2) * u->W_fh * d_h_W_ox_last;
			temp3    = u->W_ih * u->h[i - 1] + u->W_ix * u->x[i] + u->b_i;
			temp3    = 1.0 / (1.0 + exp(-temp3));
			temp3    = temp3 * (1.0 - temp3) * u->W_ih * d_h_W_ox_last;
			temp4    = u->W_Ch * u->h[i - 1] + u->W_Cx * u->x[i] + u->b_C;
			temp4    = tanh(temp4);
			temp4    = (1.0 - temp4 * temp4) * u->W_Ch * d_h_W_ox_last;
			d_C_W_ox = temp2 * u->C[i - 1] + u->f[i] * d_C_W_ox_last + temp3 * u->tilde_C[i] + u->i[i] * temp4;
			d_h_W_ox = temp1 * tanh(u->C[i]) + u->o[i] * (1.0 - tanh(u->C[i]) * tanh(u->C[i])) * d_C_W_ox;
			// Part 12
			temp1   = u->W_oh * u->h[i - 1] + u->W_ox * u->x[i] + u->b_o;
			temp1   = 1.0 / (1.0 + exp(-temp1));
			temp1   = temp1 * (1.0 - temp1) * (u->W_oh * d_h_b_o_last + 1.0);
			temp2   = u->W_fh * u->h[i - 1] + u->W_fx * u->x[i] + u->b_f;
			temp2   = 1.0 / (1.0 + exp(-temp2));
			temp2   = temp2 * (1.0 - temp2) * u->W_fh * d_h_b_o_last;
			temp3   = u->W_ih * u->h[i - 1] + u->W_ix * u->x[i] + u->b_i;
			temp3   = 1.0 / (1.0 + exp(-temp3));
			temp3   = temp3 * (1.0 - temp3) * u->W_ih * d_h_b_o_last;
			temp4   = u->W_Ch * u->h[i - 1] + u->W_Cx * u->x[i] + u->b_C;
			temp4   = tanh(temp4);
			temp4   = (1.0 - temp4 * temp4) * u->W_Ch * d_h_b_o_last;
			d_C_b_o = temp2 * u->C[i - 1] + u->f[i] * d_C_b_o_last + temp3 * u->tilde_C[i] + u->i[i] * temp4;
			d_h_b_o = temp1 * tanh(u->C[i]) + u->o[i] * (1.0 - tanh(u->C[i]) * tanh(u->C[i])) * d_C_b_o;
		}
		d_E_W_fh = d_E_W_fh + 2.0 / length * (u->h[i] - u->hat_h[i]) * d_h_W_fh;
		d_E_W_fx = d_E_W_fx + 2.0 / length * (u->h[i] - u->hat_h[i]) * d_h_W_fx;
		d_E_b_f  = d_E_b_f + 2.0 / length * (u->h[i] - u->hat_h[i]) * d_h_b_f;
		d_E_W_ih = d_E_W_ih + 2.0 / length * (u->h[i] - u->hat_h[i]) * d_h_W_ih;
		d_E_W_ix = d_E_W_ix + 2.0 / length * (u->h[i] - u->hat_h[i]) * d_h_W_ix;
		d_E_b_i  = d_E_b_i + 2.0 / length * (u->h[i] - u->hat_h[i]) * d_h_b_i;
		d_E_W_Ch = d_E_W_Ch + 2.0 / length * (u->h[i] - u->hat_h[i]) * d_h_W_Ch;
		d_E_W_Cx = d_E_W_Cx + 2.0 / length * (u->h[i] - u->hat_h[i]) * d_h_W_Cx;
		d_E_b_C  = d_E_b_C + 2.0 / length * (u->h[i] - u->hat_h[i]) * d_h_b_C;
		d_E_W_oh = d_E_W_oh + 2.0 / length * (u->h[i] - u->hat_h[i]) * d_h_W_oh;
		d_E_W_ox = d_E_W_ox + 2.0 / length * (u->h[i] - u->hat_h[i]) * d_h_W_ox;
		d_E_b_o  = d_E_b_o + 2.0 / length * (u->h[i] - u->hat_h[i]) * d_h_b_o;

		d_h_W_fh_last = d_h_W_fh;
		d_h_W_fx_last = d_h_W_fx;
		d_h_b_f_last  = d_h_b_f;
		d_h_W_ih_last = d_h_W_ih;
		d_h_W_ix_last = d_h_W_ix;
		d_h_b_i_last  = d_h_b_i;
		d_h_W_Ch_last = d_h_W_Ch;
		d_h_W_Cx_last = d_h_W_Cx;
		d_h_b_C_last  = d_h_b_C;
		d_h_W_oh_last = d_h_W_oh;
		d_h_W_ox_last = d_h_W_ox;
		d_h_b_o_last  = d_h_b_o;

		d_C_W_fh_last = d_C_W_fh;
		d_C_W_fx_last = d_C_W_fx;
		d_C_b_f_last  = d_C_b_f;
		d_C_W_ih_last = d_C_W_ih;
		d_C_W_ix_last = d_C_W_ix;
		d_C_b_i_last  = d_C_b_i;
		d_C_W_Ch_last = d_C_W_Ch;
		d_C_W_Cx_last = d_C_W_Cx;
		d_C_b_C_last  = d_C_b_C;
		d_C_W_oh_last = d_C_W_oh;
		d_C_W_ox_last = d_C_W_ox;
		d_C_b_o_last  = d_C_b_o;
	}
	u->W_fh = u->W_fh - lr * d_E_W_fh;
	u->W_fx = u->W_fx - lr * d_E_W_fx;
	u->b_f  = u->b_f - lr * d_E_b_f;
	u->W_ih = u->W_ih - lr * d_E_W_ih;
	u->W_ix = u->W_ix - lr * d_E_W_ix;
	u->b_i  = u->b_i - lr * d_E_b_i;
	u->W_Ch = u->W_Ch - lr * d_E_W_Ch;
	u->W_Cx = u->W_Cx - lr * d_E_W_Cx;
	u->b_C  = u->b_C - lr * d_E_b_C;
	u->W_oh = u->W_oh - lr * d_E_W_oh;
	u->W_ox = u->W_ox - lr * d_E_W_ox;
	u->b_o  = u->b_o - lr * d_E_b_o;
	return 1;
}

int opus_lstm_unit_save(opus_lstm_unit *u, char *file_name)
{
	FILE *file;
	int   write = 0;
	if (NULL == u) {
		return 0;
	}
	if (0 != u->error_no) {
		return 0;
	}
	file = fopen(file_name, "w");
	if (NULL == file) {
		return 0;
	}
	write += fprintf(file, "0\n"); // version
	write += fprintf(file, "%d\n", u->length);
	write += fprintf(file, "%lf\n", u->W_fh);
	write += fprintf(file, "%lf\n", u->W_fx);
	write += fprintf(file, "%lf\n", u->b_f);
	write += fprintf(file, "%lf\n", u->W_ih);
	write += fprintf(file, "%lf\n", u->W_ix);
	write += fprintf(file, "%lf\n", u->b_i);
	write += fprintf(file, "%lf\n", u->W_Ch);
	write += fprintf(file, "%lf\n", u->W_Cx);
	write += fprintf(file, "%lf\n", u->b_C);
	write += fprintf(file, "%lf\n", u->W_oh);
	write += fprintf(file, "%lf\n", u->W_ox);
	write += fprintf(file, "%lf\n", u->b_o);
	fclose(file);
	return write;
}


opus_lstm_seq *opus_lstm_seq_create(int length, int lstm_num) // TODO az13js 创建 LSTM 多单元对象
{
	int               i;
	opus_lstm_seq    *seq;
	opus_lstm_node   *node;
	opus_lstm_node   *last_node = NULL;
	OPUS_RETURN_IF(NULL, length < 1 || lstm_num < 1);
	seq = (opus_lstm_seq *) malloc(sizeof(opus_lstm_seq));
	OPUS_RETURN_IF(NULL, !seq);
	seq->length   = length;
	seq->lstm_num = lstm_num;
	for (i = 0; i < lstm_num; i++) {
		node = (opus_lstm_node *) malloc(sizeof(opus_lstm_node));
		OPUS_RETURN_IF(NULL, !node); /* TODO: free */
		node->before = last_node;
		node->lstm   = opus_lstm_unit_create(length);
		node->after  = NULL;
		if (NULL != last_node) {
			free(node->lstm->x);
			node->lstm->x = last_node->lstm->h;
			free(last_node->lstm->hat_h);
			last_node->after = node;
		}
		last_node = node;
		if (0 == i) {
			seq->first = node;
		}
		if (lstm_num - 1 == i) {
			seq->end = node;
		}
	}
	return seq;
}

char opus_lstm_seq_run(opus_lstm_seq *s)
{
	opus_lstm_node *sequence;
	if (NULL == s) {
		return 0;
	}
	sequence = s->first;
	while (sequence) {
		opus_lstm_unit_run(sequence->lstm);
		sequence = sequence->after;
	}
	return 1;
}

char opus_lstm_seq_train(opus_lstm_seq *s, double lr)
{
	return 1;
}
