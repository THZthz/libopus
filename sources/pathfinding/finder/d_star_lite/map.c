/**
 * @file map.c
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

#include "pathfinding/finder/d_star_lite/map.h"
#include "utils/utils.h"

#include <stdlib.h>
#include <string.h>

static void *deallocate_cells(map_t *map)
{
	unsigned y, rows;

	rows = map->rows;
	for (y = 0; y < rows; y++)
		if (map->cells[y]) {
			OPUS_FREE(map->cells[y]);
			map->cells[y] = NULL;
		} else {
			break; /* following rows are not "malloc"ed */
		}

	OPUS_FREE(map->cells);
	map->cells = NULL;

	return NULL;
}

static void *deallocate_neighbors(map_t *map)
{
	unsigned x, y, rows, cols;

	rows = map->rows;
	cols = map->cols;
	for (y = 0; y < rows; ++y)
		for (x = 0; x < cols; ++x) {
			cell_t *cell = &(map->cells[y][x]);
			if (cell->neighbors) {
				OPUS_FREE(cell->neighbors);
				cell->neighbors = NULL;
			} else {
				break;
			}
		}

	return NULL;
}

map_t *map_init(map_t *map, unsigned rows, unsigned cols)
{
	map->rows = rows;
	map->cols = cols;

	/* allocate space for cells and initialize it */
	{
		unsigned x, y;
		map->cells = (cell_t **) malloc(sizeof(cell_t *) * rows);
		if (!map->cells) return NULL;
		memset(map->cells, 0, sizeof(cell_t *) * rows); /* for convenience of recycling */
		for (y = 0; y < rows; ++y) {
			cell_t *row = (cell_t *) malloc(sizeof(cell_t) * cols);
			if (!row) {
				deallocate_cells(map);
				return NULL;
			}
			map->cells[y] = row;
			for (x = 0; x < cols; ++x) {
				cell_t *cell    = &(map->cells[y][x]);
				cell->x         = x;
				cell->y         = y;
				cell->cost      = 0;
				cell->neighbors = NULL;
				cell->is_open     = 0;
				cell->priority.k1 = CELL_PRIORITY_MAX;
				cell->priority.k2 = CELL_PRIORITY_MAX;
			}
		}
	}

	/* initialize cell's neighbors */
	{
		unsigned x, y;
		for (y = 0; y < rows; ++y) {
			for (x = 0; x < cols; ++x) {
				int ox, oy, k;

				cell_t  *cell      = &(map->cells[y][x]);
				cell_t **neighbors = (cell_t **) malloc(sizeof(cell_t *) * MAX_NEIGHBORS);

				if (!neighbors) {
					deallocate_neighbors(map);
					deallocate_cells(map);
					return NULL;
				}

				k = 0;
				for (ox = -1; ox <= 1; ox++) {
					for (oy = -1; oy <= 1; oy++) {
						cell_t *neighbor = NULL;
						if (x == 0 && ox < 0) goto next;
						if (y == 0 && oy < 0) goto next;
						if (y == map->rows - 1 && oy > 0) goto next;
						if (x == map->cols - 1 && ox > 0) goto next;

						neighbor = &(map->cells[y + oy][x + ox]);
					next:
						neighbors[k] = neighbor;
						k++;
					}
				}

				cell->neighbors = neighbors;
			}
		}
	}

	return map;
}

void map_done(map_t *map)
{
	deallocate_neighbors(map);
	deallocate_cells(map);
}
