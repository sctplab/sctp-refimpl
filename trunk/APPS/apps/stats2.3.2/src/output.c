
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

FILE *out_fp = NULL;
unsigned short prev_interactive;

void cmd_open ()
{
    char *filename;

    if ((filename = strtok (NULL," ")) == NULL)
    {
	fprintf (stderr,"usage: output filename\n");
	if (script_file)
	    exit (1);
	return;
    }
    if ((out_fp = fopen (filename,"w")) == NULL)
    {
	fprintf (stderr,"cannot open %s for output\n", filename);
	if (script_file)
	    exit (1);
	return;
    }
    out = out_fp;
    prev_interactive = interactive;
    interactive = FALSE;
}


void cmd_close ()
{
    if (out_fp == NULL)
    {
	fprintf (stderr,"no file open\n");
	if (script_file)
	    exit (1);
	return;
    }
    fclose (out_fp);
    out = stdout;
    interactive = prev_interactive;
}
