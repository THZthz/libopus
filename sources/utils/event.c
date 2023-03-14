#include <stdint.h>

#include "utils/event.h"
#include "data_structure/array.h"
#include "utils/utils.h"
#include "utils/slre.h"

char event_namespace_splitter[EVENT_NAMESPACE_SPLITTER_SIZE + 1] = "/";
char event_events_splitter[EVENT_EVENTS_SPLITTER_SIZE + 1]       = ";";

static void event_namespace_dump_table_(const void *ele, char *text, uint64_t n)
{
	sprintf(text, "%s", ((event_namespace_t *) ele)->name);
}

static void dump(opus_hashmap *map, opus_hashmap_print_data_cb cb)
{
	opus_hashmap_dump(map, stdout, cb);
}

static int event_compare_(opus_hashmap *map, const void *a, const void *b, void *user_data)
{
	const event_t *ea = a, *eb = b;
	return strcmp(ea->name, eb->name);
}

static uint64_t event_hash_(opus_hashmap *map, const void *ele, uint64_t seed0, uint64_t seed1, void *user_data)
{
	const event_namespace_t *e = ele;
	return opus_hashmap_murmur(e->name, strlen(e->name), seed0, seed1);
}

static void event_free_(opus_hashmap *map, const void *ele, void *user_data)
{
}

static event_namespace_t *event_namespace_init_(event_namespace_t *namespace, const char *name)
{
	if (namespace == NULL) return NULL;
	namespace->table = opus_hashmap_create(sizeof(event_t), 0, 0, 0, event_compare_, event_hash_, event_free_);

	namespace->context    = NULL;
	namespace->identifier = EVENT_IS_NAMESPACE;
	strncpy(namespace->name, name, EVENT_NAME_SIZE);
	return namespace;
}

static event_namespace_t *event_namespace_create_(const char *name)
{
	event_namespace_t *namespace = (event_namespace_t *) OPUS_MALLOC(sizeof(event_namespace_t));
	namespace                    = event_namespace_init_(namespace, name);
	return namespace;
}

static int event_namespace_done_(event_namespace_t *namespace)
{
	if (namespace->table->user_data != NULL) {
		event_destroy(namespace->table->user_data);
	}
	if (namespace->table->buckets_used > 0) {
		size_t i;
		size_t done             = 0,
		       buckets_used     = namespace->table->buckets_used,
		       buckets_capacity = namespace->table->buckets_capacity;
		for (i = 0; done < buckets_used && i < buckets_capacity; i++) {
			unsigned int       psl;
			event_namespace_t *ele;
			opus_hashmap_get_bucket_info(namespace->table, i, &psl, (void **) &ele);
			if (psl != 0) {
				if (ele != NULL) {
					if (ele->identifier == EVENT_IS_NAMESPACE)
						event_namespace_done_(ele);
					else
						opus_arr_destroy(((event_t *) ele)->callback_list);
				}
				done++;
			}
		}
	}
	opus_hashmap_destroy(namespace->table);
	return 0;
}

static void event_namespace_destroy_(event_namespace_t *namespace)
{
	OPUS_NOT_NULL(namespace);
	event_namespace_done_(namespace);
	OPUS_FREE(namespace);
}

static event_namespace_t *event_namespace_search_(event_namespace_t *namespace, const char *name)
{
	char              *token;
	char              *name_cp = (char *) OPUS_MALLOC(sizeof(char) * (strlen(name) + 1));
	event_namespace_t *res     = NULL;
	memcpy(name_cp, name, sizeof(char) * (strlen(name) + 1));

	token = strtok(name_cp, event_namespace_splitter);

	while (token != NULL) {
		event_namespace_t key, *res_namespace;
		strcpy(key.name, token);
		res_namespace = opus_hashmap_retrieve(namespace->table, &key);
		if (res_namespace == NULL) break;
		if (res_namespace->identifier != EVENT_IS_NAMESPACE) {
			res = res_namespace;
			break;
		}
		namespace = res_namespace; /* continue search the next namespace */

		token = strtok(NULL, event_namespace_splitter);
	}

	OPUS_FREE(name_cp);
	return res;
}

event_hub_t *event_hub_init(event_hub_t *hub)
{
	return event_namespace_init_(hub, EVENT_MASTER_NAME);
}

event_hub_t *event_hub_create()
{
	return event_namespace_create_(EVENT_MASTER_NAME);
}

/**
 * @brief
 * @param hub
 * @return 0 if success
 */
int event_hub_done(event_hub_t *hub)
{
	return event_namespace_done_(hub);
}

void event_hub_destroy(event_hub_t *hub)
{
	event_namespace_destroy_(hub);
}

event_t *event_hub_search(event_hub_t *hub, char *name)
{
	return NULL;
}

/**
 * @brief register an event on an event_hub to make the event able to be emitted later
 * @param hub
 * @param event
 * @return 0 if success
 */
int event_hub_on(event_hub_t *hub, const char *name, event_t *event)
{
	char *token;
	char  name_cp[EVENT_MAX_EVENT_NAME_SIZE];

	event_namespace_t *cur = hub, *next = NULL, key;
	memcpy(name_cp, name, sizeof(char) * (strlen(name) + 1));
	token = strtok(name_cp, event_namespace_splitter);

	while (token != NULL) {
		char *next_token;
		strcpy(key.name, token);
		next       = opus_hashmap_retrieve(cur->table, &key);
		next_token = strtok(NULL, event_namespace_splitter);
		if (next == NULL) {
			/* add a new namespace and continue process */
			event_namespace_init_(&key, token);
			opus_hashmap_insert(cur->table, &key);
			if (next_token == NULL) {
				/* we reach the name of the event, however the event does not exist */
				/* so create a new event */
				event_t *temp        = (event_t *) OPUS_MALLOC(sizeof(event_t));
				key.table->user_data = temp;
				memcpy(temp, event, sizeof(event_t));
				memcpy(temp->name, token, sizeof(char) * EVENT_NAME_SIZE);
				break;
			}
			cur = opus_hashmap_retrieve(cur->table, &key);
		} else {
			if (next_token == NULL) {
				if (next->table->user_data != NULL) {
					event_t *e = next->table->user_data;
					opus_arr_concat(e->callback_list, event->callback_list);
					opus_arr_destroy(event->callback_list);
				} else {
					event_t *temp          = (event_t *) OPUS_MALLOC(sizeof(event_t));
					next->table->user_data = temp;
					memcpy(temp, event, sizeof(event_t));
					memcpy(temp->name, token, sizeof(char) * EVENT_NAME_SIZE);
				}
				break;
			} else {
				cur = next;
			}
		}
		token = next_token;
	}
	OPUS_FREE(event);
	return 0;
}

/**
 * @brief
 * @param hub
 * @param event_names
 * @param callback return -1 to interrupt subsequent events' processing,
 * 		return 0 for normal cases.
 * @return
 */
static int event_hub_foreach_events_by_name(event_hub_t *hub, const char *event_names, event_cb callback, void *args, int is_nested)
{
	event_namespace_t *cur;
	char               events[EVENT_MAX_EVENT_NAME_SIZE], *event;
	int                events_count     = 0;
	int                is_pointer_event = 0;
	strcpy(events, "master/");
	memcpy(events + 7, event_names, sizeof(char) * (strlen(event_names) + 1));

	event = strtok(events, event_events_splitter);
	while (event != NULL) {
		event = strtok(NULL, event_events_splitter);
		events_count++;
	}

	event = events;
	while (event != NULL && events_count-- > 0) { /* loop through all events */
		int  is_name_match = 0;
		int  is_last_name;
		char names[EVENT_MAX_EVENT_NAME_SIZE], *name, *next_name;
		memcpy(names, event, sizeof(char) * (strlen(event) + 1));

		cur  = hub;
		name = strtok(names, event_namespace_splitter);
		while (name != NULL) { /* loop through all namespaces */
			next_name    = strtok(NULL, event_namespace_splitter);
			is_last_name = next_name == NULL;
			if (is_name_match != -1) is_name_match = strcmp(cur->name, name) == 0;
			else
				is_name_match = 1;

			/* if we get a wildcard matching */
		WILDCARD_MATCHING:
			if (name[0] == '*' /*&& name[1] == '\0'*/) {
				if (is_last_name) {
					size_t i;
					int    j;
					if (cur->table->user_data != NULL) {
						if (callback(hub, cur->table->user_data, args) == EVENT_INTERRUPT) return EVENT_INTERRUPT;
					}

					break; /* last name, and there is no more namespaces to cope with */
				} else {
					if (!is_nested) {
						size_t i;
						int    j;
					CALL_RECURSIVE_FUNC_TO_HANDLE:
						j = (int) cur->table->buckets_used;
						for (i = 0; j > 0 && i < cur->table->buckets_capacity; i++) {
							unsigned int       psl;
							event_namespace_t *ele;
							opus_hashmap_get_bucket_info(cur->table, i, &psl, (void **) &ele);
							if (psl > 0) {
								j--;
								if (event_hub_foreach_events_by_name(ele, event + (name - names), callback, args, 1) == EVENT_INTERRUPT) return EVENT_INTERRUPT;
							}
						}
						break; /* we have passed the processing work to recursive functions, so we just leave */
					} else {
						/* we are in nested mode, namely we have "*" to match the name of "cur"'s name */
						if (next_name[0] == '*') {
							name = next_name;
							goto CALL_RECURSIVE_FUNC_TO_HANDLE;
						}
					}
				}
				/* fall through, NOTICE */
			}

			if (is_last_name) {
				if (is_name_match) {
					if (cur->identifier == EVENT_IS_NAMESPACE && cur->table->user_data != NULL) {
						if (callback(hub, cur->table->user_data, args) == EVENT_INTERRUPT) return EVENT_INTERRUPT;
					}
				}
				break;
			} else {
				if (is_name_match) {
					event_namespace_t *res, key;
					strcpy(key.name, next_name);
					res = opus_hashmap_retrieve(cur->table, &key);
					if (res != NULL) {
						cur           = res;
						is_name_match = -1; /* do not check again */
					}
				} else {
					break;
				}
			}

			name = next_name;
		}

		event += strlen(event) + 1;
	}

	return 0;
}

static int event_hub_emit_callback_(event_hub_t *hub, event_t *e, void *args)
{
	size_t i;
	for (i = 0; i < opus_arr_len(e->callback_list); i++)
		if (e->times != 0 && e->callback_list[i](hub, e, args) == EVENT_INTERRUPT) break;
	if (e->times > 0) e->times--; /* decrease emission counter */
	return 0;
}

static int event_hub_remove_event_by_name_callback_(event_hub_t *hub, event_t *e, void *args)
{
	event_t **events = (event_t **) args;
	opus_arr_push(events, &e);
	return 0;
}

/**
 * @brief If you want to emit multiple events, use event_hub_emit(hub, "event_1;event_2;event_3", args).
 * 		When the counter ("times") reaches zero, the event is destroyed and you can never fetch it back again.
 * 		The events(with the same name) are emitted normally by the time you created it,
 * 			but if you use regexp in emitting events, it can hardly be predicted.
 * 		Use "*" to match any name.
 * @param hub
 * @param event_names
 * @param args
 * @return 0 if success
 */
int event_hub_emit(event_hub_t *hub, const char *event_names, void *args)
{
	return event_hub_foreach_events_by_name(hub, event_names, event_hub_emit_callback_, args, 0);
}

int event_hub_remove_event_by_name(event_hub_t *hub, const char *name)
{
}

int event_hub_remove_event(event_hub_t *hub, event_t *e)
{
}

int event_hub_remove_callback_from_event_by_name(event_hub_t *hub, const char *name, event_cb callback_to_remove)
{
}

/**
 * @brief Create an event, specify its name and its context.
 *		Although it will request memory from system, its memory recycle is handled by event_hub when
 *		you call "event_hub_destroy" or "event_hub_done".
 * 		Usually used together with "event_hub_on", for example,
 * 			event_hub_on(hub, event_create("event_name", NULL, 0, -1, NULL));
 * @param name the name of you can call the id of the event
 * @param callback_list
 * @param len length of the callback_list
 * @param times if specified as -1, it will able to be emitted infinity times, otherwise its emission times is limited
 * @param context
 * @return 0 if success
 */
event_t *event_create(event_cb *callback_list, int len, int times, void *context)
{
	event_t *event = (event_t *) OPUS_MALLOC(sizeof(event_t));
	if (event == NULL) return NULL;

	opus_arr_create(event->callback_list, sizeof(event_cb *));

	/* no space for callback list, no need to create this event */
	if (event->callback_list == NULL) {
		OPUS_FREE(event);
		return NULL;
	} else {
		int i;
		opus_arr_resize(event->callback_list, len);
		for (i = 0; i < len; i++) event->callback_list[i] = callback_list[i];
	}

	event->name[0] = '\0'; /* initialize this later */
	event->times   = times;
	event->context = context;

	return event;
}

void event_destroy(event_t *event)
{
	opus_arr_destroy(event->callback_list);
	OPUS_FREE(event);
}
