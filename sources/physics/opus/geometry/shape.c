/**
* @file shape.c
*     Author:    _              _
*               / \   _ __ ___ (_)  __ _ ___
*              / _ \ | '_ ` _ \| |/ _` / __|
*             / ___ \| | | | | | | (_| \__ \
*            /_/   \_\_| |_| |_|_|\__,_|___/  2023/2/28
*
* @example
*
* @development_log
*
*/

#include "physics/opus/physics.h"


unsigned int OPUS_physics_shape_max_size = sizeof(opus_shape);


void opus_shape_get_max_struct_size_()
{
	OPUS_physics_shape_max_size = opus_max(OPUS_physics_shape_max_size, sizeof(opus_polygon));
	OPUS_physics_shape_max_size = opus_max(OPUS_physics_shape_max_size, sizeof(opus_circle));
}
