
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


void dump_data (where)
FILE *where;
{
    unsigned long i;

    if (where == NULL)
	return;
    for (i = 0; i < num_samples; i++)
	fprintf (where,"%f\n", sample [i]);
}


void cmd_dump ()
{
    FILE *ofp;
    char *outfile;
    unsigned short flag = 0;

    if ((outfile = strtok (NULL," ")) != NULL)
    {
	if ((ofp = fopen (outfile,"w")) == NULL)
	{
	    fprintf (stderr,"cannot open file: %s\n", outfile);
	    if (script_file)
		exit (1);
	    return;
	}
	flag = 1;
    }
    else
	ofp = out;
    dump_data (ofp);
    if (flag)
	fclose (ofp);
}
