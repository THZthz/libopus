/**
 * @file neat.h
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/10/19
 *
 * @example
 *
 * @development_log
 *
 */
#ifndef NEAT_H
#define NEAT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "brain/ann.h"

void neat_mutate(ann_t *net);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* NEAT_H */
