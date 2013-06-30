#ifndef EVENTS_H
#define EVENTS_H

#include <stddef.h>
#include <stdbool.h>

typedef enum {
	EVENT_TYPE_DEAD = 0,
	EVENT_TYPE_MAX
} EventType;

int event_trigger( EventType type, size_t args_len, char * const args[static args_len], bool async );
void event_trigger_async( EventType type, size_t args_len, char * const args[static args_len] );

#endif
