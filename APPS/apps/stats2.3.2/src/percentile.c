
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


void perc_usage ()
{
    fprintf (stderr,"usage: perc X\n");
    fprintf (stderr,"  returns the Xth percentile of the dataset\n");
}


void find_perc (n)
int n;
{
    double perc, val, findex;
    unsigned long index;
    char label [MAX_LINE];

    perc = (double)n / 100.0;
    findex = num_samples * perc;
    index = (unsigned long)findex;
    if (findex > index)
	index++;
    val = sample [index - 1];
    sprintf (label,"percentile %d", n);
    PRINT_LABEL (label);
    fprintf (out,"%f\n", val);
}


void cmd_perc ()
{
    char *perc;
    int iperc;

    if ((perc = strtok (NULL," ")) == NULL)
    {
	perc_usage ();
	if (script_file)
	    exit (1);
	return;
    }
    iperc = atoi (perc);
    if ((iperc > 0) && (iperc <= 100))
	find_perc (iperc);
    else
    {
	fprintf (stderr,"percentile must be 1-100\n");
	if (script_file)
	    exit (1);
    }
}
