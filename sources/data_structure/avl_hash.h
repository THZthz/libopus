#ifndef __AVLHASH_H__
#define __AVLHASH_H__

#include "data_structure/avl.h"

#ifndef __ILIST_DEF__
#define __ILIST_DEF__

struct ILISTHEAD {
	struct ILISTHEAD *next, *prev;
};

typedef struct ILISTHEAD ilist_head;


#define ILIST_HEAD_INIT(name) \
	{                         \
		&(name), &(name)      \
	}
#define ILIST_HEAD(name) \
	struct ILISTHEAD name = ILIST_HEAD_INIT(name)

#define ILIST_INIT(ptr) ( \
	    (ptr)->next = (ptr), (ptr)->prev = (ptr))

#define IOFFSETOF(TYPE, MEMBER) ((size_t) & ((TYPE *) 0)->MEMBER)

#define ICONTAINEROF(ptr, type, member) ( \
	    (type *) (((char *) ((type *) ptr)) - IOFFSETOF(type, member)))

#define ILIST_ENTRY(ptr, type, member) ICONTAINEROF(ptr, type, member)

#define ILIST_ADD(node, head) (                             \
	    (node)->prev = (head), (node)->next = (head)->next, \
	    (head)->next->prev = (node), (head)->next = (node))

#define ILIST_ADD_TAIL(node, head) (                        \
	    (node)->prev = (head)->prev, (node)->next = (head), \
	    (head)->prev->next = (node), (head)->prev = (node))

#define ILIST_DEL_BETWEEN(p, n) ((n)->prev = (p), (p)->next = (n))

#define ILIST_DEL(entry) (                   \
	    (entry)->next->prev = (entry)->prev, \
	    (entry)->prev->next = (entry)->next, \
	    (entry)->next = 0, (entry)->prev = 0)

#define ILIST_DEL_INIT(entry) \
	do {                      \
		ILIST_DEL(entry);     \
		ILIST_INIT(entry);    \
	} while (0)

#define ILIST_IS_EMPTY(entry) ((entry) == (entry)->next)

#define ilist_init ILIST_INIT
#define ilist_entry ILIST_ENTRY
#define ilist_add ILIST_ADD
#define ilist_add_tail ILIST_ADD_TAIL
#define ilist_del ILIST_DEL
#define ilist_del_init ILIST_DEL_INIT
#define ilist_is_empty ILIST_IS_EMPTY

#define ILIST_FOREACH(iterator, head, TYPE, MEMBER)            \
	for ((iterator) = ilist_entry((head)->next, TYPE, MEMBER); \
	     &((iterator)->MEMBER) != (head);                      \
	     (iterator) = ilist_entry((iterator)->MEMBER.next, TYPE, MEMBER))

#define ilist_foreach(iterator, head, TYPE, MEMBER) \
	ILIST_FOREACH(iterator, head, TYPE, MEMBER)

#define ilist_foreach_entry(pos, head) \
	for ((pos) = (head)->next; (pos) != (head); (pos) = (pos)->next)


#define __ilist_splice(list, head)                              \
	do {                                                        \
		ilist_head *first = (list)->next, *last = (list)->prev; \
		ilist_head *at = (head)->next;                          \
		(first)->prev = (head), (head)->next = (first);         \
		(last)->next = (at), (at)->prev = (last);               \
	} while (0)

#define ilist_splice(list, head)                               \
	do {                                                       \
		if (!ilist_is_empty(list)) __ilist_splice(list, head); \
	} while (0)

#define ilist_splice_init(list, head) \
	do {                              \
		ilist_splice(list, head);     \
		ilist_init(list);             \
	} while (0)

#define ilist_replace(oldnode, newnode) (        \
	    (newnode)->next       = (oldnode)->next, \
	    (newnode)->next->prev = (newnode),       \
	    (newnode)->prev       = (oldnode)->prev, \
	    (newnode)->prev->next = (newnode))

#ifdef _MSC_VER
#pragma warning(disable : 4311)
#pragma warning(disable : 4312)
#pragma warning(disable : 4996)
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif


/**
 * avl_hash_node_t: embedded in your structure
 */
struct avl_hash_node {
	avltree_node_t avlnode; /*  avltree node */

	void  *key; /*  generic type pointer */
	size_t hash;
};
typedef struct avl_hash_node avl_hash_node_t;

/**
 * hash index (or slot/bucket)
 */
struct avl_hash_index {
	struct ILISTHEAD    node;    /*  linked to the list for non-empty indexes */
	struct avltree_root avlroot; /*  avl root */
};

#define avl_hash_INIT_SIZE 8


/**
 * static hash table: zero memory allocation
 */
struct avl_hash_table {
	size_t count;      /*  node count */
	size_t index_size; /*  must be the power of 2 */
	size_t index_mask; /*  must be (index_size - 1); */
	size_t (*hash)(const void *key);
	int (*compare)(const void *key1, const void *key2);
	struct ILISTHEAD       head;
	struct avl_hash_index *index;
	struct avl_hash_index  init[avl_hash_INIT_SIZE];
};
typedef struct avl_hash_table avl_hash_table_t;

void avl_hash_init(avl_hash_table_t *ht,
                   size_t (*hash)(const void *key),
                   int (*compare)(const void *key1, const void *key2));

/*
 * node traverse
 */

avl_hash_node_t *avl_hash_node_first(avl_hash_table_t *ht);
avl_hash_node_t *avl_hash_node_last(avl_hash_table_t *ht);
avl_hash_node_t *avl_hash_node_next(avl_hash_table_t *ht, avl_hash_node_t *node);
avl_hash_node_t *avl_hash_node_prev(avl_hash_table_t *ht, avl_hash_node_t *node);


/**
 * setup key
 * @param ht
 * @param node
 * @param key
 */
static INLINE void avl_hash_node_key(avl_hash_table_t *ht, avl_hash_node_t *node, void *key)
{
	node->key  = key;
	node->hash = ht->hash(key);
}


avl_hash_node_t *avl_hash_find(avl_hash_table_t *ht, const avl_hash_node_t *node);

avltree_node_t **avl_hash_track(avl_hash_table_t *ht, const avl_hash_node_t *node, avltree_node_t **parent);

avl_hash_node_t *avl_hash_add(avl_hash_table_t *ht, avl_hash_node_t *node);

void avl_hash_erase(avl_hash_table_t *ht, avl_hash_node_t *node);

void avl_hash_replace(avl_hash_table_t *ht, avl_hash_node_t *victim, avl_hash_node_t *newnode);

void avl_hash_clear(avl_hash_table_t *ht, void (*destroy)(avl_hash_node_t *node));


/**
 * swap index memory: used for rehash
 * @param ht
 * @param index
 * @param nbytes re-index nbytes must be: sizeof(struct avl_hash_index) * n, n must be the power of 2
 * @return
 */
void *avl_hash_swap(avl_hash_table_t *ht, void *index, size_t nbytes);


/**
 * fastbin - fixed size object allocator
 */
struct avl_fastbin {
	size_t obj_size;
	size_t page_size;
	size_t maximum;
	char  *start;
	char  *endup;
	void  *next;
	void  *pages;
};


#define AVL_NEXT(ptr) (((void **) (ptr))[0])

void avl_fastbin_init(struct avl_fastbin *fb, size_t obj_size);
void avl_fastbin_destroy(struct avl_fastbin *fb);

void *avl_fastbin_new(struct avl_fastbin *fb);
void  avl_fastbin_del(struct avl_fastbin *fb, void *ptr);


/**
 * hash map (wrapper of hash table)
 */
struct avl_hash_entry {
	avl_hash_node_t node;
	void           *value;
};
typedef struct avl_hash_entry avl_hash_entry_t;

struct avl_hash_map {
	size_t count;
	int    insert;
	int    fixed;
	int    builtin;
	void *(*key_copy)(void *key);
	void (*key_destroy)(void *key);
	void *(*value_copy)(void *value);
	void (*value_destroy)(void *value);
	struct avl_fastbin fb;
	avl_hash_table_t   ht;
};
typedef struct avl_hash_map avl_hash_map_t;


#define avl_hash_key(entry) ((entry)->node.key)
#define avl_hash_value(entry) ((entry)->value)

#define avl_map_get(TYPE, hm, key) ((TYPE) avl_map_get_v((hm), (key)))

void avl_map_init(avl_hash_map_t *hm, size_t (*hash)(const void *),
                  int (*compare)(const void *, const void *));
void avl_map_done(avl_hash_map_t *hm);

avl_hash_entry_t *avl_map_first(avl_hash_map_t *hm);
avl_hash_entry_t *avl_map_last(avl_hash_map_t *hm);
avl_hash_entry_t *avl_map_next(avl_hash_map_t *hm, avl_hash_entry_t *n);
avl_hash_entry_t *avl_map_prev(avl_hash_map_t *hm, avl_hash_entry_t *n);

avl_hash_entry_t *avl_map_find(avl_hash_map_t *hm, const void *key);
void             *avl_map_lookup(avl_hash_map_t *hm, const void *key, void *defval);
avl_hash_entry_t *avl_map_add(avl_hash_map_t *hm, void *key, void *value, int *success);
avl_hash_entry_t *avl_map_set(avl_hash_map_t *hm, void *key, void *value);
void             *avl_map_get_v(avl_hash_map_t *hm, const void *key);
void              avl_map_erase(avl_hash_map_t *hm, avl_hash_entry_t *entry);


/**
 *
 * @param hm
 * @param key
 * @return 0 for success, -1 for key mismatch
 */
int avl_map_remove(avl_hash_map_t *hm, const void *key);

void avl_map_clear(avl_hash_map_t *hm);


/**
 * fast inline search template
 */
#define avl_map_search(hm, srckey, hash_func, cmp_func, result)                    \
	do {                                                                           \
		size_t                 __hash = (hash_func) (srckey);                      \
		struct avl_hash_index *__index =                                           \
		        &((hm)->ht.index[__hash & ((hm)->ht.index_mask)]);                 \
		avltree_node_t *__anode = __index->avlroot.node;                           \
		(result)                = NULL;                                            \
		while (__anode) {                                                          \
			avl_hash_node *__snode = AVL_ENTRY(__anode, avl_hash_node_t, avlnode); \
			size_t         __shash = __snode->hash;                                \
			if (__hash == __shash) {                                               \
				int __hc = (cmp_func) ((srckey), __snode->key);                    \
				if (__hc == 0) {                                                   \
					(result) = AVL_ENTRY(__snode, avl_hash_entry_t, node);         \
					break;                                                         \
				}                                                                  \
				__anode = (__hc < 0) ? __anode->left : __anode->right;             \
			} else {                                                               \
				__anode = (__hash < __shash) ? __anode->left : __anode->right;     \
			}                                                                      \
		}                                                                          \
	} while (0)


#ifdef __cplusplus
}
#endif


#endif