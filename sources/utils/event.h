/**
 * @file event.h
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/8/25
 *
 * @example
 *
 * event_hub_t     *hub = event_hub_create();
 * event_callback_t arr[10];
 * arr[0] = on1;
 * event_hub_on(hub, event_create("hello_on1", arr, 1, 1, "context of on1"));
 * arr[0] = on;
 * event_hub_on(hub, event_create("hello_*", arr, 1, -1, "context of hello_*"));
 *
 * event_hub_emit(hub, "hello_on1", "text on1");
 * event_hub_emit(hub, "hello_on1", "text on");
 * event_hub_destroy(hub);
 *
 * @development_log
 *
 */
#ifndef EVENT_H
#define EVENT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <limits.h>
#include "data_structure/hashmap.h"

#define EVENT_MASTER_NAME "master"
#define EVENT_IS_NAMESPACE INT8_MIN
#define EVENT_NAME_SIZE (64) /* the size of single name, e.g. "event_a" in "master/event_a" is event_name */
#define EVENT_NAMESPACE_SPLITTER_SIZE (1)
#define EVENT_EVENTS_SPLITTER_SIZE (1)
#define EVENT_MAX_EVENT_NAME_SIZE (1024) /* the total size of an event, e.g. "master/event_a" is a whole event name */
#define EVENT_STAY (-2)                  /* enable the event to be triggered twice if it is emitted */
#define EVENT_INTERRUPT (-1)             /* interrupt event processing and all callbacks after this won't be able to execute */
#define EVENT_OK (0)                     /* continue event processing normally */

typedef struct event           event_t;
typedef struct event_namespace event_namespace_t;
typedef event_namespace_t      event_hub_t;
typedef int (*event_cb)(event_hub_t *event_hub, event_t *event, void *args);

struct event {
	char              name[EVENT_NAME_SIZE]; /* name size include char '\0' */
	int               times;
	event_cb         *callback_list; /* created by array_create */
	void             *context;
};
struct event_namespace {
	char       name[EVENT_NAME_SIZE];
	int        identifier;
	opus_hashmap *table;
	void      *context;
};

event_hub_t *event_hub_init(event_hub_t *hub);
int          event_hub_done(event_hub_t *hub);
event_hub_t *event_hub_create();
void         event_hub_destroy(event_hub_t *hub);
event_t     *event_hub_search(event_hub_t *hub, char *name);
int          event_hub_on(event_hub_t *hub, const char *name, event_t *event);
int          event_hub_emit(event_hub_t *hub, const char *event_names, void *args);
int          event_hub_remove_event_by_name(event_hub_t *hub, const char *name);
int          event_hub_remove_event(event_hub_t *hub, event_t *e);
int          event_hub_remove_callback_from_event_by_name(event_hub_t *hub, const char *name, event_cb callback_to_remove);
event_t     *event_create(event_cb *callback_list, int len, int times, void *context);
void         event_destroy(event_t *event);

event_t *event_create(event_cb *callback_list, int len, int times, void *context);
void     event_destroy(event_t *event);

extern char event_namespace_splitter[EVENT_NAMESPACE_SPLITTER_SIZE + 1];
extern char event_events_splitter[EVENT_EVENTS_SPLITTER_SIZE + 1];

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* EVENT_H */
