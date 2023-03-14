/**
 * @file autodiff_demo.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2023/1/1
 *
 * @example
 *
 * @development_log
 *
 */

#include <stdlib.h>

#include "math/math.h"
#include "math/autodiff.h"
#include "utils/utils.h"

#define AD_MALLOC_(_v, _s)           \
	do {                            \
		(_v) = (void *) malloc(_s); \
	} while (0)
#define AD_CALLOC_(_v, _n, _ele_size)           \
	do {                            \
		(_v) = (void *) OPUS_CALLOC((_n), (_ele_size)); \
	} while (0)
#define AD_FREE_(_v) \
	do {                \
		if (_v) free(_v);    \
		(_v) = NULL; \
	} while (0)

/* major type of node */
enum {
	AD_VAR_IN = 0x01, /* 0001 */
	AD_VAR_OUT = 0x02,  /* 0010 */
	AD_FUNC = 0x3, /* 0011 */
	AD_OPERATOR = 0x4 , /* 0100 */
};

typedef opus_real       ad_real;
typedef unsigned int    ad_count_t;
typedef struct ad_flags ad_flags_t;
typedef struct ad_node  ad_node_t;
typedef struct ad_net   ad_net_t;

struct ad_flags {
	unsigned int type : 4;
	unsigned int sub_type : 4;
};

struct ad_net {
	ad_count_t n_in, n_out;
	ad_real *in, *out;

	ad_count_t n_bin_, c_bin_; /* length and the capacity of the bin */
	ad_node_t *nodes_bin_;
};

/* simply a header of all kinds of nodes, that means "flags" field must be present */
struct ad_node {
	ad_flags_t flags;
};

/* external node which acts as dependent variables like X1, X2, etc., namely input */
struct ad_node_in {
	ad_flags_t flags;

	ad_real *value; /* pointer to the corresponding value in ad_net_t */
};


/* external node which acts as independent variables like Y1, Y2, etc., namely output */
ad_node_t *ad_node_out()
{
}

static int bin_init(ad_net_t *net)
{
	OPUS_RETURN_IF(0, !net);
	net->c_bin_ = 10;
	net->n_bin_ = 0;
	AD_MALLOC_(net->nodes_bin_, net->c_bin_);
	if (!net->nodes_bin_) {
		net->c_bin_ = 0;
		return 0;
	}
	return 1;
}

static void bin_done(ad_net_t *net)
{
	net->c_bin_ = 0;
	net->n_bin_ = 0;
	AD_FREE_(net->nodes_bin_);
}

static ad_node_t *bin_get_node(ad_net_t *net)
{
	ad_node_t *n;
	AD_MALLOC_(n, sizeof(ad_node_t)); /* TODO */
	return n;
}

static void bin_free_node(ad_node_t *node)
{
	AD_FREE_(node); /* TODO */
}

ad_net_t *ad_net_create(ad_count_t n_in, ad_count_t n_out)
{
	ad_net_t *net;

	/* allocate memory */
	AD_CALLOC_(net, 1, sizeof(ad_net_t));
	if (!net) goto no_memory;
	AD_MALLOC_(net->in, sizeof(ad_real) * n_in);
	AD_MALLOC_(net->out, sizeof(ad_real) * n_out);
	if (!net->in || !net->out) goto no_memory;

	/* create bin */
	bin_init(net);

	net->n_in = n_in;
	net->n_out = n_out;

	/* create node for "in" and "out" */
	{
		size_t i;
		for (i = 0; i < n_in; i++) {
			ad_node_t *in = bin_get_node(net);
			if (!in) goto no_memory;
		}
	}

	return net;
no_memory:
	if (net) {
		AD_FREE_(net->nodes_bin_);
		AD_FREE_(net->in);
		AD_FREE_(net->out);
	}
	return NULL;
}

void ad_net_destroy(ad_net_t *net)
{
	bin_done(net);
	AD_FREE_(net);
}

int main()
{
	ad_net_t *net = ad_net_create(2, 1);


	ad_net_destroy(net);

	return 0;
}
