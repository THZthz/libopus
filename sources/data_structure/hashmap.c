#include "data_structure/hashmap.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

typedef struct bucket__ {
	uint64_t hash;
	unsigned psl;
} bucket__;

/**
 * @brief copied from https://github.com/rxi/map/tree/master/src
 * @param map
 * @param key
 * @param count
 * @return
 */
uint64_t opus_hashmap_simple_hash(opus_hashmap *map, void *key, int count)
{
	char    *s    = key;
	unsigned hash = 5381;
	while (count-- > 0)
		hash = ((hash << 5) + hash) ^ *s++;
	return hash;
}

OPUS_INLINE void mm86128_(const void *key, const int count, uint32_t seed, void *out)
{
#define ROTL32(x, r) (((x) << (r)) | ((x) >> (32 - (r))))
#define FMIX32(h)      \
	(h) ^= (h) >> 16;  \
	(h) *= 0x85ebca6b; \
	(h) ^= (h) >> 13;  \
	(h) *= 0xc2b2ae35; \
	(h) ^= (h) >> 16;
	const uint8_t *data     = (const uint8_t *) key;
	const int      n_blocks = count / 16;

	uint32_t h1 = seed;
	uint32_t h2 = seed;
	uint32_t h3 = seed;
	uint32_t h4 = seed;
	uint32_t c1 = 0x239b961b;
	uint32_t c2 = 0xab0e9789;
	uint32_t c3 = 0x38b34ae5;
	uint32_t c4 = 0xa1e38b93;

	const uint32_t *blocks = (const uint32_t *) (data + n_blocks * 16);
	const uint8_t  *tail;

	uint32_t k1, k2, k3, k4;
	int      i;

	for (i = -n_blocks; i; i++) {
		k1 = blocks[i * 4 + 0];
		k2 = blocks[i * 4 + 1];
		k3 = blocks[i * 4 + 2];
		k4 = blocks[i * 4 + 3];
		k1 *= c1;
		k1 = ROTL32(k1, 15);
		k1 *= c2;
		h1 ^= k1;
		h1 = ROTL32(h1, 19);
		h1 += h2;
		h1 = h1 * 5 + 0x561ccd1b;
		k2 *= c2;
		k2 = ROTL32(k2, 16);
		k2 *= c3;
		h2 ^= k2;
		h2 = ROTL32(h2, 17);
		h2 += h3;
		h2 = h2 * 5 + 0x0bcaa747;
		k3 *= c3;
		k3 = ROTL32(k3, 17);
		k3 *= c4;
		h3 ^= k3;
		h3 = ROTL32(h3, 15);
		h3 += h4;
		h3 = h3 * 5 + 0x96cd1c35;
		k4 *= c4;
		k4 = ROTL32(k4, 18);
		k4 *= c1;
		h4 ^= k4;
		h4 = ROTL32(h4, 13);
		h4 += h1;
		h4 = h4 * 5 + 0x32ac3b17;
	}
	tail = (const uint8_t *) (data + n_blocks * 16);
	k1   = 0;
	k2   = 0;
	k3   = 0;
	k4   = 0;
	switch (count & 15) {
		case 15:
			k4 ^= tail[14] << 16;
		case 14:
			k4 ^= tail[13] << 8;
		case 13:
			k4 ^= tail[12] << 0;
			k4 *= c4;
			k4 = ROTL32(k4, 18);
			k4 *= c1;
			h4 ^= k4;
		case 12:
			k3 ^= tail[11] << 24;
		case 11:
			k3 ^= tail[10] << 16;
		case 10:
			k3 ^= tail[9] << 8;
		case 9:
			k3 ^= tail[8] << 0;
			k3 *= c3;
			k3 = ROTL32(k3, 17);
			k3 *= c4;
			h3 ^= k3;
		case 8:
			k2 ^= tail[7] << 24;
		case 7:
			k2 ^= tail[6] << 16;
		case 6:
			k2 ^= tail[5] << 8;
		case 5:
			k2 ^= tail[4] << 0;
			k2 *= c2;
			k2 = ROTL32(k2, 16);
			k2 *= c3;
			h2 ^= k2;
		case 4:
			k1 ^= tail[3] << 24;
		case 3:
			k1 ^= tail[2] << 16;
		case 2:
			k1 ^= tail[1] << 8;
		case 1:
			k1 ^= tail[0] << 0;
			k1 *= c1;
			k1 = ROTL32(k1, 15);
			k1 *= c2;
			h1 ^= k1;
	};
	h1 ^= count;
	h2 ^= count;
	h3 ^= count;
	h4 ^= count;
	h1 += h2;
	h1 += h3;
	h1 += h4;
	h2 += h1;
	h3 += h1;
	h4 += h1;
	FMIX32(h1);
	FMIX32(h2);
	FMIX32(h3);
	FMIX32(h4);
	h1 += h2;
	h1 += h3;
	h1 += h4;
	h2 += h1;
	h3 += h1;
	h4 += h1;
	((uint32_t *) out)[0] = h1;
	((uint32_t *) out)[1] = h2;
	((uint32_t *) out)[2] = h3;
	((uint32_t *) out)[3] = h4;
}

/**
 * @brief wrapper function, returns a hash value for `data` using Murmur3_86_128
 * @param data
 * @param count
 * @param seed0
 * @param seed1
 * @return
 */
uint64_t opus_hashmap_murmur(const void *data, uint64_t count, uint64_t seed0, uint64_t seed1)
{
	char out[16];
	mm86128_(data, (int) count, seed0, &out);
	return *(uint64_t *) out;
}

/**
 * @brief make sure capacity is always the multiple of 2
 * @param desired_capacity
 * @return
 */
OPUS_INLINE uint64_t get_proper_capacity_(uint64_t desired_capacity)
{
	uint64_t n_cap = 16;
	if (desired_capacity < n_cap) {
		desired_capacity = n_cap;
	} else {
		while (n_cap < desired_capacity)
			n_cap *= 2;
		desired_capacity = n_cap;
	}
	return desired_capacity;
}

/**
 * @brief
 * @param map
 * @param ele_size
 * @param capacity
 * @param seed0
 * @param seed1
 * @param compare
 * @param hash
 * @param ele_free
 * @return
 */
opus_hashmap *opus_hashmap_init(opus_hashmap *map, uint64_t ele_size, uint64_t capacity, uint64_t seed0, uint64_t seed1,
                                opus_hashmap_compare_cb compare, opus_hashmap_hash_cb hash, opus_hashmap_ele_free_cb ele_free)
{
	uint64_t bucket_size = sizeof(bucket__) + ele_size;

	while (bucket_size & (sizeof(bucket__) - 1)) {
		bucket_size++;
	}

	map->ele_size         = ele_size;
	map->bucket_size      = bucket_size;
	map->buckets_capacity = get_proper_capacity_(capacity);
	map->mask_            = map->buckets_capacity - 1;
	map->buckets_used     = 0;
	map->temp_data_       = OPUS_MALLOC(map->bucket_size * 2);
	if (!map->temp_data_) return NULL;
	map->ele_data_ = (char *) map->temp_data_ + map->bucket_size;
	map->buckets_  = OPUS_MALLOC(map->bucket_size * map->buckets_capacity);
	if (!map->buckets_) {
		OPUS_FREE(map->temp_data_);
		return NULL;
	}

	/* we must clear the bits of buckets because we need psl to be 0 at the beginning */
	memset(map->buckets_, 0, map->bucket_size * map->buckets_capacity);

	map->grow_at_   = (uint64_t) ((double) map->buckets_capacity * HASHMAP_GROW_AT_FACTOR);
	map->shrink_at_ = (uint64_t) ((double) map->buckets_capacity * HASHMAP_SHRINK_AT_FACTOR);

	map->seed0        = seed0;
	map->seed1        = seed1;
	map->compare_cb_  = compare;
	map->hash_cb_     = hash;
	map->ele_free_cb_ = ele_free;

	map->user_data = NULL;

	return map;
}

opus_hashmap *opus_hashmap_create(uint64_t ele_size, uint64_t capacity, uint64_t seed0, uint64_t seed1,
                                  opus_hashmap_compare_cb compare, opus_hashmap_hash_cb hash, opus_hashmap_ele_free_cb ele_free)
{
	opus_hashmap *map;
	map = (opus_hashmap *) OPUS_MALLOC(sizeof(opus_hashmap));
	if (!map) return NULL;
	return opus_hashmap_init(map, ele_size, capacity, seed0, seed1, compare, hash, ele_free);
}

OPUS_INLINE uint64_t bucket_idx_(opus_hashmap *map, uint64_t hash_value)
{
	/* If the implementation is changed to allow a non-power-of-2 bucket count, */
	/* the line below should be changed to use mod instead of AND */
	return hash_value & map->mask_;
}

OPUS_INLINE bucket__ *bucket_at_(opus_hashmap *map, void *buckets, uint64_t i)
{
	return (bucket__ *) ((char *) buckets + i * map->bucket_size);
}

OPUS_INLINE void *bucket_data_(bucket__ *bucket)
{
	return (void *) ((char *) bucket + sizeof(bucket__));
}

OPUS_INLINE uint64_t get_hash_(opus_hashmap *map, void *ptr_to_ele)
{
	/* clear the 16 bits of the left of the hashed value(for the sake of psl) */
	return map->hash_cb_(map, ptr_to_ele, map->seed0, map->seed1, map->user_data) << 16 >> 16;
}

OPUS_INLINE uint64_t next_probe_sequence_(opus_hashmap *map, uint64_t cur_idx, void *ele)
{
	/* simple linear probe */
	return bucket_idx_(map, cur_idx + 1);
}

void *insert_(opus_hashmap *map, void *buckets, bucket__ *bucket_to_insert)
{
	uint64_t  i;
	bucket__ *bucket;
	void     *pa, *pb;

	i = bucket_idx_(map, bucket_to_insert->hash);
	for (;;) {
		bucket = bucket_at_(map, buckets, i);
		if (!bucket->psl) { /* no other items shares the same value with the current item to insert */
			memcpy(bucket, bucket_to_insert, map->bucket_size);
			map->buckets_used++;
			return NULL;
		}
		/* test if there exists item of the same hashed value has inserted before */
		pa = bucket_data_(bucket_to_insert);
		pb = bucket_data_(bucket);
		if (bucket->hash == bucket_to_insert->hash &&
		    map->compare_cb_(map, pa, pb, map->user_data) == 0) {
			/* let user decide whether to destroy it (free resources of the item) */
			memcpy(map->temp_data_, pb, map->ele_size);
			/* replace */
			memcpy(pb, pa, map->ele_size);
			return map->temp_data_;
		}
		if ((uint32_t) bucket->psl < (uint32_t) bucket_to_insert->psl) {
			memcpy(map->temp_data_, bucket, map->bucket_size);
			memcpy(bucket, bucket_to_insert, map->bucket_size);
			memcpy(bucket_to_insert, map->temp_data_, map->bucket_size);
		}
		i = next_probe_sequence_(map, i, bucket_to_insert);
		bucket_to_insert->psl += 1;
	}
}

/**
 * @brief use user defined function <b>ele_OPUS_FREE_cb</b> to OPUS_FREE resources of all the
 * 		elements stored in the hashmap
 * @param map
 */
void destroy_elements_(opus_hashmap *map)
{
	uint64_t  i;
	bucket__ *bucket;

	if (map->ele_free_cb_) {
		for (i = 0; i < map->buckets_capacity; i++) {
			bucket = bucket_at_(map, map->buckets_, i);
			if (bucket->psl) map->ele_free_cb_(map, bucket_data_(bucket), map->user_data);
		}
	}
}

void opus_hashmap_done(opus_hashmap *map)
{
	if (!map) return;
	destroy_elements_(map);
	OPUS_FREE(map->temp_data_);
	OPUS_FREE(map->buckets_);
}

void opus_hashmap_destroy(opus_hashmap *map)
{
	if (!map) return;
	opus_hashmap_done(map);
	OPUS_FREE(map);
}

void opus_hashmap_clear(opus_hashmap *map)
{
	destroy_elements_(map);
	memset(map->buckets_, 0, map->buckets_capacity * map->bucket_size);
}

/**
 * @brief resize the hashmap, notices that capacity must be the multiple of 2
 * @param map
 * @param capacity must be the multiple of 2
 * @return
 */
opus_hashmap *hashmap_resize(opus_hashmap *map, uint64_t capacity)
{
	void     *new_buckets;
	uint64_t  i, old_buckets_capacity;
	bucket__ *bucket;

	/* reject illegal resize operation */
	if (capacity % 2 != 0) return map;

	if (capacity <= map->buckets_capacity && map->buckets_used > capacity)
		/* we have already reached the need, or we are not trying to shrink the size of hashmap */
		return map;

	new_buckets = OPUS_CALLOC(capacity, map->bucket_size);
	if (!new_buckets) return NULL;

	old_buckets_capacity  = map->buckets_capacity;
	map->buckets_capacity = capacity;
	map->mask_            = capacity - 1; /* mask is used in hashmap_bucket_idx */
	map->buckets_used     = 0;            /* hashmap_insert_internal will update this information */

	/* re-insert all the item from the old hashmap */
	for (i = 0; i < old_buckets_capacity; i++) {
		bucket = bucket_at_(map, map->buckets_, i);
		if (!bucket->psl) continue; /* no data is stored in this bucket, skip */
		bucket->psl = 1;
		/* no need to consider items with duplicated hash value */
		insert_(map, new_buckets, bucket);
	}

	OPUS_FREE(map->buckets_);
	map->buckets_   = new_buckets;
	map->grow_at_   = (uint64_t) ((double) map->buckets_capacity * HASHMAP_GROW_AT_FACTOR);
	map->shrink_at_ = (uint64_t) ((double) map->buckets_capacity * HASHMAP_SHRINK_AT_FACTOR);
	return map;
}

/**
 * @brief insert an element into hashmap, if an element with the same key has already inserted,
 * 		the old will be replaced and returned
 * @param map
 * @param ele_ptr pointer of the element to insert
 * @return
 */
void *opus_hashmap_insert(opus_hashmap *map, void *ele_ptr)
{
	bucket__ *bucket_to_insert;

	/* check if you want to insert NULL */
	if (ele_ptr == NULL) return NULL;

	/* check if we need to resize the hashmap */
	if (map->buckets_used == map->grow_at_) {
		if (!hashmap_resize(map, map->buckets_capacity * 2)) {
			return NULL;
		}
	}

	bucket_to_insert       = map->ele_data_;
	bucket_to_insert->hash = get_hash_(map, ele_ptr);
	bucket_to_insert->psl  = 1;
	memcpy(bucket_data_(bucket_to_insert), ele_ptr, map->ele_size);

	return insert_(map, map->buckets_, bucket_to_insert);
}

static void *remove_(opus_hashmap *map, void *ele_ptr, int shrink)
{
	uint64_t hash;
	uint64_t i;

	bucket__ *prev, *bucket;

	if (!ele_ptr)
		return NULL;

	hash = get_hash_(map, ele_ptr);
	i    = bucket_idx_(map, hash);
	for (;;) {
		bucket = bucket_at_(map, map->buckets_, i);
		if (!bucket->psl) {
			return NULL;
		}
		if (bucket->hash == hash && map->compare_cb_(map, ele_ptr, bucket_data_(bucket), map->user_data) == 0) {
			memcpy(map->temp_data_, bucket_data_(bucket), map->ele_size);
			bucket->psl = 0;
			for (;;) {
				prev   = bucket;
				i      = next_probe_sequence_(map, i, ele_ptr);
				bucket = bucket_at_(map, map->buckets_, i);
				if (bucket->psl <= 1) {
					prev->psl = 0;
					break;
				}
				memcpy(prev, bucket, map->bucket_size);
				prev->psl--;
			}
			map->buckets_used--;
			if (shrink && map->buckets_used <= map->shrink_at_)
				hashmap_resize(map, map->buckets_capacity / 2);
			return map->temp_data_;
		}
		i = next_probe_sequence_(map, i, ele_ptr);
	}
}

void *opus_hashmap_delete(opus_hashmap *map, void *ele_ptr)
{
	return remove_(map, ele_ptr, 1);
}

void *opus_hashmap_remove(opus_hashmap *map, void *ele_ptr)
{
	return remove_(map, ele_ptr, 0);
}

/**
 * @brief find the bucket in the hashmap and return it, will not OPUS_FREE any resources
 * @param map
 * @param ele_ptr require this as the key to search the specific element
 * @return pointer of the element
 */
void *opus_hashmap_retrieve(opus_hashmap *map, void *ele_ptr)
{
	uint64_t  i, hash;
	bucket__ *bucket;
	void    **bucket_data;

	if (ele_ptr == NULL) return NULL;

	hash = get_hash_(map, ele_ptr);
	i    = bucket_idx_(map, hash);
	for (;;) {
		bucket = bucket_at_(map, map->buckets_, i);
		if (!bucket->psl) { /* meet tombstone? */
			/* no need to continue search */
			return NULL;
		}
		/* check if the element in this bucket is the one we want */
		bucket_data = bucket_data_(bucket);
		if (bucket->hash == hash &&
		    map->compare_cb_(map, ele_ptr, bucket_data, map->user_data) == 0)
			return bucket_data;

		i = next_probe_sequence_(map, i, bucket);
	}
}

void opus_hashmap_dump(opus_hashmap *map, FILE *fp, opus_hashmap_print_data_cb print_data)
{
	uint64_t  i;
	char      text[1024];
	bucket__ *bucket;

	if (!fp || map->buckets_ == NULL) return;

	fprintf(fp, "CAPACITY %" PRIu64 ", ELE_SIZE %" PRIu64 ", BUCKET_SIZE %" PRIu64 "\n",
	        map->buckets_capacity, map->ele_size, map->bucket_size);
	fprintf(fp, "USER_DATA 0x%p\n", map->user_data);
	fprintf(fp, "%6s\t%16s\tDATA\n", "psl", "HASH");
	for (i = 0; i < map->buckets_capacity; i++) {
		bucket = bucket_at_(map, map->buckets_, i);

		if (bucket->psl) print_data(bucket_data_(bucket), text, 1024);
		else
			text[0] = '\0';
		fprintf(fp, "%6u\t%16" PRIu64 "\t%s\n", bucket->psl, bucket->hash, text);
	}
}

void *opus_hashmap_probe(opus_hashmap *hashmap, uint64_t index)
{
	bucket__ *bucket;
	OPUS_RETURN_IF(NULL, index > hashmap->buckets_capacity);
	bucket = bucket_at_(hashmap, hashmap->buckets_, index);
	OPUS_RETURN_IF(NULL, !bucket->psl);
	return bucket_data_(bucket);
}

void opus_hashmap_get_bucket_info(opus_hashmap *hashmap, uint64_t index, unsigned int *psl, void **ele)
{
	bucket__ *bucket;

	if (index > hashmap->buckets_capacity) {
		*psl = 0;
		*ele = NULL;
	} else {
		bucket = bucket_at_(hashmap, hashmap->buckets_, index);
		*ele   = bucket_data_(bucket);
		*psl   = bucket->psl;
	}
}
