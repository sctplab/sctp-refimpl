
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
#include "stats.h"

unsigned short batch_cdf = FALSE;
unsigned short batch_basic = FALSE;
unsigned short batch_freqtab = FALSE;
double freqtab_gran;
unsigned short freqtab_hist = FALSE;
char *batch_outfile = NULL;
char *batch_basic_args = NULL;


void batch ()
{
    unsigned short opts;
    FILE *bout = stdout;

    opts = batch_cdf + batch_basic + batch_freqtab;
    if (!num_samples)
    {
	fprintf (stderr,"no data to crunch\n");
	exit (1);
    }
    if (opts > 1)
    {
	fprintf (stderr,"can't use multiple options (-b, -c, -f) together\n");
	exit (1);
    }
    if (opts < 1)
    {
	fprintf (stderr,"no batch option (-b, -c, -t) given\n");
	exit (1);
    }
    if (batch_outfile)
    {
	if ((bout = fopen (batch_outfile,"w")) == NULL)
	{
	    fprintf (stderr,"can't open output file: %s\n", batch_outfile);
	    exit (1);
	}
	out = bout;
    }
    if (batch_cdf)
	print_cdf (bout);
    else if (batch_basic)
	cmd_basic (batch_basic_args,",");
    else if (batch_freqtab)
	cmd_freqtab (freqtab_gran,freqtab_hist);
    if (batch_outfile)
	fclose (bout);
}
