/**
 * @file agents.h
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/12/22
 *
 * @brief In the late 1980s, computer scientist Craig Reynolds developed algorithmic steering behaviors for animated characters.
 * 		These behaviors allowed individual elements to navigate their digital environments in a “lifelike” manner with strategies for
 * 		fleeing, wandering, arriving, pursuing, evading, etc. Used in the case of a single autonomous agent, these behaviors are
 * 		fairly simple to understand and implement. In addition, by building a system of multiple characters that steer themselves
 * 		according to simple, locally based rules, surprising levels of complexity emerge. The most famous example is
 * 		Reynolds’s “boids” model for “flocking/swarming” behavior.
 *
 * @example
 *
 * @development_log
 *
 */
#ifndef AGENTS_H
#define AGENTS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <math.h>

#include "math/math.h"

#define AGENT_ARRIVE_RADIUS (100)
#define AGENT_SEPARATE_RADIUS (30)
#define AGENT_COHESION_RADIUS (100)
#define AGENT_SEPARATE_WEIGHT (1.5)
#define AGENT_COHESION_WEIGHT (1.0)
#define AGENT_ALIGN_WEIGHT (1.0)

typedef struct agent          agent_t;
typedef struct agent_steering agent_steering_t;

struct agent {
	vec2 pos, vel;
	real rotation;
	real orientation;
	real max_vel;
};

struct agent_steering {
	vec2 linear;
	real angular;
};

agent_steering_t agent_get_seek_steering(agent_t *agent, real max_speed, vec2 target);
agent_steering_t agent_get_flee_steering(agent_t *agent, real max_speed, vec2 target);
agent_steering_t agent_get_arrive_steering(agent_t *agent, real max_speed, vec2 target);
agent_steering_t agent_get_path_follow_steering(agent_t *agent, real max_speed, vec2 *path, unsigned n);
agent_steering_t agent_get_separate_steering(agent_t *agent, agent_t **other, unsigned n, real max_speed);
agent_steering_t agent_get_flock_steering(agent_t *agent, agent_t **other, unsigned n, real max_speed);

real agent_get_new_orientation(agent_t *agent);
void agent_update_state(agent_t *agent, agent_steering_t steering, real delta);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* AGENTS_H */
