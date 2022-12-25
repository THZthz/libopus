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

#include "game/agents.h"
#include "math/math.h"


void agent_update_state(agent_t *agent, agent_steering_t steering, real delta)
{
	real t1, t2;

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
		vec2_norm(agent->vel, agent->vel);
		vec2_scale(agent->vel, agent->vel, agent->max_vel);
	}

	agent->orientation = agent_get_new_orientation(agent);
}

/*
 * simply use the direction of velocity as the orientation,
 * but if the agent is still, keep original orientation
 */
real agent_get_new_orientation(agent_t *agent)
{
	if (agent->vel.x * agent->vel.x + agent->vel.y * agent->vel.y < 2 * 0.01 * 0.01) {
		return agent->orientation;
	} else {
		return atan2(agent->vel.y, agent->vel.x);
	}
}

agent_steering_t agent_get_seek_steering(agent_t *agent, real max_speed, vec2 target)
{
	vec2 t;

	agent_steering_t steering;

	vec2_sub(t, target, agent->pos);
	vec2_norm(t, t);
	vec2_scale(t, t, max_speed); /* desired velocity */
	vec2_sub(steering.linear, t, agent->vel);

	steering.angular = 0;

	return steering;
}

agent_steering_t agent_get_flee_steering(agent_t *agent, real max_speed, vec2 target)
{
	vec2 t;

	agent_steering_t steering;

	vec2_sub(t, agent->pos, target); /* different from seeking behavior */
	vec2_norm(t, t);
	vec2_scale(t, t, max_speed); /* desired velocity */
	vec2_sub(steering.linear, t, agent->vel);

	steering.angular = 0;

	return steering;
}

agent_steering_t agent_get_arrive_steering(agent_t *agent, real max_speed, vec2 target)
{
	agent_steering_t steering;

	real d2;
	vec2 t;

	d2 = (agent->pos.x - target.x) * (agent->pos.x - target.x);
	d2 += (agent->pos.y - target.y) * (agent->pos.y - target.y);

	vec2_sub(t, target, agent->pos);
	vec2_norm(t, t);

	if (d2 < AGENT_ARRIVE_RADIUS * AGENT_ARRIVE_RADIUS) {
		real d = r_sqrt(d2);
		real s = r_map(d, 0, AGENT_ARRIVE_RADIUS, 0, max_speed);
		vec2_scale(t, t, s);
	} else {
		vec2_scale(t, t, max_speed); /* proceed with max speed */
	}

	vec2_sub(steering.linear, t, agent->vel);

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
agent_steering_t agent_get_path_follow_steering(agent_t *agent, real max_speed, vec2 *path, unsigned n)
{
	agent_steering_t steering;

	real     d, max_d;
	unsigned i;
	vec2     predicted_pos, target;

	vec2_add(predicted_pos, agent->pos, agent->vel);

	max_d = REAL_MAX;
	for (i = 0; i < n - 1; i++) {
		vec2 a, b;
		real c_l;

		vec2_sub(a, predicted_pos, path[i]);
		vec2_sub(b, path[i + 1], path[i]);
		vec2_norm(b, b);

		c_l = vec2_dot(a, b);
		vec2_scale(b, b, c_l);

		vec2_add(b, b, path[i]); /* now "b" is the normal point */

		/* in case we do not find a normal point */
		if (b.x < r_min(path[i].x, path[i + 1].x)) vec2_copy(b, path[i]);
		else if (b.x > r_max(path[i].x, path[i + 1].x))
			vec2_copy(b, path[i + 1]);

		d = vec2_dist2(b, predicted_pos);
		if (d < max_d * max_d) {
			max_d = r_sqrt(d);
			vec2_copy(target, b); /* set new target */
		}
	}

	vec2_set(steering.linear, 0, 0);
	steering.angular = 0;

	d = vec2_dist2(target, predicted_pos);
	if (d > 10 * 10) {
		steering = agent_get_seek_steering(agent, max_speed, target);
	}

	return steering;
}

static vec2 get_separate(agent_t *agent, agent_t **other, unsigned n, real max_speed)
{
	vec2     sum = {0, 0}, ret = {0, 0};
	unsigned i, c              = 0;

	for (i = 0; i < n; i++) {
		real d2;

		if (other[i] == agent || other[i] == NULL) continue;

		d2 = vec2_dist2(agent->pos, other[i]->pos);

		if (d2 < AGENT_SEPARATE_RADIUS * AGENT_SEPARATE_RADIUS) {
			vec2 diff;
			real dd = 1 / r_sqrt(d2);
			vec2_sub(diff, agent->pos, other[i]->pos);
			vec2_norm(diff, diff);
			vec2_scale(diff, diff, dd);

			vec2_add(sum, sum, diff);
			c++;
		}
	}

	if (c > 0) {
		real dc = 1 / (real) c;
		vec2_scale(sum, sum, dc);
		vec2_norm(sum, sum);
		vec2_scale(sum, sum, max_speed);

		vec2_sub(ret, sum, agent->vel);
	}

	return ret;
}

static vec2 get_align(agent_t *agent, agent_t **other, unsigned n, real max_speed)
{
	unsigned i;
	vec2     sum = {0, 0}, ret;
	real     dn  = 1 / (real) n;

	for (i = 0; i < n; i++) {
		if (other[i] == agent || other[i] == NULL) continue;
		vec2_add(sum, sum, other[i]->vel);
	}
	vec2_scale(sum, sum, dn);
	vec2_set_length(sum, max_speed);

	vec2_sub(ret, sum, agent->vel);
	return ret;
}

static vec2 get_cohesion(agent_t *agent, agent_t **other, unsigned n, real max_speed)
{
	vec2     sum = {0, 0};
	unsigned i, c = 0;
	for (i = 0; i < n; i++) {
		real d2;
		if (other[i] == agent || other[i] == NULL) continue;
		d2 = vec2_dist2(agent->pos, other[i]->pos);
		if (d2 < AGENT_COHESION_RADIUS * AGENT_COHESION_RADIUS) {
			vec2_add(sum, sum, other[i]->pos);
			c++;
		}
	}
	if (c > 0) {
		agent_steering_t s;
		real             dc = 1 / (real) c;
		vec2_scale(sum, sum, dc);
		s = agent_get_seek_steering(agent, max_speed, sum);
		return s.linear;
	} else {
		return vec2_(0, 0);
	}
}

agent_steering_t agent_get_separate_steering(agent_t *agent, agent_t **other, unsigned n, real max_speed)
{
	agent_steering_t steering;

	steering.linear  = get_separate(agent, other, n, max_speed);
	steering.angular = 0;

	return steering;
}

agent_steering_t agent_get_flock_steering(agent_t *agent, agent_t **other, unsigned n, real max_speed)
{
	agent_steering_t steering;

	vec2 sum, separate, align, cohesion;

	/* make agent avoid colliding with its neighbors */
	separate = get_separate(agent, other, n, max_speed);

	/* make agent steer in the direction of its neighbors */
	align = get_align(agent, other, n, max_speed);

	/* make agent stay with its neighbors */
	cohesion = get_cohesion(agent, other, n, max_speed);

	sum.x = separate.x * AGENT_SEPARATE_WEIGHT + align.x * AGENT_ALIGN_WEIGHT + cohesion.x * AGENT_COHESION_WEIGHT;
	sum.y = separate.y * AGENT_SEPARATE_WEIGHT + align.y * AGENT_ALIGN_WEIGHT + cohesion.y * AGENT_COHESION_WEIGHT;

	steering.linear  = sum;
	steering.angular = 0;

	return steering;
}
