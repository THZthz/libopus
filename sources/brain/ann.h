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

#define ANN_ACTIVATION ann_sigmoid_
#define ANN_DERIVATION ann_sigmoid_derivation_
#define ANN_COST ann_cost_
#define ANN_COST_DERIVATION ann_cost_derivation_

typedef struct ann ann_t;
typedef double     ann_real;

typedef ann_real (*ann_activation_cb)(ann_real v);
typedef ann_real (*ann_activation_derivation_cb)(ann_real v);

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
struct ann {
	uint32_t  n_layer_, n_input_, n_output_, n_neuron_;
	uint32_t *nmap_;  /* neuron count of each layer */
	uint32_t *tnmap_; /* total neurons count before the layer, does not contain bias neuron(which is added automatically) */
	uint32_t *wmap_;  /* total weights count before the layer, count the link of bias neuron */
	ann_real *weights;
	ann_real *outputs_; /* the output value of each neuron */
	ann_real *errors_; /* used for back propagation */
};

ann_t *ann_init(ann_t *net, uint32_t n_layer, const uint32_t *nmap);
ann_t *ann_create(uint32_t n_layer, const uint32_t *nmap);
void   ann_done(ann_t *net);
void   ann_destroy(ann_t *net);

void      ann_randomize(ann_t *net);
ann_real *ann_get_weights(ann_t *net, uint32_t l, uint32_t n);
void      ann_set_input(ann_t *net, const ann_real *input_data);
ann_real *ann_get_output(ann_t *net);
void      ann_feed_forward(ann_t *net);
void      ann_back_propagate(ann_t *net, const ann_real *target_outputs, ann_real learning_rate);
ann_real *ann_predict(ann_t *net, const ann_real *input);
void      ann_learn(ann_t *net, const ann_real *input, const ann_real *target_output, ann_real learning_rate);

ann_t *ann_get_copy(ann_t *net);
void   ann_save(ann_t *net, FILE *out);
ann_t *ann_load(FILE *in);
void   ann_console(ann_t *net);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* ANN_H */
