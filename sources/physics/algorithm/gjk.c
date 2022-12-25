/**
 * @file gjk.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/11/4
 *
 * @example
 *
 * @development_log
 *
 */

/**
 * @brief in fact it is a voronoi region test
 * 		refer to `geo_voronoi_region` for more detail
 */
#define gjk_origin_to_right(a, b) ((vec_x(a) - vec_x(b)) * (vec_y(a) + vec_y(b)) > (vec_y(a) - vec_y(b)) * (vec_x(a) + vec_x(b)))
#define gjk_origin_to_left(a, b) ((vec_x(a) - vec_x(b)) * (vec_y(a) + vec_y(b)) < (vec_y(a) - vec_y(b)) * (vec_x(a) + vec_x(b)))

/**
 * @brief initialize a minkowski point
 */
#define gjk_minkowski_point_init(pnt, _a, _b) \
	do {                                      \
		vec2_copy((pnt).a, (_a));             \
		vec2_copy((pnt).b, (_b));             \
		vec2_sub((pnt).v, (_a), (_b));        \
	} while (0)

typedef void (*gjk_support_func_t)(void *shape, vec2 dir, vec2 res);

struct gjk_minkowski_point {
	vec2 a; /*point on shape_a*/
	vec2 b; /*point on shape_b*/
	vec2 v; /*b - a*/
};

struct gjk_minkowski_edge {
	struct gjk_minkowski_point head, tail;

	vec2 normal;   /*collision normal of the edge, rotate 90deg clockwise from vector head.to(tail), it always points to shape_b*/
	vec2 point;    /*interpolated minkowski point*/
	real distance; /*distance of the two world points (contact separation), can be negative, if so, the two shape do not intersect*/
	real t;        /*lerp ratio of the two world points*/
};

struct gjk_result {
	void   *shape_a;
	void   *shape_b;
	vec2    simplex[3];
	count_t n;

	struct gjk_minkowski_edge  edge; /*the nearest edge between the two shape, on shape_a*/
	struct gjk_minkowski_point m0, m1, m2;
};


static void GJK_minkowski_edge_init(struct gjk_minkowski_edge *edge, struct gjk_minkowski_point *head, struct gjk_minkowski_point *tail)
{
	real t, distance;
	vec2 point, delta, normal;

	/*get the closest minkowski point*/
	vec2_lerp_ratio(t, tail->v, head->v);
	vec2_lerp(point, tail->v, head->v, t);

	/*calculate the contact normal and penetration distance*/
	vec2_sub(delta, head->v, tail->v);
	vec2_skewT(normal, delta);
	vec2_norm(normal, normal);
	distance = vec2_dot(normal, point);

	/*init the minkowski edge*/
	edge->distance = distance;
	vec2_copy(edge->normal, normal);
	vec2_copy(edge->point, point);
	edge->head = *head;
	edge->tail = *tail;
	edge->t    = t;
}

static real GJK_dist_to_origin_sq(vec2 t, vec2 h)
{
	vec2 p, origin;
	vec2_set(origin, 0, 0);
	get_nearest_point_on_line(t, h, origin, &p);
	return vec2_get_length2(p);
}

static void GJK_support_point(vertices_t vertices1, count_t count1, vertices_t vertices2, count_t count2, vec2 d, struct gjk_minkowski_point *res)
{
	vertex_t a, b;
	/*a = collision_get_support_point(vertices1, vec2_(0, 0), vec2_(-d.x, -d.y));
	b = collision_get_support_point(vertices2, vec2_(0, 0), d);*/

	/*subtract (Minkowski sum) the two points to see if bodies 'overlap'*/
	gjk_minkowski_point_init(*res, a, b);
}

static void EPA(vertices_t vertices1, count_t n1, vertices_t vertices2, count_t n2, struct gjk_result *result)
{
	int     iter;
	count_t n, o, limit_iteration = 29;

	/*hull of the expanding simplex*/
	struct gjk_minkowski_point  hull0[32] = {0};
	struct gjk_minkowski_point  hull1[32] = {0};
	struct gjk_minkowski_point *hull      = NULL; /*pointer points to the hull we are using*/
	struct gjk_minkowski_point *hull_old, *hull_new;
	struct gjk_minkowski_point  head, tail, m;

	count_t   count, ti, hi, t, i, h;
	real      min_dist, d;
	vec2      dir;
	core_bool odd;

	count = 3;

	/*hull has 3 points at the beginning*/
	hull0[0] = result->m0;
	hull0[1] = result->m1;
	hull0[2] = result->m2;
	hull     = hull0;

	/*begin main EPA algorithm*/
	for (iter = 0; iter < limit_iteration; ++iter) {
		min_dist = REAL_MAX; /*min distance to an edge*/

		ti = 0; /*index of the tail*/
		hi = 0; /*index of the head*/

		/*find the closest edge to the origin*/
		for (t = count - 1, h = 0; h < count; t = h, h++) {
			d = GJK_dist_to_origin_sq(hull[t].v, hull[h].v);
			if (d < min_dist) {
				min_dist = d;
				ti       = t;
				hi       = h;
			}
		}

		/*get the two minkowski points on the hull with the closest edge to the origin*/
		head = hull[hi];
		tail = hull[ti];

		/*get the new point on the hull*/
		vec2_sub(dir, head.v, tail.v);
		vec2_skewT(dir, dir);
		GJK_support_point(vertices1, n1, vertices2, n2, dir, &m);

		/*check if the point is already in the hull*/
		for (i = 0; i < count; ++i) {
			if (vec2_equal(hull[i].v, m.v)) {
				/*the point is already on the hull, return the minkowski edge*/
				GJK_minkowski_edge_init(&result->edge, &head, &tail);
				return;
			}
		}

		/*rebuild the hull*/
		odd      = iter & 1; /*check if we are on an odd number iter*/
		hull_old = odd ? hull1 : hull0;
		hull_new = odd ? hull0 : hull1;

		count++;

		/*copy the hull over*/
		for (n = 0, o = 0; n < count; ++n) {
			hull_new[n] = (n == hi) ? m : hull_old[o++];
		}
		hull = hull_new;
	}
	GJK_minkowski_edge_init(&result->edge, hull + 0, hull + 1);
}

/* FIXME: return value check */
static core_bool GJK(vertices_t vertices1, count_t n1, vertices_t vertices2, count_t n2, struct gjk_result *result)
{
	int limit_iteration = 16; /*avoid endless loop*/

	vec2                       normal, negate; /*direction for support function*/
	vec2                       c1, c2;         /*average center of two polygons, it is acceptable to use average center*/
	struct gjk_minkowski_point m0, m1, m2;
	vec2                       dir; /*new direction to search for support point*/
	vec2                       tmp; /*for testing usage*/
	core_bool                  m0_is_closer;
	count_t                    i;

	vec2_set(tmp, 0, 0);
	vertices_mean(vertices1, array_len(vertices1), &c1, sizeof(vertex_t));
	vertices_mean(vertices2, array_len(vertices2), &c2, sizeof(vertex_t));
	vec2_sub(normal, c1, c2);
	vec2_skew(normal, normal);
	vec2_reverse(negate, normal);

	/*initial support points*/
	GJK_support_point(vertices1, n1, vertices2, n2, normal, &m0);
	GJK_support_point(vertices1, n1, vertices2, n2, negate, &m1);

	/*make sure the origin is always in the left voronoi region of the edge.*/
	if (gjk_origin_to_right(m0.v, m1.v)) {
		vec2_swap(m0.v, m1.v);
		vec2_swap(m0.a, m1.a);
		vec2_swap(m0.b, m1.b);
	}

	for (i = 0; i < limit_iteration; ++i) {
		vec2_sub(dir, m0.v, m1.v);
		vec2_skew(dir, dir); /*a direction points to origin*/

		GJK_support_point(vertices1, n1, vertices2, n2, dir, &m2); /*now we get a simplex*/

		/*check if the origin is in the bound of the simplex or the new support point is near the origin*/
		if (gjk_origin_to_left(m1.v, m2.v) && gjk_origin_to_left(m2.v, m0.v) || vec2_equal(m2.v, tmp)) {
			result->m0 = m0;
			result->m1 = m1;
			result->m2 = m2;
			EPA(vertices1, n1, vertices2, n2, result); /*expand polytope to get penetration information*/
			return result->edge.distance >= 0;
		} else {
			/*check the closest edge to origin*/
			if (core_max(vec2_dot(m0.v, dir), vec2_dot(m1.v, dir)) >= vec2_dot(m2.v, dir)) {
				GJK_minkowski_edge_init(&result->edge, &m1, &m0);
				return result->edge.distance >= 0;
			} else {
				/*check which edge contains a closer point to the origin*/
				m0_is_closer = GJK_dist_to_origin_sq(m0.v, m2.v) > GJK_dist_to_origin_sq(m1.v, m2.v);

				/*update simplex, discard the simplex vertex that is farther from origin*/
				if (m0_is_closer) {
					m0 = m2;
				} else {
					m1 = m2;
				}
			}
		}
	}

	/*GJK terminated without converging and without an intersection.*/
	GJK_minkowski_edge_init(&result->edge, &m1, &m0);
	return result->edge.distance >= 0;
}
