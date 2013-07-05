#!/bin/sh
event="$1"
shift
exec nethack_events/${event} "$@"
