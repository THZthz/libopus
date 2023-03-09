/**
 * @file ann.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/10/18
 *
 * @example
 *
 * @development_log
 *
 */


#include "brain/ann.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils/utils.h"


/* input layer has no weights, so "l" could not be "0" */
#define weights_layer(net, l) ((net)->weights + (net)->wmap_[l])
#define outputs_layer(net, l) ((net)->outputs_ + (net)->tnmap_[l])

#define errors_layer(net, l) ((net)->errors_ + (net)->tnmap_[l] - (net)->n_input_)

/* the offset of neuron(n) in the weights layer(l) */
#define weights_neuron(net, l, n) (weights_layer((net), (l)) + (n) *link_count((net), (l)))

/* return the count of neuron on the layer "l" */
#define neuron_count(net, l) ((net)->nmap_[l])

/* return the link count of the neuron on layer "l", "l" should not be "0" */
#define link_count(net, l) ((net)->nmap_[(l) -1] + 1)

static OPUS_INLINE double sigmoid_(double a)
{
	return 1.0 / (1 + exp(-a));
}

static OPUS_INLINE double sigmoid_derivation_(double a)
{
	return a * (1.0 - a);
}

static OPUS_INLINE opus_real ann_sigmoid_(opus_real v)
{
	static const double   max = 15, min = -15;
	static const uint64_t size = 4096;

	static int    is_inited = 0;
	static double interval;
	static double table[4096];

	uint64_t j;

	if (!is_inited) {
		double f = (double) (max - min) / (double) size;
		int    i;

		for (i = 0; i < size; ++i)
			table[i] = sigmoid_((double) min + f * i);

		interval  = (double) size / (double) (max - min);
		is_inited = 1;
	}

	if (v < min) return table[0];
	if (v >= max) return table[size - 1];

	j = (uint64_t) ((v - min) * interval + 0.5);

	if (OPUS_UNLIKELY(j >= size)) return table[size - 1];

	return table[j];
}

static OPUS_INLINE opus_real ann_sigmoid_derivation_(opus_real v)
{
	return (opus_real) sigmoid_derivation_((double) v);
}

static OPUS_INLINE opus_real ann_cost_(opus_real y, opus_real a)
{
	return (opus_real) 0.5 * (y - a) * (y - a);
}

static OPUS_INLINE opus_real ann_cost_derivation_(opus_real y, opus_real a)
{
	return y - a;
}

static OPUS_INLINE opus_real dot_(const opus_real *a, const opus_real *b, uint32_t len)
{
	opus_real sum = 0;
	uint32_t  i;
	for (i = 0; i < len; i++) sum += a[i] * b[i];
	return sum;
}

opus_ann *opus_ann_create(uint32_t n_layer, const uint32_t *nmap)
{
	opus_ann *net = (opus_ann *) malloc(sizeof(opus_ann));
	OPUS_NOT_NULL(net);
	return opus_ann_init(net, n_layer, nmap);
}

opus_ann *opus_ann_init(opus_ann *net, uint32_t n_layer, const uint32_t *nmap)
{
	uint32_t i;

	net->n_layer_  = n_layer;
	net->n_input_  = nmap[0];
	net->n_output_ = nmap[n_layer - 1];

	net->nmap_  = (uint32_t *) malloc(sizeof(uint32_t) * n_layer);
	net->tnmap_ = (uint32_t *) malloc(sizeof(uint32_t) * (n_layer + 1));
	net->wmap_  = (uint32_t *) malloc(sizeof(uint32_t) * (n_layer + 1));

	/* set neuron map information */
	net->n_neuron_ = 0;
	net->tnmap_[0] = 0;
	for (i = 0; i < n_layer; i++) {
		net->nmap_[i]      = nmap[i];
		net->tnmap_[i + 1] = nmap[i] + net->tnmap_[i];
		net->n_neuron_ += nmap[i];
	}

	/* allocate memory for weights */
	{
		uint32_t n_input  = net->n_input_;
		uint32_t n_neuron = net->n_neuron_;
		uint32_t n_link;

		/* calculate links */
		n_link        = 0; /* input layer does not have any link */
		net->wmap_[0] = 0;
		net->wmap_[1] = 0;
		for (i = 1; i < n_layer; i++) {
			n_link += (nmap[i - 1] + 1) * nmap[i];
			if (i >= 2) net->wmap_[i] = net->wmap_[i - 1] + (nmap[i - 2] + 1) * nmap[i - 1];
		}
		net->wmap_[i] = net->wmap_[i - 1] + (nmap[i - 2] + 1) * nmap[i - 1];

		net->errors_  = (opus_real *) malloc(sizeof(opus_real) * (n_neuron - n_input));
		net->weights  = (opus_real *) malloc(sizeof(opus_real) * n_link);
		net->outputs_ = (opus_real *) malloc(sizeof(opus_real) * n_neuron);

		memset(net->weights, 0, sizeof(opus_real) * n_link);
		memset(net->outputs_, 0, sizeof(opus_real) * n_neuron);
		memset(net->errors_, 0, sizeof(opus_real) * (n_neuron - n_input));
	}

	return net;
}

void opus_ann_done(opus_ann *net)
{
	if (net->weights) free(net->weights);
	if (net->errors_) free(net->errors_);
	if (net->outputs_) free(net->outputs_);
}

void opus_ann_destroy(opus_ann *net)
{
	opus_ann_done(net);
	free(net);
}

/* randomize all the weights of the network from -0.5 to 0.5  */
void opus_ann_randomize(opus_ann *net)
{
	uint32_t i, n = net->wmap_[net->n_layer_];
	for (i = 0; i < n; ++i) {
		opus_real r     = (opus_real) rand() / RAND_MAX;
		net->weights[i] = r - (opus_real) 0.5;
	}
}

opus_real *opus_ann_predict(opus_ann *net, const opus_real *input)
{
	opus_ann_set_input(net, input);
	opus_ann_feed_forward(net);
	return opus_ann_get_output(net);
}

void opus_ann_learn(opus_ann *net, const opus_real *input, const opus_real *target_output, opus_real learning_rate)
{
	opus_ann_predict(net, input);
	opus_ann_back_propagate(net, target_output, learning_rate);
}

void opus_ann_set_input(opus_ann *net, const opus_real *input_data)
{
	/* store the input data in the output field of input neurons */
	memcpy(net->outputs_, input_data, sizeof(opus_real) * net->nmap_[0]);
}

opus_real *opus_ann_get_output(opus_ann *net)
{
	return outputs_layer(net, net->n_layer_ - 1);
}

void opus_ann_feed_forward(opus_ann *net)
{
	uint32_t   i, j;
	opus_real *input = outputs_layer(net, 0), *output;

	/* ignore input layer */
	/* we do not need to calculate the output of the neurons on input layer */
	for (i = 1; i < net->n_layer_; i++) {
		output = outputs_layer(net, i);

		for (j = 0; j < neuron_count(net, i); j++) {
			opus_real *weights = weights_neuron(net, i, j);
			opus_real  sum     = -1 * weights[0] + dot_(weights + 1, input, link_count(net, i) - 1);
			output[j]          = OPUS_ANN_ACTIVATION(sum);
		}

		/* every layer takes the output of the previous layer as input */
		input = output;
	}
}

/**
 * @brief Use back propagation to update the weights of each link.\n
 * 		Remember to feed forward the network once before you propagate it.\n
 * 		Reference for formula: \b https://blog.csdn.net/weixin_41799019/article/details/117353078.\n
 * 		Reference for algorithm: \b https://blog.csdn.net/u014313009/article/details/51039334.\n
 * 		These articles explains almost every details of the implementation below.
 * @param net
 * @param target_outputs
 * @param learning_rate
 */
void opus_ann_back_propagate(opus_ann *net, const opus_real *target_outputs, opus_real learning_rate)
{
	uint32_t   i, j, k;
	opus_real *inputs, *outputs, *errors, *targets, *weights;

	/* calculate errors for output layer */
	outputs = outputs_layer(net, net->n_layer_ - 1);
	errors  = errors_layer(net, net->n_layer_ - 1);
	targets = (opus_real *) target_outputs;
	for (i = 0; i < net->n_output_; ++i)
		errors[i] = OPUS_ANN_COST_DERIVATION(targets[i], outputs[i]) * OPUS_ANN_DERIVATION(outputs[i]);

	/* back-propagate the errors of neurons on each layer except input layer */
	for (i = net->n_layer_ - 2; i != 0; --i) {
		opus_real *next_errors;

		next_errors = errors_layer(net, i + 1);
		outputs     = outputs_layer(net, i);
		errors      = errors_layer(net, i);

		/* for each neuron on this layer */
		for (j = 0; j < net->nmap_[i]; j++) {
			opus_real error = 0;

			/* calculate forwarded error from following layer */
			for (k = 0; k < net->nmap_[i + 1]; ++k)
				error += next_errors[k] * weights_neuron(net, i + 1, k)[j + 1];

			/* calculate the error of this neuron */
			errors[j] = OPUS_ANN_DERIVATION(outputs[j]) * error;
		}
	}

	/* gradient descent to update all the weight and bias */
	for (i = net->n_layer_ - 1; i != 0; --i) {
		errors = errors_layer(net, i);
		inputs = outputs_layer(net, i - 1); /* take the previous layer's output as the output layer's input */

		for (j = 0; j < net->nmap_[i]; ++j) {
			/* get the weights array of the neuron */
			weights = weights_neuron(net, i, j);

			weights[j] += errors[j] * learning_rate * -1.0; /* for bias neuron */
			for (k = 1; k < net->nmap_[i - 1] + 1; ++k) {
				weights[k] += errors[j] * learning_rate * inputs[k - 1];
			}
		}
	}
}

/* get the weights array for the "n"th neuron on the layer "l" */
opus_real *opus_ann_get_weights(opus_ann *net, uint32_t l, uint32_t n)
{
	return weights_neuron(net, l, n);
}

void opus_ann_console(opus_ann *net)
{
	FILE    *out = stdout;
	uint32_t i, j, k;

	/* print links and neurons info */
	fprintf(out, "neuron map  ");
	for (i = 0; i < net->n_layer_; i++) fprintf(out, "%d ", neuron_count(net, i));
	fprintf(out, "\n");

	/* print network input and output data */
	fprintf(out, "input data: ");
	for (j = 0; j < net->n_input_; j++) fprintf(out, " %+.3f", outputs_layer(net, 0)[j]);
	fprintf(out, "\n");
	fprintf(out, "output data: ");
	for (j = 0; j < net->n_output_; j++) fprintf(out, " %+.3f", outputs_layer(net, net->n_layer_ - 1)[j]);
	fprintf(out, "\n");

	/* input layer */
	fprintf(out, "[INPUT #0]-------------------------------\n\t(%d neurons with no info of weights)\n", neuron_count(net, 0));

	/* hidden layer */
	for (i = 1; i < net->n_layer_ - 1; i++) {
		fprintf(out, "[HIDDEN #%d]------------------------------\n", i);
		for (j = 0; j < neuron_count(net, i); j++) { /* for each neuron on the hidden layer */
			fprintf(out, "\tneuron #%d:", j);
			for (k = 0; k < link_count(net, i); k++) fprintf(out, " %+.3f", weights_neuron(net, i, j)[k]);
			fprintf(out, "\n");
		}
	}

	/* output layer */
	fprintf(out, "[OUTPUT #%d]-------------------------------\n", net->n_layer_ - 1);
	i = net->n_layer_ - 1;
	for (j = 0; j < neuron_count(net, i); j++) { /* for each neuron on the output layer */
		fprintf(out, "\tneuron #%d:", j);
		for (k = 0; k < link_count(net, i); k++) fprintf(out, " %+.3f", weights_neuron(net, i, j)[k]);
		fprintf(out, "\n");
	}
}

opus_ann *opus_ann_get_copy(opus_ann *net)
{
	opus_ann *copy = opus_ann_create(net->n_layer_, net->nmap_);
	memcpy(copy->weights, net->weights, sizeof(opus_real) * net->wmap_[net->n_layer_]);
	return copy;
}

opus_ann *opus_ann_load(FILE *in)
{
	opus_ann   *net;
	uint32_t    n_layer, *nmap;

	fread(&n_layer, sizeof(uint32_t), 1, in);
	nmap = (uint32_t *) malloc(sizeof(uint32_t) * n_layer);
	fread(nmap, sizeof(uint32_t), n_layer, in);

	net = opus_ann_create(n_layer, nmap);
	free(nmap);

	fread(net->weights, sizeof(opus_real), net->wmap_[net->n_layer_], in);

	return net;
}

void opus_ann_save(opus_ann *net, FILE *out)
{
	size_t c;
	if ((c = fwrite(&net->n_layer_, sizeof(uint32_t), 1, out)) == 0) OPUS_WARNING("ANN::save::Failed to write data(1)\n");
	if ((c = fwrite(net->nmap_, sizeof(uint32_t), net->n_layer_, out)) == 0) OPUS_WARNING("ANN::save::Failed to write data(2)\n");
	if ((c = fwrite(net->weights, sizeof(opus_real), net->wmap_[net->n_layer_], out)) == 0) OPUS_WARNING("ANN::save::Failed to write data(3)\n");
}
