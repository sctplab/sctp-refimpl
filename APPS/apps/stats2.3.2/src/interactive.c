
/*
 * Copyright (c) 2001 BBNT Solutions LLC
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice and this permission
 * appear in all copies and in supporting documentation, and that the
 * name of BBN Technologies not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  BBN makes no representations about the
 * suitability of this software for any purposes.  It is provided "AS
 * IS" without express or implied warranties.
 */

#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "stats.h"


void print_info ()
{
    if (silent)
	return;
    fprintf (stdout,"\nstats v%s\n", VERSION);
    fprintf (stdout,"Mark Allman (mallman@ir.bbn.com)\n\n");
}


void do_interactive ()
{
    char *cmd;

    print_info ();
    using_history ();
    while ((cmd = readline (PROMPT)) != NULL)
    {
	if (*cmd)
	{
	    add_history (cmd);
	    do_cmd (cmd);
	}
	free (cmd);
    }
    fprintf (stdout,"\n");
}
