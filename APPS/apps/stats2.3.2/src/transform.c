
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
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "stats.h"

double scale_factor = 0;


void transform_usage ()
{
    fprintf (stderr,"usage: transform func\n");
    fprintf (stderr," functions:\n");
    fprintf (stderr,"   loge     natural log\n");
    fprintf (stderr,"   log10    log base 10\n");
    fprintf (stderr,"   scale X  scale by given factor\n");
}


void do_transform (by)
unsigned short by;
{
    unsigned long i;

    sum = sum_sq = 0;
    prod = 1;
    for (i = 0; i < num_samples; i++)
    {
	if (by == LOGE)
	    sample [i] = log (sample [i]);
	else if (by == LOG10)
	    sample [i] = log10 (sample [i]);
	else if (by == SCALE)
	    sample [i] *= scale_factor;
	sum += sample [i];
	sum_sq += (sample [i] * sample [i]);
	prod *= sample [i];
    }
    data_changed++;
}

void cmd_transform ()
{
    char *func, *arg;
    unsigned short transform_by;

    if ((func = strtok (NULL," ")) == NULL)
    {
	transform_usage ();
	if (script_file)
	    exit (1);
	return;
    }
    if (!strcmp (func,"loge"))
	transform_by = LOGE;
    else if (!strcmp (func,"log10"))
	transform_by = LOG10;
    else if (!strcmp (func,"scale"))
    {
	if ((arg = strtok (NULL," ")) == NULL)
	{
	    transform_usage ();
	    if (script_file)
		exit (1);
	    return;
	}
	transform_by = SCALE;
	scale_factor = atof (arg);
    }
    else
    {
	transform_usage ();
	if (script_file)
	    exit (1);
	return;
    }
    do_transform (transform_by);
}
