
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
#include <string.h>
#include "stats.h"

char *script_file = NULL;
FILE *sfp = NULL;

void script_open ()
{
    if ((sfp = fopen (script_file,"r")) == NULL)
    {
	fprintf (stderr,"cannot open script: %s\n", script_file);
	exit (1);
    }
}


void script_close ()
{
    fclose (sfp);
}


void script_proc ()
{
    char cmd [MAX_LINE];

    script_open ();
    while (fgets (cmd,MAX_LINE,sfp) != NULL)
    {
	cmd [strlen (cmd) - 1] = '\0';
	if (cmd [0] != '#')
	    do_cmd (cmd);
    }
    script_close ();
}
