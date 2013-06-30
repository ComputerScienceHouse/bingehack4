#include "events.h"
#include "hack.h"
#include "extern.h"

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/wait.h>

// TODO: Make this a configurable option
#define EVENT_SCRIPT "nethack_event.sh"

static const char * const event_type_strings[EVENT_TYPE_MAX] = {
	[EVENT_TYPE_DEAD] = "EVENT_TYPE_DEAD",
};

int event_trigger( EventType type, size_t args_len, char * const args[static args_len], bool async ) {
	if( type >= EVENT_TYPE_MAX ) impossible("unrecognized event type");

	pid_t pid = fork();
	if( pid == -1 ) {
		if( async ) {
			pline("fork(): %s", strerror(errno));
		} else {
			impossible("fork(): %s", strerror(errno));
		}
	} else if( pid == 0 ) {
		const char *argv[args_len + 3];
		memcpy(argv + 2, args, args_len * sizeof(char *));
		argv[args_len + 2] = NULL;
		argv[0] = EVENT_SCRIPT;
		argv[1] = event_type_strings[type];
		chdir(fqn_prefix[DATAPREFIX]);
		execvp(fqname(EVENT_SCRIPT, DATAPREFIX, DATAPREFIX), (char **) argv);
		char *str = strerror(errno);
		exit(errno == ENOENT ? EXIT_SUCCESS : EXIT_FAILURE); // l'impossible!
	}

	if( async ) return 0;

	int child_status;
	pid_t waitpid_ret = waitpid(pid, &child_status, 0);
	if( waitpid_ret == -1 ) impossible("waitpid(): %s", strerror(errno));
	// Undefined if the process exited due to a signal.
	return WEXITSTATUS(child_status);
}

void event_trigger_async( EventType type, size_t args_len, char * const args[static args_len] ) {
	event_trigger(type, args_len, args, true);
}
