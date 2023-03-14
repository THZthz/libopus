/**
 * @file agents.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/12/22
 *
 * @reference https://natureofcode.com/book/chapter-6-autonomous-agents/
 * @example
 *
 * @development_log
 *
 */

#include <stdlib.h>

#include "_/agents.h"
#include "math/math.h"


void agent_update_state(agent_t *agent, agent_steering_t steering, opus_real delta)
{
	opus_real t1, t2;

	agent->pos.x += agent->vel.x * delta + 0.5 * steering.linear.x * delta * delta;
	agent->pos.y += agent->vel.y * delta + 0.5 * steering.linear.y * delta * delta;
	agent->orientation += agent->rotation * delta + 0.5 * steering.angular * delta * delta;

	agent->vel.x += steering.linear.x * delta;
	agent->vel.y += steering.linear.y * delta;
	agent->rotation += steering.angular * delta;

	/* truncate velocity */
	t1 = agent->vel.x * agent->vel.x + agent->vel.y * agent->vel.y;
	t2 = agent->max_vel * agent->max_vel;
	if (t1 > t2) {
		agent->vel = opus_vec2_norm(agent->vel);
		agent->vel = opus_vec2_scale(agent->vel, agent->max_vel);
	}

	agent->orientation = agent_get_new_orientation(agent);
}

/*
 * simply use the direction of velocity as the orientation,
 * but if the agent is still, keep original orientation
 */
opus_real agent_get_new_orientation(agent_t *agent)
{
	if (agent->vel.x * agent->vel.x + agent->vel.y * agent->vel.y < 2 * 0.01 * 0.01) {
		return agent->orientation;
	} else {
		return atan2(agent->vel.y, agent->vel.x);
	}
}

agent_steering_t agent_get_seek_steering(agent_t *agent, opus_real max_speed, opus_vec2 target)
{
	opus_vec2 t;

	agent_steering_t steering;

	t               = opus_vec2_sub(target, agent->pos);
	t               = opus_vec2_norm(t);
	t               = opus_vec2_scale(t, max_speed); /* desired velocity */
	steering.linear = opus_vec2_sub(t, agent->vel);

	steering.angular = 0;

	return steering;
}

agent_steering_t agent_get_flee_steering(agent_t *agent, opus_real max_speed, opus_vec2 target)
{
	opus_vec2 t;

	agent_steering_t steering;

	t               = opus_vec2_sub(agent->pos, target); /* different from seeking behavior */
	t               = opus_vec2_norm(t);
	t               = opus_vec2_scale(t, max_speed); /* desired velocity */
	steering.linear = opus_vec2_sub(t, agent->vel);

	steering.angular = 0;

	return steering;
}

agent_steering_t agent_get_arrive_steering(agent_t *agent, opus_real max_speed, opus_vec2 target)
{
	agent_steering_t steering;

	opus_real d2;
	opus_vec2 t;

	d2 = (agent->pos.x - target.x) * (agent->pos.x - target.x);
	d2 += (agent->pos.y - target.y) * (agent->pos.y - target.y);

	t = opus_vec2_norm(opus_vec2_sub(target, agent->pos));

	if (d2 < AGENT_ARRIVE_RADIUS * AGENT_ARRIVE_RADIUS) {
		opus_real d = opus_sqrt(d2);
		opus_real s = opus_map(d, 0, AGENT_ARRIVE_RADIUS, 0, max_speed);
		t      = opus_vec2_scale(t, s);
	} else {
		t = opus_vec2_scale(t, max_speed); /* proceed with max speed */
	}

	steering.linear = opus_vec2_sub(t, agent->vel);

	steering.angular = 0;

	return steering;
}

/**
 * @brief will not strictly follow the path
 * @param agent
 * @param max_speed
 * @param path
 * @param n
 * @return
 */
agent_steering_t agent_get_path_follow_steering(agent_t *agent, opus_real max_speed, opus_vec2 *path,
                                                unsigned n)
{
	agent_steering_t steering;

	opus_real d, max_d;
	unsigned i;
	opus_vec2 predicted_pos, target;

	predicted_pos = opus_vec2_add(agent->pos, agent->vel);

	max_d = OPUS_REAL_MAX;
	for (i = 0; i < n - 1; i++) {
		opus_vec2 a, b;
		opus_real c_l;

		a = opus_vec2_sub(predicted_pos, path[i]);
		b = opus_vec2_sub(path[i + 1], path[i]);
		b = opus_vec2_norm(b);

		c_l = opus_vec2_dot(a, b);
		b   = opus_vec2_scale(b, c_l);

		b = opus_vec2_add(b, path[i]); /* now "b" is the normal point */

		/* in case we do not find a normal point */
		if (b.x < opus_min(path[i].x, path[i + 1].x)) opus_vec2_copy(&b, path[i]);
		else if (b.x > opus_max(path[i].x, path[i + 1].x))
			opus_vec2_copy(&b, path[i + 1]);

		d = opus_vec2_dist2(b, predicted_pos);
		if (d < max_d * max_d) {
			max_d = opus_sqrt(d);
			opus_vec2_copy(&target, b); /* set new target */
		}
	}

	opus_vec2_set(&steering.linear, 0, 0);
	steering.angular = 0;

	d = opus_vec2_dist2(target, predicted_pos);
	if (d > 10 * 10) { steering = agent_get_seek_steering(agent, max_speed, target); }

	return steering;
}

static opus_vec2 get_separate(agent_t *agent, agent_t **other, unsigned n, opus_real max_speed)
{
	opus_vec2 sum = {0, 0}, ret = {0, 0};
	unsigned i, c              = 0;

	for (i = 0; i < n; i++) {
		opus_real d2;

		if (other[i] == agent || other[i] == NULL) continue;

		d2 = opus_vec2_dist2(agent->pos, other[i]->pos);

		if (d2 < AGENT_SEPARATE_RADIUS * AGENT_SEPARATE_RADIUS) {
			opus_vec2 diff;
			opus_real dd = 1 / opus_sqrt(d2);
			diff    = opus_vec2_sub(agent->pos, other[i]->pos);
			diff    = opus_vec2_norm(diff);
			diff    = opus_vec2_scale(diff, dd);

			sum = opus_vec2_add(sum, diff);
			c++;
		}
	}

	if (c > 0) {
		opus_real dc = 1 / (opus_real) c;
		sum     = opus_vec2_scale(sum, dc);
		sum     = opus_vec2_norm(sum);
		sum     = opus_vec2_scale(sum, max_speed);

		ret = opus_vec2_sub(sum, agent->vel);
	}

	return ret;
}

static opus_vec2 get_align(agent_t *agent, agent_t **other, unsigned n, opus_real max_speed)
{
	unsigned i;
	opus_vec2 sum = {0, 0}, ret;

	for (i = 0; i < n; i++) {
		if (other[i] == agent || other[i] == NULL) continue;
		sum = opus_vec2_add(sum, other[i]->vel);
	}
	sum = opus_vec2_scale(sum, 1. / n);
	opus_vec2_set_length(&sum, max_speed);

	ret = opus_vec2_sub(sum, agent->vel);
	return ret;
}

static opus_vec2 get_cohesion(agent_t *agent, agent_t **other, unsigned n, opus_real max_speed)
{
	opus_vec2 sum = {0, 0};
	unsigned i, c = 0;
	for (i = 0; i < n; i++) {
		opus_real d2;
		if (other[i] == agent || other[i] == NULL) continue;
		d2 = opus_vec2_dist2(agent->pos, other[i]->pos);
		if (d2 < AGENT_COHESION_RADIUS * AGENT_COHESION_RADIUS) {
			sum = opus_vec2_add(sum, other[i]->pos);
			c++;
		}
	}
	if (c > 0) {
		agent_steering_t s;
		opus_real        dc;
		dc  = 1 / (opus_real) c;
		sum = opus_vec2_scale(sum, dc);
		s   = agent_get_seek_steering(agent, max_speed, sum);
		return s.linear;
	} else {
		return opus_vec2_(0, 0);
	}
}

agent_steering_t agent_get_separate_steering(agent_t *agent, agent_t **other, unsigned n,
                                             opus_real max_speed)
{
	agent_steering_t steering;

	steering.linear  = get_separate(agent, other, n, max_speed);
	steering.angular = 0;

	return steering;
}

agent_steering_t agent_get_flock_steering(agent_t *agent, agent_t **other, unsigned n,
                                          opus_real max_speed)
{
	agent_steering_t steering;

	opus_vec2 sum, separate, align, cohesion;

	/* make agent avoid colliding with its neighbors */
	separate = get_separate(agent, other, n, max_speed);

	/* make agent steer in the direction of its neighbors */
	align = get_align(agent, other, n, max_speed);

	/* make agent stay with its neighbors */
	cohesion = get_cohesion(agent, other, n, max_speed);

	sum.x = separate.x * AGENT_SEPARATE_WEIGHT + align.x * AGENT_ALIGN_WEIGHT +
	        cohesion.x * AGENT_COHESION_WEIGHT;
	sum.y = separate.y * AGENT_SEPARATE_WEIGHT + align.y * AGENT_ALIGN_WEIGHT +
	        cohesion.y * AGENT_COHESION_WEIGHT;

	steering.linear  = sum;
	steering.angular = 0;

	return steering;
}
