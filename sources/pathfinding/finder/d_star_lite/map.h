/**
 * @file map.h
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/12/17
 *
 * @example
 *
 * @development_log
 *
 */
#ifndef MAP_H
#define MAP_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <limits.h>
#include <float.h>

#define MAP_MAX (UINT_MAX)
#define CELL_PRIORITY_MAX (DBL_MAX)
#define CELL_UNWALKABLE (DBL_MAX)
#define CELL_PRIORITY_MIN (DBL_MIN)
#define MAX_NEIGHBORS (8)

typedef struct cell cell_t;
typedef struct map map_t;
typedef struct priority priority_t;

struct priority {
	double k1; /* g */
	double k2; /* rhs */
};

struct cell {
	double cost;
	unsigned int x, y;
	int           is_open;
	struct cell **neighbors;
	priority_t priority;
};

/* occupancy map */
struct map {
	cell_t **cells;
	unsigned int cols, rows;
};

map_t *map_init(map_t *map, unsigned rows, unsigned cols);
void map_done(map_t *map);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* MAP_H */

