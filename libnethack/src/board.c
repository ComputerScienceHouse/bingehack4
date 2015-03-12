/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
struct nh_board_entry *nh_get_board_entries(char *entries_since, int *length) {
    //The binge board is not available if the user is not connected to the
    //server. They shouldn't even have the menu option to open it.
    *length = 0;
    return NULL;
}
