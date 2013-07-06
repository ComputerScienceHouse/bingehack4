#!/bin/sh
event="$1"
shift
cd nethack_events
exec ./"${event}" "$@"
