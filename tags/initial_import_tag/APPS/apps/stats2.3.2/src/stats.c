

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
#include <math.h>
#include "stats.h"

unsigned long num_samples = 0, num_base = 0;
unsigned short verbose = FALSE;
unsigned short silent = FALSE;
unsigned short interactive = TRUE;
unsigned long max_samples = MEM_SIZE_INC;
unsigned long max_base = MEM_SIZE_INC;
double *sample, *base;
double sum = 0, sum_sq = 0;
double prod = 1;
unsigned long data_changed = 0;
FILE *out = NULL;

void usage (progname)
char *progname;
{
    fprintf (stderr,"usage: %s [batch_arguments] [-f script_file]\n", 
	     progname);
    fprintf (stderr," batch_arguments:\n");
    fprintf (stderr,"  -b[v1,v2,...] generate basic statistics\n");
    fprintf (stderr,"  -c            generate CDF\n");
    fprintf (stderr,"  -h            usage instructions\n");
    fprintf (stderr,"  -M X          set memory for X data points\n");
    fprintf (stderr,"  -n            don't print labels\n");
    fprintf (stderr,"  -O X          send output to given file (X)\n");
    fprintf (stderr,"  -t[h] G       generate frequency table (and \n");
    fprintf (stderr,"                histogram) with granularity G\n");
    fprintf (stderr,"  -v            version information\n");
    exit (1);
}


void parseargs (argc,argv)
int argc;
char *argv [];
{
    int i;

    if (argc == 1)
	return;
    for (i = 1; i < argc; i++)
    {
	if ((argv [i] [0] == '-') && (strlen (argv [i]) > 1))
	{
	    interactive = FALSE;
	    if (argv [i] [1] == 'c')
		batch_cdf = TRUE;
	    else if (argv [i] [1] == 'n')
		basic_labels = FALSE;
	    else if (argv [i] [1] == 'h')
		usage (argv [0]);
	    else if (argv [i] [1] == 'v')
	    {
		silent = FALSE;
		print_info ();
		exit (0);
	    }
	    else if (argv [i] [1] == 'b')
	    {
		batch_basic = TRUE;
		if (strlen (argv [i]) > 2)
		    batch_basic_args = argv [i] + 2;
	    }
	    else if ((argv [i] [1] == 't') && (i + 1) < argc)
	    {
		batch_freqtab = TRUE;
		if (argv [i] [2] == 'h')
		    freqtab_hist = TRUE;
		freqtab_gran = atof (argv [++i]);
	    }
	    else if (argv [i] [1] == 'O' && (i + 1) < argc)
		batch_outfile = argv [++i];
	    else if (argv [i] [1] == 'f' && (i + 1) < argc)
		script_file = argv [++i];
/*
	    else if (argv [i] [1] == 'M' && (i + 1) < argc)
	    {
		free (sample);
		free (base);
		max_samples = atoi (argv [++i]);
		sample = (double *)malloc (sizeof (double) * max_samples);
		base = (double *)malloc (sizeof (double) * max_samples);
		if ((sample == NULL) || (base == NULL))
		{
		    fprintf (stderr,"can't allocate requested memory\n");
		    exit (1);
		}
	    }
*/
	    else
		usage (argv [0]);
	}
	else if ((num_samples = readdata (argv [i])) < 0)
	    exit (1);
    }
    if (num_samples)
	quick_sort (sample,0,num_samples - 1);
}


int main (argc,argv)
int argc;
char *argv [];
{
    out = stdout;
    sample = (double *)malloc (sizeof (double) * MEM_SIZE_INC);
    base = (double *)malloc (sizeof (double) * MEM_SIZE_INC);
    if ((sample == NULL) || (base == NULL))
    {
	fprintf (stderr,"can't allocate enough memory\n");
	exit (1);
    }
    parseargs (argc,argv);
    if (interactive)
	do_interactive ();
    else if (script_file != NULL)
    {
	silent = TRUE;
	script_proc ();
    }
    else
	batch ();
    exit (0);
}
