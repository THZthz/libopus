#include "data_structure/hashmap.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

typedef struct _bucket {
	uint64_t hash;
	unsigned psl;
} _bucket_t;

/**
 * @brief copied from https://github.com/rxi/map/tree/master/src
 * @param map
 * @param key
 * @param count
 * @return
 */
uint64_t hashmap_simple_hash(hashmap_t *map, void *key, int count)
{
	char    *s    = key;
	unsigned hash = 5381;
	while (count-- > 0) {
		hash = ((hash << 5) + hash) ^ *s++;
	}
	return hash;
}

OPUS_INLINE void hashmap_mm86128_internal(const void *key, const int count, uint32_t seed, void *out)
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
uint64_t hashmap_murmur(const void *data, uint64_t count, uint64_t seed0, uint64_t seed1)
{
	char out[16];
	hashmap_mm86128_internal(data, (int) count, seed0, &out);
	return *(uint64_t *) out;
}

/**
 * @brief make sure capacity is always the multiple of 2
 * @param desired_capacity
 * @return
 */
OPUS_INLINE uint64_t hashmap_get_proper_capacity(uint64_t desired_capacity)
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
hashmap_t *hashmap_init(hashmap_t *map, uint64_t ele_size, uint64_t capacity, uint64_t seed0, uint64_t seed1,
                        hashmap_compare_cb compare, hashmap_hash_cb hash, hashmap_ele_free_cb ele_free)
{
	uint64_t bucket_size = sizeof(_bucket_t) + ele_size;

	while (bucket_size & (sizeof(_bucket_t) - 1)) {
		bucket_size++;
	}

	map->ele_size         = ele_size;
	map->bucket_size      = bucket_size;
	map->buckets_capacity = hashmap_get_proper_capacity(capacity);
	map->mask_            = map->buckets_capacity - 1;
	map->buckets_used     = 0;
	map->temp_data_       = malloc(map->bucket_size * 2);
	if (!map->temp_data_) return NULL;
	map->ele_data_ = (char *) map->temp_data_ + map->bucket_size;
	map->buckets_  = malloc(map->bucket_size * map->buckets_capacity);
	if (!map->buckets_) {
		free(map->temp_data_);
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

hashmap_t *hashmap_create(uint64_t ele_size, uint64_t capacity, uint64_t seed0, uint64_t seed1,
                          hashmap_compare_cb compare, hashmap_hash_cb hash, hashmap_ele_free_cb ele_free)
{
	hashmap_t *map = (hashmap_t *) malloc(sizeof(hashmap_t));
	if (!map) return NULL;
	return hashmap_init(map, ele_size, capacity, seed0, seed1, compare, hash, ele_free);
}

OPUS_INLINE uint64_t hashmap_bucket_idx_(hashmap_t *map, uint64_t hash_value)
{
	/* If the implementation is changed to allow a non-power-of-2 bucket count, */
	/* the line below should be changed to use mod instead of AND */
	return hash_value & map->mask_;
}

OPUS_INLINE _bucket_t *hashmap_bucket_at_(hashmap_t *map, void *buckets, uint64_t i)
{
	return (_bucket_t *) ((char *) buckets + i * map->bucket_size);
}

OPUS_INLINE void *hashmap_bucket_data_(_bucket_t *bucket)
{
	return (void *) ((char *) bucket + sizeof(_bucket_t));
}

OPUS_INLINE uint64_t hashmap_get_hash_(hashmap_t *map, void *ele)
{
	/* clear the 16 bits of the left of the hashed value(for the sake of psl) */
	return map->hash_cb_(map, ele, map->seed0, map->seed1, map->user_data) << 16 >> 16;
}

OPUS_INLINE uint64_t hashmap_next_probe_sequence_(hashmap_t *map, uint64_t cur_idx, void *ele)
{
	/* simple linear probe */
	return hashmap_bucket_idx_(map, cur_idx + 1);
}

void *hashmap_insert_internal_(hashmap_t *map, void *buckets, _bucket_t *bucket_to_insert)
{
	uint64_t i = hashmap_bucket_idx_(map, bucket_to_insert->hash);
	for (;;) {
		_bucket_t *bucket = hashmap_bucket_at_(map, buckets, i);
		if (!bucket->psl) { /* no other items shares the same value with the current item to insert */
			memcpy(bucket, bucket_to_insert, map->bucket_size);
			map->buckets_used++;
			return NULL;
		}
		/* test if there exists item of the same hashed value has inserted before */
		if (bucket->hash == bucket_to_insert->hash &&
		    map->compare_cb_(map, hashmap_bucket_data_(bucket_to_insert), hashmap_bucket_data_(bucket), map->user_data) == 0) {
			/* let user decide whether to destroy it (free resources of the item) */
			memcpy(map->temp_data_, hashmap_bucket_data_(bucket), map->ele_size);
			/* replace */
			memcpy(hashmap_bucket_data_(bucket), hashmap_bucket_data_(bucket_to_insert), map->ele_size);
			return map->temp_data_;
		}
		if ((uint32_t) bucket->psl < (uint32_t) bucket_to_insert->psl) { /* FIXME */
			memcpy(map->temp_data_, bucket, map->bucket_size);
			memcpy(bucket, bucket_to_insert, map->bucket_size);
			memcpy(bucket_to_insert, map->temp_data_, map->bucket_size);
		}
		i = hashmap_next_probe_sequence_(map, i, bucket_to_insert);
		bucket_to_insert->psl += 1;
	}
}

/**
 * @brief use user defined function <b>ele_free_cb</b> to free resources of all the
 * 		elements stored in the hashmap
 * @param map
 */
void hashmap_destroy_elements(hashmap_t *map)
{
	if (map->ele_free_cb_) {
		uint64_t i;
		for (i = 0; i < map->buckets_capacity; i++) {
			_bucket_t *bucket = hashmap_bucket_at_(map, map->buckets_, i);
			if (bucket->psl) map->ele_free_cb_(map, hashmap_bucket_data_(bucket), map->user_data);
		}
	}
}

void hashmap_destroy(hashmap_t *map)
{
	if (!map) return;
	hashmap_destroy_elements(map);
	free(map->temp_data_);
	free(map->buckets_);
	free(map);
}

void hashmap_clear(hashmap_t *map)
{
	hashmap_destroy_elements(map);
	memset(map->buckets_, 0, map->buckets_capacity * map->bucket_size);
}

/**
 * @brief resize the hashmap, notices that capacity must be the multiple of 2
 * @param map
 * @param capacity must be the multiple of 2
 * @return
 */
hashmap_t *hashmap_resize(hashmap_t *map, uint64_t capacity)
{
	uint64_t i, old_buckets_capacity;
	void    *new_buckets;

	/* reject illegal resize operation */
	if (capacity % 2 != 0) return map;

	if (capacity <= map->buckets_capacity && map->buckets_used > capacity)
		/* we have already reached the need, or we are not trying to shrink the size of hashmap */
		return map;

	new_buckets = malloc(map->bucket_size * capacity);
	if (!new_buckets) return NULL;

	old_buckets_capacity  = map->buckets_capacity;
	map->buckets_capacity = capacity;
	map->mask_            = capacity - 1; /* mask is used in hashmap_bucket_idx */
	map->buckets_used     = 0;            /* hashmap_insert_internal will update this information */
	memset(new_buckets, 0, map->bucket_size * capacity);

	/* re-insert all the item from the old hashmap */
	for (i = 0; i < old_buckets_capacity; i++) {
		_bucket_t *bucket = hashmap_bucket_at_(map, map->buckets_, i);
		if (!bucket->psl) continue; /* no data is stored in this bucket, skip */
		bucket->psl = 1;
		/* no need to consider items with duplicated hash value */
		hashmap_insert_internal_(map, new_buckets, bucket);
	}

	free(map->buckets_);
	map->buckets_   = new_buckets;
	map->grow_at_   = (uint64_t) ((double) map->buckets_capacity * HASHMAP_GROW_AT_FACTOR);
	map->shrink_at_ = (uint64_t) ((double) map->buckets_capacity * HASHMAP_SHRINK_AT_FACTOR);
	return map;
}

/**
 * @brief insert an element into hashmap, if an element with the same key has already inserted,
 * 		the old will be replaced and returned
 * @param map
 * @param ele
 * @return
 */
void *hashmap_insert(hashmap_t *map, void *ele)
{
	_bucket_t *bucket_to_insert;

	/* check if you want to insert NULL */
	if (ele == NULL) return NULL;

	/* check if we need to resize the hashmap */
	if (map->buckets_used == map->grow_at_) {
		if (!hashmap_resize(map, map->buckets_capacity * 2)) {
			return NULL;
		}
	}

	bucket_to_insert       = map->ele_data_;
	bucket_to_insert->hash = hashmap_get_hash_(map, ele);
	bucket_to_insert->psl  = 1;
	memcpy(hashmap_bucket_data_(bucket_to_insert), ele, map->ele_size);

	return hashmap_insert_internal_(map, map->buckets_, bucket_to_insert);
}

void *hashmap_delete(hashmap_t *map, void *ele)
{
	uint64_t hash;
	uint64_t i;

	if (!ele)
		return NULL;

	hash = hashmap_get_hash_(map, ele);
	i    = hashmap_bucket_idx_(map, hash);
	for (;;) {
		_bucket_t *bucket = hashmap_bucket_at_(map, map->buckets_, i);
		if (!bucket->psl) {
			return NULL;
		}
		if (bucket->hash == hash && map->compare_cb_(map, ele, hashmap_bucket_data_(bucket), map->user_data) == 0) {
			memcpy(map->temp_data_, hashmap_bucket_data_(bucket), map->ele_size);
			bucket->psl = 0;
			for (;;) {
				_bucket_t *prev = bucket;
				i               = hashmap_next_probe_sequence_(map, i, ele);
				bucket          = hashmap_bucket_at_(map, map->buckets_, i);
				if (bucket->psl <= 1) {
					prev->psl = 0;
					break;
				}
				memcpy(prev, bucket, map->bucket_size);
				prev->psl--;
			}
			map->buckets_used--;
			if (map->buckets_used <= map->shrink_at_)
				hashmap_resize(map, map->buckets_capacity / 2);
			return map->temp_data_;
		}
		i = hashmap_next_probe_sequence_(map, i, ele);
	}
}

/**
 * @brief find the bucket in the hashmap and return it, will not free any resources
 * @param map
 * @param ele require this as the key to search the specific element
 * @return
 */
void *hashmap_retrieve(hashmap_t *map, void *ele)
{
	uint64_t i, hash;
	if (ele == NULL) return NULL;

	hash = hashmap_get_hash_(map, ele);
	i    = hashmap_bucket_idx_(map, hash);
	for (;;) {
		_bucket_t *bucket = hashmap_bucket_at_(map, map->buckets_, i);
		if (!bucket->psl) { /* meet tombstone? */
			/* no need to continue search */
			return NULL;
		}
		/* check if the element in this bucket is the one we want */
		if (bucket->hash == hash && map->compare_cb_(map, ele, hashmap_bucket_data_(bucket), map->user_data) == 0)
			return hashmap_bucket_data_(bucket);

		i = hashmap_next_probe_sequence_(map, i, bucket);
	}
}

void hashmap_dump(hashmap_t *map, FILE *fp, hashmap_print_data_cb print_data)
{
	uint64_t i;
	char     text[1024];

	if (!fp || map->buckets_ == NULL) return;

	fprintf(fp, "CAPACITY %" PRIu64 ", ELE_SIZE %" PRIu64 ", BUCKET_SIZE %" PRIu64 "\n", map->buckets_capacity, map->ele_size, map->bucket_size);
	fprintf(fp, "USER_DATA 0x%p\n", map->user_data);
	fprintf(fp, "%6s\t%16s\tDATA\n", "psl", "HASH");
	for (i = 0; i < map->buckets_capacity; i++) {
		_bucket_t *bucket  = hashmap_bucket_at_(map, map->buckets_, i);

		if (bucket->psl) print_data(hashmap_bucket_data_(bucket), text, 1024);
		else
			text[0] = '\0';
		fprintf(fp, "%6u\t%16"PRIu64"\t%s\n", bucket->psl, bucket->hash, text);
	}
}

void *hashmap_probe(hashmap_t *hashmap, uint64_t index)
{
	if (index > hashmap->buckets_capacity) return NULL;
	return hashmap_bucket_data_(hashmap_bucket_at_(hashmap, hashmap->buckets_, index));
}

void hashmap_get_bucket_info(hashmap_t *hashmap, uint64_t index, unsigned int *psl, void **ele)
{
	if (index > hashmap->buckets_capacity) {
		*psl = 0;
		*ele = NULL;
	} else {
		struct _bucket *bucket = hashmap_bucket_at_(hashmap, hashmap->buckets_, index);
		*ele                   = hashmap_bucket_data_(bucket);
		*psl                   = bucket->psl;
	}
}
