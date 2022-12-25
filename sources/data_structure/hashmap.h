#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdint.h>
#include <stdio.h>
#include "utils/utils.h"

#define HASHMAP_GROW_AT_FACTOR (0.75)
#define HASHMAP_SHRINK_AT_FACTOR (0.10)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * ERROR CODE
 */
enum {
	EHASHMAPOK     = 0, /* no problem occurred in recent hashmap operation */
	EHASHMAPRESIZE = 1, /* illegal hashmap resize count or resize operation failed due to other problems */
	EHASHMAPMEM    = 2  /* no sufficient memory for hashmap to allocate */
};

typedef struct hashmap hashmap_t;
typedef int (*hashmap_compare_cb)(hashmap_t *map, const void *a, const void *b, void *user_data);
typedef uint64_t (*hashmap_hash_cb)(hashmap_t *map, const void *ele, uint64_t seed0, uint64_t seed1, void *user_data);
typedef void (*hashmap_ele_free_cb)(hashmap_t *map, const void *ele, void *user_data);
typedef void (*hashmap_print_data_cb)(const void *ele, char *text, uint64_t n);

/**
 * <h2>ROBIN HOOD HASHING</h2>
 * <p>
 * Robin Hood hashing is a technique for implementing hash tables.
 * It is based on <b>open addressing</b> with a simple but clever twist:
 * 		<blockquote>
 * 		As new keys are inserted, old keys are shifted around in a way such that
 * 		all keys stay reasonably close to the slot they originally hash to.
 * 		In particular, the variance of the keys distances from their "home" slots is minimized.
 * 		</blockquote>
 * </p>
 * <p>
 * Key properties include:
 * <blockquote><ul>
 *		<li>Lookup is O(1) amortized and O(ln n) in worst case</li>
 *		<li>Fast also when looking up keys that does not exist</li>
 *		<li>Minimal variance on distances to "home" slots</li>
 *		<li>Cache friendly and memory efficient (no linked lists or additional pointers)</li>
 *		<li>Performs well under load factors as high as ~0.9</li>
 * </blockquote></ul>
 * </p>
 *
 * for more information about the implementation, please refer to <a>https://programming.guide/robin-hood-hashing.html</a>
 */
struct hashmap {
	uint64_t ele_size;         /* size of elements stored in this map */
	uint64_t bucket_size;      /* size of bucket (ele_size + sizeof(struct _bucket)) */
	uint64_t buckets_capacity; /* size of slots allocated for buckets */
	uint64_t buckets_used;     /* size of elements stored in this map */

	uint64_t   grow_at_;     /* if the count of buckets used in the hashmap reaches the count, hashmap will grow its memory usage */
	uint64_t   shrink_at_;   /* if the count of buckets used in the hashmap reaches the count, hashmap will shrink its memory usage */
	uint64_t   mask_;        /* mask for hashmap_bucket_idx, for internal usage, will always be buckets_capacity - 1 */
	uint64_t seed0, seed1; /* seed number for hash_cb to generate hash value */
	void    *user_data;    /* hashmap do not allocate memory for this */
	void    *ele_data_;    /* (has its own memory allocated) */
	void    *temp_data_;   /* for swapping(has its own memory allocated) */
	void    *buckets_;     /* stores data for buckets */

	hashmap_compare_cb  compare_cb_;
	hashmap_hash_cb     hash_cb_;
	hashmap_ele_free_cb ele_free_cb_;
};

hashmap_t *hashmap_init(hashmap_t *map, uint64_t ele_size, uint64_t capacity, uint64_t seed0, uint64_t seed1,
                        hashmap_compare_cb compare, hashmap_hash_cb hash, hashmap_ele_free_cb ele_free);
hashmap_t *hashmap_create(uint64_t ele_size, uint64_t capacity, uint64_t seed0, uint64_t seed1,
                          hashmap_compare_cb compare, hashmap_hash_cb hash, hashmap_ele_free_cb ele_free);
void       hashmap_destroy(hashmap_t *map);
void      *hashmap_insert(hashmap_t *map, void *ele);
void      *hashmap_retrieve(hashmap_t *map, void *ele);
void       hashmap_clear(hashmap_t *map);
void      *hashmap_probe(hashmap_t *hashmap, uint64_t index);
void       hashmap_get_bucket_info(hashmap_t *hashmap, uint64_t index, unsigned int *psl, void **ele);

void     hashmap_dump(hashmap_t *map, FILE *fp, hashmap_print_data_cb print_data);
uint64_t hashmap_simple_hash(hashmap_t *map, void *key, int count);
uint64_t hashmap_murmur(const void *data, uint64_t count, uint64_t seed0, uint64_t seed1);

#ifdef __cplusplus
};
#endif /* __cplusplus */
#endif /* HASHMAP_H */

