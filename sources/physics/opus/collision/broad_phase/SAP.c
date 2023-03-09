/**
 * @file SAP.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2023/2/28
 *
 * @brief sweep and prune
 *
 */

#include "data_structure/array.h"
#include "physics/opus/physics.h"
#include "physics/opus/physics_private.h"

static int bodies_sort_x_(const void *a, const void *b)
{
	opus_body *ba = *(opus_body **) a;
	opus_body *bb = *(opus_body **) b;
	return opus_sign(ba->shape->bound.min.x - bb->shape->bound.min.x);
}

void opus_SAP(opus_body **bodies, size_t n, opus_sap_cb callback, void *data)
{
	size_t i, j;

	opus_body *A, *B;

	opus_mat2d ta, tb;
	opus_aabb  ba, bb;

	/* if no bodies exist, exit */
	if (bodies == NULL) return;

	/* update AABB */
	for (i = 0; i < n; i++) {
		A = bodies[i];
		A->shape->update_bound(A->shape, A->rotation, A->position);
	}

	/* sort bodies by X in ascending order */
	qsort(bodies, n, sizeof(opus_body *), bodies_sort_x_);
	for (i = 0; i < n; i++) {
		A = bodies[i];

		for (j = i + 1; j < n; j++) {
			B = bodies[j];

			/* calculate rigid body transformation matrix */
			opus_mat2d_rotate_about(ta, (float) A->rotation, A->position);
			opus_mat2d_rotate_about(tb, (float) B->rotation, B->position);

			ba = A->shape->bound;
			bb = B->shape->bound;

			/* X-axis: we have already sorted all the bodies in X axis */
			if (bb.min.x > ba.max.x) break;

			/* Y-axis: check AABB bounding box to check the two bodies can collide */
			if (ba.max.y < bb.min.y || ba.min.y > bb.max.y) continue;

			/* check bitmask to determine if the two can collide */
			if (!(A->bitmask & B->bitmask)) continue;

			callback(A, B, ta, tb, data);
		}
	}
}

