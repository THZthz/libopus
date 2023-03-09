#ifndef LSTMLIB
#define LSTMLIB

typedef struct opus_lstm opus_lstm_unit;

typedef struct opus_lstm_node opus_lstm_node;
typedef struct opus_lstm_seq  opus_lstm_seq;

struct opus_lstm {
	int     length;
	double *x; /* input data */
	double *h;
	double *f;
	double *i;
	double *tilde_C;
	double *C;
	double *o;
	double *hat_h; /* desired output */
	double  W_fh;
	double  W_fx;
	double  b_f;
	double  W_ih;
	double  W_ix;
	double  b_i;
	double  W_Ch;
	double  W_Cx;
	double  b_C;
	double  W_oh;
	double  W_ox;
	double  b_o;
	int     error_no;
	char   *error_msg;
};

struct opus_lstm_node {
	opus_lstm_unit *lstm;
	opus_lstm_node *before, *after;
};

struct opus_lstm_seq {
	int length;
	int lstm_num;

	opus_lstm_node *first, *end;
};

opus_lstm_seq *opus_lstm_seq_create(int length, int lstm_num);
char           opus_lstm_seq_run(opus_lstm_seq *s);
char           opus_lstm_seq_train(opus_lstm_seq *s, double lr);

opus_lstm_unit *opus_lstm_unit_create(int length);
void            opus_lstm_unit_destroy(opus_lstm_unit *u);
char            opus_lstm_unit_random_params(opus_lstm_unit *u, double min, double max);
char            opus_lstm_unit_predict(opus_lstm_unit *u, double *input, double *output);
char            opus_lstm_unit_run(opus_lstm_unit *u);
double          opus_lstm_unit_get_mse(opus_lstm_unit *u);
char            opus_lstm_unit_train(opus_lstm_unit *u, double lr);
int             opus_lstm_unit_save(opus_lstm_unit *u, char *file_name);

#endif
