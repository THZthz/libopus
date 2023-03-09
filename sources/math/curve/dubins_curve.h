/*
 * Created by amias on 2022/2/17.
 */
/**
 * @brief dubins curve
 * 		- Dubins Curve is the shortest path on 2d plane to connect two points(do not consider obstacles) under the condition of
 * 		    a certain curvature and a designated tangent direction of start and end.
 * 		- Also notice that this assumes the vehicle could only drive forward(S), left(L) and right(R).
 * 		    If the vehicle could drive backward, that is Reeds-Shepp Curve.
 */

#ifndef DUBINS_H
#define DUBINS_H

#include "math/math.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum opus_dubins_type {
	LSL = 1,
	LSR = 2,
	RSL = 3,
	RSR = 4,
	RLR = 5,
	LRL = 6
} opus_dubins_type_t;

#define EDUBOK (0)     /* No error */
#define EDUBNOPATH (4) /* no connection between configurations with this word */

typedef struct opus_dubins_path {
	opus_real          start_vec[3];
	opus_real          end_vec[3];
	opus_real          seg_lengths[3]; /* length of each seg */
	opus_real          turn_radius;
	opus_dubins_type_t type;
} opus_dubins_path_t;

/**
 * @brief calculate everything you need for a dubins' path
 * @param param
 * @param wpt1 start pose
 * @param wpt2 end pose
 * @param r the maximum radius of turning circle
 * @return
 */
int opus_dubins_calc(opus_dubins_path_t *param, opus_vec3 wpt1, opus_vec3 wpt2, float r);
/**
 * @brief
 * @param path
 * @param step
 * @return a dynamic array, remember to use "array_destroy" to release its resources
 */
opus_vec3 *opus_dubins_build_trajectory(opus_dubins_path_t *path, opus_real step);
opus_real  opus_dubins_get_length(opus_dubins_path_t *path);
void       opus_dubins_get_point(opus_dubins_path_t *path, opus_real t, opus_vec3 *end_pt);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* DUBINS_H */
