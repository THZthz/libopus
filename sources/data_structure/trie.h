/**
 * @file trie.h
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/3/16
 *
 * @brief This trie associates an arbitrary void pointer with a NULL-terminated
 *		C string key. All lookups are O(n), n being the length of the string.
 * 	Strings are stored in sorted order, so visitor functions __trie_visit keys
 *  	in lexicographical order. The visitor can also be used to __trie_visit keys
 * 	by a string prefix. An empty prefix "" matches all keys (the prefix
 * 	argument should never be NULL).
 *
 * 	Except for trie_destroy() and trie_prune(), memory is never freed by the
 * 	trie, even when entries are "removed" by associating a NULL pointer.
 *
 * @see http://en.wikipedia.org/wiki/Trie
 *
 * @example
 *
 * @development_log
 * 	3/21 feel unnecessary, abort
 * 	3/31 rewrite code and add tests :D
 *
 *
 */
#ifndef TRIE_H
#define TRIE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stddef.h>

enum {
	ETRIEOK = 0,
	ETRIEFAIL = -1
};

typedef struct trie    trie_t;
typedef struct trie_it trie_it_t;

struct trie;
struct trie_it;

struct trie_ptr {
	trie_t *trie;
	int     c;
};

struct trie {
	void *data;
	short n_children, size;
};

typedef int (*trie_visitor)(const char *key, void *data, void *arg);
typedef void *(*trie_replacer)(const char *key, void *current, void *arg);


trie_t *trie_create(void);
int     trie_destroy(trie_t *trie);

void *trie_search(const trie_t *, const char *key);
int   trie_insert(trie_t *, const char *key, void *data);
int   trie_replace(trie_t *, const char *key, trie_replacer f, void *arg);

int    trie_visit(trie_t *, const char *prefix, trie_visitor v, void *arg);
int    trie_prune(trie_t *);
size_t trie_count(trie_t *, const char *prefix);
size_t trie_size(trie_t *);

/*-----------------
    Iterator
 -----------------*/

trie_it_t * trie_it_create(trie_t *, const char *prefix);
int         trie_it_next(trie_it_t *);
const char *trie_it_key(trie_it_t *);
void *      trie_it_data(trie_it_t *);
int         trie_it_done(trie_it_t *);
int         trie_it_error(trie_it_t *);
void        trie_it_destroy(trie_it_t *);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* TRIE_H */
