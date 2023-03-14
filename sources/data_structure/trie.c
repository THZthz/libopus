#include "data_structure/trie.h"
#include "utils/utils.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define get_child(t) ((struct trie_ptr *) ((char *) (t) + sizeof(trie_t)))

/**
 * @brief simple implementation of \b node \b of \b stack for non-recursive traversal.
 */
struct trie_stack_node {
	trie_t *trie;
	int     i;
};

struct trie_stack {
	struct trie_stack_node *stack;
	/**
	 * number of nodes in the stack
	 */
	size_t fill;
	/**
	 * number of available slots to store nodes
	 */
	size_t size;
};

/**
 * init the stack
 * @param s
 * @return -1 (ETRIEFAIL) if failed, 0 (ETRIEOK) if success
 */
static /* inline */ int trie_stack_init(struct trie_stack *s)
{
	s->size  = 256;
	s->fill  = 0;
	s->stack = OPUS_MALLOC(s->size * sizeof(struct trie_stack_node));
	return !s->stack ? ETRIEFAIL : ETRIEOK;
}

static /* inline */ void trie_stack_free(struct trie_stack *s)
{
	OPUS_FREE(s->stack);
	s->stack = NULL;
}

static /* inline */ int trie_stack_grow(struct trie_stack *s)
{
	size_t new_size = s->size * 2 * sizeof(struct trie_stack_node);

	struct trie_stack_node *resize = OPUS_REALLOC(s->stack, new_size);
	if (!resize) {
		trie_stack_free(s);
		return ETRIEFAIL;
	}
	s->size *= 2;
	s->stack = resize;
	return ETRIEOK;
}

static /* inline */ int trie_stack_push(struct trie_stack *s, trie_t *trie)
{
	struct trie_stack_node node;
	if (s->fill == s->size && trie_stack_grow(s) != 0) return ETRIEFAIL;
	node.trie           = trie;
	node.i              = 0;
	s->stack[s->fill++] = node;
	return ETRIEOK;
}

static /* inline */ trie_t *trie_stack_pop(struct trie_stack *s)
{
	return s->stack[--s->fill].trie;
}

/**
 * return the top node of the  stack
 * @param s
 * @return
 */
static /* inline */ struct trie_stack_node *trie_stack_peek(struct trie_stack *s)
{
	return &s->stack[s->fill - 1];
}


/**
 * @brief allocate a trie tree on heap and return its pointer
 * @return pointer to a trie on heap, NULL if `OPUS_MALLOC` fails
 */
trie_t *trie_create(void)
{
	/* Root never needs to be resized. */
	size_t  tail_size = sizeof(struct trie_ptr) * 255;
	trie_t *root      = (trie_t *) OPUS_MALLOC(sizeof(*root) + tail_size);
	if (!root)
		return NULL;
	root->size       = 255;
	root->n_children = 0;
	root->data       = NULL;
	return root;
}

/**
 * destroy a trie, note that it will not OPUS_FREE any resources of the data you insert
 * @param trie
 * @return 0 (ETRIEOK) if success, 1 (does not mean anything) otherwise
 */
int trie_destroy(trie_t *trie)
{
	struct trie_stack       stack, *s = &stack;
	struct trie_stack_node *node;
	if (trie_stack_init(s) != ETRIEOK)
		return 1;
	trie_stack_push(s, trie); /* first push always successful */
	while (s->fill > 0) {
		node = trie_stack_peek(s);
		/* check if all the children of current trie is correctly freed */
		if (node->i < node->trie->n_children) {
			int i = node->i++; /* update the index of the next trie to OPUS_FREE in current trie */
			/* push its children first and deal with it */
			if (trie_stack_push(s, get_child(node->trie)[i].trie) != ETRIEOK)
				return 1;
		} else {
			OPUS_FREE_R(trie_stack_pop(s)); /* free current trie */
		}
	}
	trie_stack_free(s);
	return ETRIEOK;
}

/**
 * use binary search
 * @param self
 * @param child
 * @param ptr
 * @param key
 * @return the index of matching char, e.g. if we have "hello" in our trie and search for "hell", then 4 will be returned
 */
static size_t _trie_binary_search(trie_t *self, trie_t **child, struct trie_ptr **ptr, const unsigned char *key)
{
	size_t i     = 0;
	int    found = 1; /* should we continue to search ? */
	int    first, middle, last;
	*ptr = 0;
	while (found && key[i]) {
		first = 0;
		last  = self->n_children - 1;
		found = 0;
		while (first <= last) {
			struct trie_ptr *p;
			middle = (first + last) / 2;
			p      = &get_child(self)[middle];
			if (p->c < key[i])
				first = middle + 1;
			else if (p->c == key[i]) {
				self  = p->trie; /* self is the current searching trie */
				*ptr  = p;
				found = 1;
				i++;
				break; /* continue search for next char in the key */
			} else
				last = middle - 1;
		}
	}
	*child = self;
	return i;
}

/**
 * Finds for the data associated with KEY.
 * @return the previously inserted data
 */
void *trie_search(const trie_t *self, const char *key)
{
	trie_t          *child;
	struct trie_ptr *parent;
	unsigned char   *u_key = (unsigned char *) key;
	size_t           depth = _trie_binary_search((trie_t *) self, &child, &parent, u_key);
	return !key[depth] ? child->data : NULL;
}

static trie_t *__trie_grow(trie_t *self)
{
	trie_t *resized;
	size_t  children_size;
	int     size = self->size * 2; /* int for fear of exceeding short's bound */
	if (size > 255)
		size = 255;
	children_size = sizeof(struct trie_ptr) * size;
	resized       = OPUS_REALLOC(self, sizeof(*self) + children_size);
	if (!resized)
		return NULL;
	resized->size = (short) size;
	return resized;
}

static int __trie_ptr_cmp(const void *a, const void *b)
{
	return ((struct trie_ptr *) a)->c - ((struct trie_ptr *) b)->c;
}

static trie_t *__trie_node_add(trie_t *self, int c, trie_t *child)
{
	int i;
	if (self->size == self->n_children) {
		self = __trie_grow(self);
		if (!self)
			return NULL;
	}
	i                       = self->n_children++;
	get_child(self)[i].c    = c;
	get_child(self)[i].trie = child;
	qsort(get_child(self), self->n_children, sizeof(get_child(self)[0]), __trie_ptr_cmp);
	return self;
}

static void __trie_node_remove(trie_t *self, int i)
{
	size_t len = (--self->n_children - i) * sizeof(get_child(self)[0]);
	memmove(get_child(self) + i, get_child(self) + i + 1, len);
}

static trie_t *__trie_create(void)
{
	int     size = 1;
	trie_t *trie = OPUS_MALLOC(sizeof(*trie) + sizeof(struct trie_ptr) * size);
	if (!trie)
		return 0;
	trie->size       = (short) size;
	trie->n_children = 0;
	trie->data       = NULL;
	return trie;
}

static void *__trie_identity(const char *key, void *data, void *arg)
{
	(void) key;
	(void) data;
	return arg;
}

/**
 * @brief replace data associated with KEY using a replacer function.\n
 * The replacer function gets the key, the original data (NULL if none) and ARG.\n
 * Its return value is inserted into the trie.
 * @return 0 on success
 */
int trie_replace(trie_t *self, const char *key, trie_replacer f, void *arg)
{
	trie_t          *last;
	struct trie_ptr *parent;
	unsigned char   *ukey  = (unsigned char *) key;
	size_t           depth = _trie_binary_search(self, &last, &parent, ukey);
	while (ukey[depth]) {
		trie_t *sub_trie = __trie_create();
		trie_t *added;
		if (!sub_trie)
			return 1;
		added = __trie_node_add(last, ukey[depth], sub_trie);
		if (!added) {
			OPUS_FREE(sub_trie);
			return 1;
		}
		if (parent) {
			parent->trie = added;
			parent       = 0;
		}
		last = sub_trie;
		depth++;
	}
	last->data = f(key, last->data, arg);
	return ETRIEOK;
}

/**
 * @brief insert or replace DATA associated with KEY. \n
 * inserting NULL is equivalent of unassociating that key, \n
 * though no memory will be released.
 * @return 0 on success
 */
int trie_insert(trie_t *trie, const char *key, void *data)
{
	return trie_replace(trie, key, __trie_identity, data);
}

struct __trie_buffer {
	char  *buffer;
	size_t size, fill;
};

static int __trie_buffer_init(struct __trie_buffer *b, const char *prefix)
{
	b->fill   = strlen(prefix);
	b->size   = b->fill >= 256 ? b->fill * 2 : 256;
	b->buffer = OPUS_MALLOC(b->size);
	if (b->buffer)
		memcpy(b->buffer, prefix, b->fill + 1);
	return !b->buffer ? ETRIEFAIL : ETRIEOK;
}

static void __trie_buffer_free(struct __trie_buffer *b)
{
	OPUS_FREE(b->buffer);
	b->buffer = NULL;
}

static int __trie_buffer_grow(struct __trie_buffer *b)
{
	char *resize = OPUS_REALLOC(b->buffer, b->size * 2);
	if (!resize) {
		__trie_buffer_free(b);
		return -1;
	}
	b->buffer = resize;
	b->size *= 2;
	return ETRIEOK;
}

static int __trie_buffer_push(struct __trie_buffer *b, char c)
{
	if (b->fill + 1 == b->size)
		if (__trie_buffer_grow(b) != 0)
			return -1;
	b->buffer[b->fill++] = c;
	b->buffer[b->fill]   = 0;
	return ETRIEOK;
}

static void __trie_buffer_pop(struct __trie_buffer *b)
{
	if (b->fill > 0)
		b->buffer[--b->fill] = 0;
}

static int __trie_visit(trie_t *self, const char *prefix, trie_visitor visitor, void *arg)
{
	struct __trie_buffer buffer, *b = &buffer;
	struct trie_stack    stack, *s  = &stack;
	if (__trie_buffer_init(b, prefix) != 0)
		return -1;
	if (trie_stack_init(s) != 0) {
		__trie_buffer_free(b);
		return -1;
	}
	trie_stack_push(s, self);
	while (s->fill > 0) {
		struct trie_stack_node *node = trie_stack_peek(s);
		if (node->i == 0 && node->trie->data) {
			if (visitor(b->buffer, node->trie->data, arg) != 0) {
				__trie_buffer_free(b);
				trie_stack_free(s);
				return 1;
			}
		}
		if (node->i < node->trie->n_children) {
			trie_t *trie = get_child(node->trie)[node->i].trie;
			int     c    = get_child(node->trie)[node->i].c;
			node->i++;
			if (trie_stack_push(s, trie) != 0) {
				__trie_buffer_free(b);
				return -1;
			}
			if (__trie_buffer_push(b, c) != 0) {
				trie_stack_free(s);
				return -1;
			}
		} else {
			__trie_buffer_pop(b);
			trie_stack_pop(s);
		}
	}
	__trie_buffer_free(b);
	trie_stack_free(s);
	return 0;
}

/**
 * @brief visit in lexicographical order each key that matches the prefix. \n
 * an \b empty \b prefix visits every key in the trie. \n
 * the visitor must accept three arguments: the key, the data, and ARG.\n
 * iteration is aborted (with success) if visitor returns non-zero.
 * @param self
 * @param prefix
 * @param v
 * @param arg
 * @return 0 on success
 */
int trie_visit(trie_t *self, const char *prefix, trie_visitor v, void *arg)
{
	trie_t          *start = self;
	struct trie_ptr *ptr;
	unsigned char   *uprefix = (unsigned char *) prefix;
	int              depth   = _trie_binary_search(self, &start, &ptr, uprefix);
	int              r;
	if (prefix[depth])
		return 0;
	r = __trie_visit(start, prefix, v, arg);
	return r >= 0 ? 0 : -1;
}

static int __trie_visitor_counter(const char *key, void *data, void *arg)
{
	size_t *count = arg;
	count[0]++;
	return 0;
}

/**
 * @brief count the number of entries with a given prefix. \n
 * an empty prefix counts the entire trie.
 * @param trie
 * @param prefix
 * @return the number of entries matching PREFIX
 */
size_t trie_count(trie_t *trie, const char *prefix)
{
	size_t count = 0;
	trie_visit(trie, prefix, __trie_visitor_counter, &count);
	return count;
}

/**
 * @brief remove all unused branches in a trie
 * @param trie
 * @return 0 on success
 */
int trie_prune(trie_t *trie)
{
	struct trie_stack stack, *s = &stack;
	if (trie_stack_init(s) != 0)
		return -1;
	trie_stack_push(s, trie);
	while (s->fill > 0) {
		struct trie_stack_node *node = trie_stack_peek(s);
		int                     i    = node->i++;
		if (i < node->trie->n_children) {
			if (trie_stack_push(s, get_child(node->trie)[i].trie) != 0)
				return 0;
		} else {
			int     i;
			trie_t *t = trie_stack_pop(s);
			for (i = 0; i < t->n_children; i++) {
				trie_t *child = get_child(t)[i].trie;
				if (!child->n_children && !child->data) {
					__trie_node_remove(t, i--);
					OPUS_FREE(child);
				}
			}
		}
	}
	trie_stack_free(s);
	return 1;
}

/**
 * @brief compute the total memory usage of a trie.
 * @param trie
 * @return the size in bytes, or 0 on error
 */
size_t trie_size(trie_t *trie)
{
	size_t            size = 0;
	struct trie_stack stack, *s = &stack;
	if (trie_stack_init(s) != 0)
		return 0;
	trie_stack_push(s, trie);
	while (s->fill > 0) {
		struct trie_stack_node *node = trie_stack_peek(s);
		int                     i    = node->i++;
		if (i < node->trie->n_children) {
			if (trie_stack_push(s, get_child(node->trie)[i].trie) != 0)
				return 0;
		} else {
			trie_t *t = trie_stack_pop(s);
			size += sizeof(*t) + sizeof(*get_child(t)) * t->size;
		}
	}
	trie_stack_free(s);
	return size;
}

struct trie_it {
	struct trie_stack    stack;
	struct __trie_buffer buffer;
	void                *data;
	int                  error;
};

/**
 * @brief create an iterator that visits each key with the given prefix, in lexicographical order.\n
 * making any modifications to the trie invalidates the iterator.
 * @example
 * 		struct trie_it *it = trie_it_create(trie, "");
 * 		for (; !trie_it_done(it); trie_it_next(it)) {
 * 			const char *key = trie_it_key(it);
 * 			const void *data = trie_it_data(it);
 * 		}
 * 		if (trie_it_error(it)) abort();
 * 		trie_it_destroy(it);
 * @param trie
 * @param prefix
 * @return a fresh iterator pointing to the first key
 */
trie_it_t *trie_it_create(trie_t *trie, const char *prefix)
{
	trie_it_t *it = OPUS_MALLOC(sizeof(*it));
	if (!it)
		return 0;
	if (trie_stack_init(&it->stack)) {
		OPUS_FREE(it);
		return 0;
	}
	if (__trie_buffer_init(&it->buffer, prefix)) {
		trie_stack_free(&it->stack);
		OPUS_FREE(it);
		return 0;
	}
	trie_stack_push(&it->stack, trie); /* first push always successful */
	it->data  = 0;
	it->error = 0;
	trie_it_next(it);
	return it;
}

/**
 * @brief advance iterator to the next key in the sequence
 * @param it
 * @return 0 if done, else 1
 */
int trie_it_next(trie_it_t *it)
{
	while (!it->error && it->stack.fill) {
		struct trie_stack_node *node = trie_stack_peek(&it->stack);

		if (node->i == 0 && node->trie->data) {
			if (!it->data) {
				it->data = node->trie->data;
				return 1;
			} else {
				it->data = 0;
			}
		}

		if (node->i < node->trie->n_children) {
			trie_t *trie = get_child(node->trie)[node->i].trie;
			int     c    = get_child(node->trie)[node->i].c;
			node->i++;
			if (trie_stack_push(&it->stack, trie)) {
				it->error = 1;
				return 0;
			}
			if (__trie_buffer_push(&it->buffer, c)) {
				it->error = 1;
				return 0;
			}
		} else {
			__trie_buffer_pop(&it->buffer);
			trie_stack_pop(&it->stack);
		}
	}
	return 0;
}

/**
 * @brief returned __trie_buffer is invalidated on the next call of \b trie_it_next() \n
 * @param it
 * @return a __trie_buffer containing the current key
 */
const char *trie_it_key(trie_it_t *it)
{
	return it->buffer.buffer;
}

/**
 * @param it
 * @return the data pointer for the current key
 */
void *trie_it_data(trie_it_t *it)
{
	return it->data;
}

/**
 * @brief check if the iterator has completed the iteration
 * @param it
 * @return 1 if the iterator has completed (including errors)
 */
int trie_it_done(trie_it_t *it)
{
	return it->error || !it->stack.fill;
}

/**
 * @brief check if error appeared in iteration
 * @param it
 * @return 1 if error
 */
int trie_it_error(trie_it_t *it)
{
	return it->error;
}

/**
 * @brief destroys the iterator
 * @param it
 * @return 0 on success
 */
void trie_it_destroy(trie_it_t *it)
{
	__trie_buffer_free(&it->buffer);
	trie_stack_free(&it->stack);
	OPUS_FREE(it);
}


#ifdef TRIE_TEST_MAIN
//#if 1
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data_structure/trie.h"
#include "utils/core_utils.h"

#define EXP_TOTAL 5
#define EXP_WORDS 13

static void die(const char *s)
{
	fprintf(stderr, "%s\n", s);
	exit(EXIT_FAILURE);
}

static uint32_t pcg32(uint64_t s[1])
{
	uint64_t h = s[0];
	s[0]       = h * UINT64_C(0x5851f42d4c957f2d) + UINT64_C(0xd737232eeccdf7ed);
	uint32_t x = ((h >> 18) ^ h) >> 27;
	unsigned r = h >> 59;
	return (x >> r) | (x << (-r & 31u));
}

static char *generate(uint64_t *s)
{
	int   min = 8;
	int   max = 300;
	int   len = min + pcg32(s) % (max - min);
	char *key = OPUS_MALLOC(len + 1);
	if (!key)
		die("out of memory");
	for (int i = 0; i < len; i++)
		key[i] = 1 + pcg32(s) % 255;
	key[len] = 0;
	return key;
}

static void *replacer(const char *key, void *value, void *arg)
{
	(void) key;
	OPUS_FREE(value);
	return arg;
}

static void hexprint(const char *label, const char *s)
{
	unsigned char *p = (unsigned char *) s;
	fputs(label, stdout);
	for (size_t i = 0; *p; i++)
		printf("%c%02x", i % 16 ? ' ' : '\n', *p++);
	putchar('\n');
}

static int check_order(const char *key, void *value, void *arg)
{
	char **prev = arg;
	if (*prev && strcmp(*prev, key) >= 0) {
		hexprint("first:", *prev);
		hexprint("second:", key);
		die("FAIL: keys not ordered");
	}
	*prev = value;
	return 0;
}

int main(void)
{
	uint64_t     rng = 0xabf4206f849fdf21;
	struct trie *t   = trie_create();

	for (int i = 0; i < 1 << EXP_TOTAL; i++) {
		long     count   = (1L << EXP_WORDS) + pcg32(&rng) % (1L << EXP_WORDS);
		uint64_t rngsave = rng;

		UTILS_START_BENCHMARK(insert_key);
		for (long j = 0; j < count; j++) {
			char *key = generate(&rng);
			if (trie_insert(t, key, key))
				die("out of memory");
		}
		UTILS_END_BENCHMARK(insert_key);

		/* Check that all keys are present. */
		UTILS_START_BENCHMARK(check_key_is_present);
		uint64_t rngcopy = rngsave;
		for (long j = 0; j < count; j++) {
			char *key = generate(&rngcopy);
			char *r   = trie_search(t, key);
			if (!r)
				die("FAIL: missing key");
			if (strcmp(r, key))
				die("FAIL: value mismatch");
			OPUS_FREE(key);
		}
		UTILS_END_BENCHMARK(check_key_is_present);

		/* Check that keys are sorted (visitor) */
		UTILS_START_BENCHMARK(check_key_sorted_visitor);
		char *prev = 0;
		if (trie_visit(t, "", check_order, &prev))
			die("out of memory");
		UTILS_END_BENCHMARK(check_key_sorted_visitor);

		/* Check that keys are sorted (iterator) */
		UTILS_START_BENCHMARK(check_key_sorted_iterator);
		prev               = 0;
		struct trie_it *it = trie_it_create(t, "");
		for (; !trie_it_done(it); trie_it_next(it)) {
			const char *key = trie_it_key(it);
			if (prev && strcmp(prev, key) >= 0)
				die("FAIL: keys not ordered");
			prev = trie_it_data(it);
		}
		if (trie_it_error(it))
			die("out of memory");
		trie_it_destroy(it);
		UTILS_END_BENCHMARK(check_key_sorted_iterator);

		/* Remove all entries. */
		UTILS_START_BENCHMARK(remove_all_entries);
		rngcopy = rngsave;
		for (long j = 0; j < count; j++) {
			char *key = generate(&rngcopy);
			if (trie_replace(t, key, replacer, NULL))
				die("out of memory");
			OPUS_FREE(key);
		}
		UTILS_END_BENCHMARK(remove_all_entries);

		/* Check that all keys are gone. */
		UTILS_START_BENCHMARK(check_key_gone);
		rngcopy = rngsave;
		for (long j = 0; j < count; j++) {
			char *key = generate(&rngcopy);
			char *r   = trie_search(t, key);
			if (r)
				die("FAIL: key not removed");
			OPUS_FREE(key);
		}
		UTILS_END_BENCHMARK(check_key_gone);

		/* Print out current trie size (as progress) */
		double mb = trie_size(t) / 1024.0 / 1024.0;
		printf("%-2d trie_size() = %10.3f MiB\n", i, mb);

		/* Prune trie every quarter. */
		if (i && i % (1 << (EXP_TOTAL - 2)) == 0) {
			/* Insert a check key to make sure it survives. */
			char               tmpkey[32];
			unsigned long long v = rngsave;
			snprintf(tmpkey, sizeof(tmpkey), "%llx", v);
			if (trie_insert(t, tmpkey, tmpkey))
				die("out of memory");
			trie_prune(t);
			if (trie_search(t, tmpkey) != tmpkey)
				die("FAIL: trie_prune() removed live key");
			trie_insert(t, tmpkey, 0); /* Cleanup */
		}
	}

	trie_destroy(t);
}
#endif /* TRIE_TEST_MAIN */
