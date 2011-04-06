/*	SCCS Id: @(#)pctty.c	3.4	1990/22/02
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* tty.c - (PC) version */

#include "hack.h"
#include "wintty.h"

char erase_char, kill_char;

/*
 * Get initial state of terminal, set ospeed (for termcap routines)
 * and switch off tab expansion if necessary.
 * Called by startup() in termcap.c and after returning from ! or ^Z
 */
void gettty(void)
{
	erase_char = '\b';
	kill_char = 21;		/* cntl-U */
	iflags.cbreak = TRUE;
	disable_ctrlP();	/* turn off ^P processing */
}

/* reset terminal to original state */
void settty(const char *s)
{
	end_screen();
	if(s) raw_print(s);
	enable_ctrlP();		/* turn on ^P processing */
}

/* called by init_nhwindows() and resume_nhwindows() */
void setftty(void)
{
	start_screen();
}

#if defined(TIMED_DELAY) && defined(_MSC_VER)
void msleep(unsigned mseconds)
{
	/* now uses clock() which is ANSI C */
	clock_t goal;

	goal = mseconds + clock();
	while ( goal > clock()) {
	    /* do nothing */
	}
}
#endif

/* fatal error */
/*VARARGS1*/

void error (const char *s, ...)
{
	va_list the_args;
	va_start(the_args, s);
	/* error() may get called before tty is initialized */
	if (iflags.window_inited) end_screen();
	putchar('\n');
	vprintf(s, the_args);
	putchar('\n');
	va_end(the_args);
	exit(EXIT_FAILURE);
}

/*pctty.c*/
