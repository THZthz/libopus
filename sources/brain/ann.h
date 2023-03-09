/**
 * @file ann.h
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/10/18
 *
 * @brief A simple implementation of Artificial Neural Network, feed-forward multi-layer perceptron with back-propagation
 * 		as training method.
 *
 * @development_log
 *
 */
#ifndef ANN_H
#define ANN_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdio.h>

#include "math/math.h"

#define OPUS_ANN_ACTIVATION ann_sigmoid_
#define OPUS_ANN_DERIVATION ann_sigmoid_derivation_
#define OPUS_ANN_COST ann_cost_
#define OPUS_ANN_COST_DERIVATION ann_cost_derivation_

typedef struct opus_ann opus_ann;

typedef opus_real (*opus_ann_activation_cb)(opus_real v);
typedef opus_real (*opus_ann_activation_derivation_cb)(opus_real v);

/*
 * The neurons are arranged this:
 *
 *      INPUT_LAYER -----> bias n0 n1 n2 ...
 *          ...
 *      HIDDEN_LAYER_1 --> bias n0 n1 n2 ...
 *      HIDDEN_LAYER_2 --> bias n0 n1 n2 ...
 *          ...
 *      OUTPUT_LAYER ----> n0 n1 n2 ...
 *
 *      note: neuron can only connect neurons in the layer before it
 *
 * The weights array are arranged by a certain rule:
 *      1` INPUT_LAYER has no weights
 *      2` The weights array of each neuron in a HIDDEN_LAYER(or OUTPUT_LAYER) has a consistent one-to-one match with the layer
 *              before it(include the input layer).
 *      3` ...
 *
 *
 */
struct opus_ann {
	uint32_t   n_layer_, n_input_, n_output_, n_neuron_;
	uint32_t  *nmap_;  /* neuron count of each layer */
	uint32_t  *tnmap_; /* total neurons count before the layer, does not contain bias neuron(which is added automatically) */
	uint32_t  *wmap_;  /* total weights count before the layer, count the link of bias neuron */
	opus_real *weights;
	opus_real *outputs_; /* the output value of each neuron */
	opus_real *errors_;  /* used for back propagation */
};

opus_ann *opus_ann_init(opus_ann *net, uint32_t n_layer, const uint32_t *nmap);
opus_ann *opus_ann_create(uint32_t n_layer, const uint32_t *nmap);
void      opus_ann_done(opus_ann *net);
void      opus_ann_destroy(opus_ann *net);

void       opus_ann_randomize(opus_ann *net);
opus_real *opus_ann_get_weights(opus_ann *net, uint32_t l, uint32_t n);
void       opus_ann_set_input(opus_ann *net, const opus_real *input_data);
opus_real *opus_ann_get_output(opus_ann *net);
void       opus_ann_feed_forward(opus_ann *net);
void       opus_ann_back_propagate(opus_ann *net, const opus_real *target_outputs, opus_real learning_rate);
opus_real *opus_ann_predict(opus_ann *net, const opus_real *input);
void       opus_ann_learn(opus_ann *net, const opus_real *input, const opus_real *target_output, opus_real learning_rate);

opus_ann *opus_ann_get_copy(opus_ann *net);
void      opus_ann_save(opus_ann *net, FILE *out);
opus_ann *opus_ann_load(FILE *in);
void      opus_ann_console(opus_ann *net);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* ANN_H */
