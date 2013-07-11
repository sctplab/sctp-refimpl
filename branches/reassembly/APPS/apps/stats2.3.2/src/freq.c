
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

/* maybe have a short/long option, as well */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stats.h"


void freqtab_usage ()
{
    fprintf (stderr,"usage: freqtab granularity [histogram]\n");
}


void do_freqtab (gran,hist)
double gran;
char *hist;
{
    unsigned short first = TRUE;
    double current, target;
    unsigned long i = 0, j;
    long skip;
    unsigned long cnt, ticks;
    double weight;
    char buffer [MAX_LINE];

    skip = (long)(sample [i] / gran);
    skip--;
    current = skip * gran;
    target = current + gran;
    if (hist != NULL)
	weight = (double)num_samples / WIDTH;
    while (i < num_samples)
    {
	cnt = 0;
	while ((i < num_samples) && (sample [i] <= target))
	{
	    cnt++;
	    i++;
	}
	if (!first || (first && cnt))
	{
	    sprintf (buffer,"(%0.2f,%0.2f]", current, target);
	    fprintf (out,"%22s ", buffer);
	    fprintf (out,"%7ld ", cnt);
	    sprintf (buffer,"(%0.2f)", 
		     (double)cnt / (double)num_samples * 100.0);
	    fprintf (out,"%8s ", buffer);
	    if (hist != NULL)
	    {
		ticks = (unsigned long)cnt / weight;
		for (j = 0; j < ticks; j++)
		    fprintf (out,"*");
	    }
	    fprintf (out,"\n");
	}
	first = FALSE;
	current = target;
	target += gran;
    }
}


void cmd_freqtab (g,h)
double g;
unsigned short h;
{
    char *gran;
    char * hist;
    double dgran;

    if (num_samples == 0)
    {
	fprintf (stderr,"cannot generate frequency table -- no data\n");
	if (script_file)
	    exit (1);
	return;
    }
    if (!g)
    {
	if ((gran = strtok (NULL," ")) == NULL)
	{
	    freqtab_usage ();
	    if (script_file)
		exit (1);
	    return;
	}
	if ((hist = strtok (NULL," ")) != NULL)
	{
	    if (strcmp (hist,"histogram"))
	    {
		freqtab_usage ();
		if (script_file)
		    exit (1);
		return;
	    }
	    else
		h = TRUE;
	}
	dgran = atof (gran);
    }
    else
	dgran = g;
    do_freqtab (dgran,h);
}
