#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "graph.h"
#include "pathfinder.h"

void print_n(graph_t *g, graph_id_t id)
{
	struct arc {
		graph_weight_t weight;
		graph_id_t     next, to;
	};

	struct context {
		graph_count_t head_count, arc_count;
		graph_count_t max_head, max_arc;

		void      **vertex_data;
		graph_id_t *heads;
		struct arc *arcs;
	};

	struct context *c    = g->context_;
	graph_id_t      next = c->heads[id];
	while (next != GRAPH_MAX_ID) {
		printf("%" PRIu32 "  ", c->arcs[next].to);
		next = c->arcs[next].next;
	}
	printf("\n");
}

void forward_star()
{
	graph_t *g = graph_forward_star_create(1, 10, 4 * 10);
	int      i;
	for (i = 0; i < 10; i++) {
		uint64_t j = i;
		g->add_vertex(g, (graph_data_t) j);
	}
	g->add_arc_by_id(g, 0, 1, 2);
	g->add_arc_by_id(g, 0, 4, 8);
	g->add_arc_by_id(g, 0, 2, 0);
	g->add_arc_by_id(g, 1, 4, 3);
	g->add_arc_by_id(g, 1, 2, 6);
	g->add_arc_by_id(g, 1, 3, 7);
	g->add_arc_by_id(g, 2, 4, 4);
	g->add_arc_by_id(g, 2, 3, 5);
	g->add_arc_by_id(g, 4, 1, 1);

	print_n(g, 0);
	print_n(g, 1);
	print_n(g, 2);
	print_n(g, 3);
	print_n(g, 4);

	{
		graph_id_t   *neighbors = malloc(sizeof(void *) * 4);
		graph_count_t count;
		g->get_neighbors_by_id(g, 1, neighbors, &count);

		printf("=========\n");
		for (i = 0; i < count; i++) {
			printf("%d  ", neighbors[i]);
		}
		printf("\n");
	}

	graph_forward_star_destroy(g);
}

void grid()
{
	char map[10][10] = {
	        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	        {0, 1, 0, 0, 0, 1, 0, 1, 0, 0},
	        {0, 1, 1, 0, 0, 1, 0, 1, 0, 0},
	        {0, 0, 1, 0, 0, 1, 0, 1, 0, 1},
	        {0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
	        {0, 1, 1, 1, 1, 1, 0, 1, 1, 0},
	        {0, 0, 1, 0, 0, 0, 0, 1, 0, 0},
	        {1, 1, 1, 0, 0, 0, 0, 1, 0, 1},
	        {0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
	        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	};
	graph_t      *g    = graph_grid_create(1, 0, 0, 0, 10, 10, 10, 10);
	pathfinder_t *p    = pathfinder_d_star_create(g);
	graph_id_t   *path = NULL;
	graph_count_t n    = 0;
	graph_grid_map(g, (char **) map);
	graph_grid_print_map01(g);

	pathfinder_search(p, 0, 90);
	pathfinder_get_path(p, &path, &n);

	if (p) {
		int  x, y, i;
		char md[10][10];
		memcpy(md, map, sizeof(char) * 100);
		printf("\n\npath(%d):", n);
		for (i = 0; i < n; i++) {
			graph_id_t tx, ty;
			graph_id2xy(path[i], 10, &tx, &ty);
			md[ty][tx] = 2;
			printf("(%d, %d) ", tx, ty);
		}
		printf("\n\n");
		for (y = 0; y < 10; y++) {
			for (x = 0; x < 10; x++) {
				if (md[y][x] == 0) printf(".");
				else if (md[y][x] == 1)
					printf("#");
				else
					printf("+");
				printf(" ");
			}
			printf("\n");
		}
	}

	g->set_arc_weight_by_id(g, 1, 0, 1);
	printf("\n\nchanged map: \n\n");
	graph_grid_print_map01(g);
	pathfinder_d_star_mark_dirty(p);
	pathfinder_search(p, 0, 90);

	if (p) {
		int  x, y, i;
		char md[10][10];
		memcpy(md, map, sizeof(char) * 100);
		printf("\n\npath(%d):", n);
		for (i = 0; i < n; i++) {
			graph_id_t tx, ty;
			graph_id2xy(path[i], 10, &tx, &ty);
			md[ty][tx] = 2;
			printf("(%d, %d) ", tx, ty);
		}
		printf("\n\n");
		for (y = 0; y < 10; y++) {
			for (x = 0; x < 10; x++) {
				if (md[y][x] == 0) printf(".");
				else if (md[y][x] == 1)
					printf("#");
				else
					printf("+");
				printf(" ");
			}
			printf("\n");
		}
	}

	graph_destroy(g);
	pathfinder_destroy(p);
}

void parr(int *arr, int n)
{
	int i;
	for (i = 0; i < n; i++) {
		printf("%d  ", arr[i]);
	}
	printf("\n");
}

int c(void *a, void *b)
{
	return *(int *) a - *(int *) b;
}

int c2(graph_min_bheap_t *h, void *a, void *b)
{
	return *(int *) a - *(int *) b;
}

void bheap()
{
	int               arr[10] = {0};
	graph_min_bheap_t h       = {
            NULL, NULL, 10, 0, sizeof(int), c2};
	int  e1 = 1, e2 = 2, e3 = 3, e4 = 4, e5 = 5, e6 = 6, e7 = 7, e8 = 8, e9 = 9, e10 = 10;
	int *e;

	h.data_ = arr;

	parr(arr, 10);

	graph_min_bheap_push(&h, &e4);
	graph_min_bheap_push(&h, &e5);
	graph_min_bheap_push(&h, &e2);
	graph_min_bheap_push(&h, &e3);
	graph_min_bheap_push(&h, &e7);
	graph_min_bheap_push(&h, &e8);
	graph_min_bheap_push(&h, &e6);
	graph_min_bheap_push(&h, &e9);
	parr(arr, 10);

	graph_min_bheap_remove(&h, 2);
	parr(arr, 10);
}

int main()
{
	grid();
	/*forward_star();*/
	/*bheap();*/


	return 0;
}
