/**
 * @file axes.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/8/25
 *
 * @example
 *
 * @development_log
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "physics/physics.h"

struct gradient_cont {
	real id;
	vec2 n;
};

static int gradient_cmp(const void *a, const void *b)
{
	real res = ((struct gradient_cont *) a)->id - ((struct gradient_cont *) b)->id;
	return r_sign(res);
}

static int sort_gradients(const void *a, const void *b)
{
	const struct gradient_cont* ga = a, *gb = b;
	return r_sign (ga->n.x == gb->n.x ? ga->n.y - gb->n.y : ga->n.x - gb->n.x);
}

static size_t bisect_left_asc(void *arr, size_t ele_size, size_t n, const void *ptr, int (*cmp)(const void *, const void *))
{
	int64_t left = 0, right = (int64_t) n - 1, mid = (left + right) / 2;
	while (left <= right) {
		if (cmp((char *) arr + ele_size * mid, ptr) >= 0) right = mid - 1;
		else
			left = mid + 1;
		mid = (left + right) / 2;
	}
	return left;
}

/**
 * @param axes should contain at least `n` slots
 */
void axes_set_from_vertices(vertices_t vertices, size_t n, axes_t axes, size_t *n_axes)
{
	struct gradient_cont *gradients = (struct gradient_cont *) malloc(sizeof(struct gradient_cont) * n);
	struct gradient_cont  index;
	size_t                n_gradients = 0;
	size_t                i, j, k;
	real                  length;

	/* make sure all the normals are unique and do not have parallel siblings */
	/* also confine its precision to avoid consuming quantities of computing power */
	for (i = 0; i < n; i++) {
		j = (i + 1) % n;

		/* normal perpendicular to edge */
		index.n.x = vertices[j].y - vertices[i].y;
		index.n.y = vertices[i].x - vertices[j].x;
		length    = r_hypot(index.n.x, index.n.y);
		index.n.x /= length;
		index.n.y /= length;
		index.id = (index.n.y == 0) ? REAL_MAX : (index.n.x / index.n.y);
		/*index.id = (real) round_with_precision(index.id, 3);*/ /* limit precision */

		k = bisect_left_asc(gradients, sizeof(struct gradient_cont), n_gradients, &index, gradient_cmp);

		if (k < n_gradients && gradients[k].id == index.id) {
			gradients[k].n.x = index.n.x;
			gradients[k].n.y = index.n.y;
		} else {
			/* insert to position "k" */
			if (k != n_gradients) memmove(gradients + k + 1, gradients + k, sizeof(struct gradient_cont) * (n_gradients - k));
			gradients[k].id  = index.id;
			gradients[k].n.x = index.n.x;
			gradients[k].n.y = index.n.y;
			n_gradients++;
		}
	}

	/* output */
	*n_axes = n_gradients;
	qsort(gradients, n_gradients,sizeof(struct gradient_cont),  sort_gradients);
	for (i = 0; i < n_gradients; i++) {
		axes[i].x = gradients[i].n.x;
		axes[i].y = gradients[i].n.y;
	}

	free(gradients);
}

/**
 * rotate the normals by a given angle
 * @param axes
 * @param n
 * @param angle
 */
void axes_rotate(axes_t axes, size_t n, real angle)
{
	size_t i;
	real   c, s, x;

	if (angle == 0) return;

	c = r_cos(angle),
	s = r_sin(angle);

	for (i = 0; i < n; i++) {
		x         = axes[i].x * c - axes[i].y * s;
		axes[i].y = axes[i].x * s + axes[i].y * c;
		axes[i].x = x;
	}
}

