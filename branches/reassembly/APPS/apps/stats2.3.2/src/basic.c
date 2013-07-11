
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
#include <math.h>
#include "stats.h"

unsigned short basic_labels = TRUE;


void basic_usage ()
{
    fprintf (stdout,"usage: basic [variable_list]\n");
    fprintf (stdout," variables:\n");
    fprintf (stdout,"  all, n, sum, min, max, range, mean, median,\n");
    fprintf (stdout,"  meand, variance, standd, gmean, fairness\n");
    if (script_file)
	exit (1);
}


void basic_sum ()
{
    PRINT_LABEL ("sum");
    fprintf (out,"%f\n", sum);
}


void basic_mean ()
{
    PRINT_LABEL ("mean");
    fprintf (out,"%f\n", sum / (double)num_samples);
}


void basic_min ()
{
    PRINT_LABEL ("minimum");
    fprintf (out,"%f\n", sample [0]);
}


void basic_max ()
{
    PRINT_LABEL ("maximum");
    fprintf (out,"%f\n", sample [num_samples - 1]);
}


void basic_range ()
{
    PRINT_LABEL ("range");
    fprintf (out,"%f\n", sample [num_samples - 1] - sample [0]);
}


void basic_n ()
{
    PRINT_LABEL ("N");
    fprintf (out,"%ld\n", num_samples);
}


void basic_median ()
{
    double med;

    if ((num_samples % 2) == 1)
	med = sample [(int)(num_samples / 2)];
    else
	med = (sample [(int)(num_samples / 2)] + 
	       sample [(int)((num_samples / 2) - 1)]) / 2;
    PRINT_LABEL ("median");
    fprintf (out,"%f\n", med);
}


void basic_meand ()
{
    unsigned long i;
    static int flag = 0;
    static double meand = 0;
    double mean;

    if (data_changed > flag)
    {
	meand = 0;
	mean = sum / (double)num_samples;
	for (i = 0; i < num_samples; i++)
	    meand += fabs (sample [i] - mean);
	meand /= (double)num_samples;
	flag = data_changed;
    }
    PRINT_LABEL ("mean deviation");
    fprintf (out,"%f\n", meand);
}


void basic_variance ()
{
    double s2;

    s2 = (1.0 / ((double)num_samples - 1.0)) * 
	(sum_sq - ((sum * sum) / (double)num_samples));
    PRINT_LABEL ("variance");
    fprintf (out,"%f\n", s2);
}


void basic_standd ()
{
    double s2;

    s2 = (1.0 / ((double)num_samples - 1.0)) * 
	(sum_sq - ((sum * sum) / (double)num_samples));
    PRINT_LABEL ("standard deviation");
    fprintf (out,"%f\n", sqrt (s2));
}


void basic_gmean ()
{
    double gmean;

    gmean = pow (prod,(1.0 / (double)num_samples));
    PRINT_LABEL ("geometric mean");
    fprintf (out,"%f\n", gmean);
}


void basic_fairness ()
{
    double f;

    f = (sum * sum) / ((double)num_samples * sum_sq);
    PRINT_LABEL ("fairness index");
    fprintf (out,"%f\n", f);
}


void basic_all ()
{
    basic_n ();
    basic_sum ();
    basic_min ();
    basic_max ();
    basic_range ();
    basic_mean ();
    basic_median ();
    basic_meand ();
    basic_variance ();
    basic_standd ();
    basic_gmean ();
    basic_fairness ();
}


void cmd_basic (line,delim)
char *line, *delim;
{
    char *var = NULL;

    if (num_samples == 0)
    {
	fprintf (stderr,"no samples on which to run statistics\n");
	if (script_file)
	    exit (1);
	return;
    }
    if (interactive)
	fprintf (out,"\n");
    if ((line == NULL) || ((var = strtok (line,delim)) == NULL))
	basic_all ();
    while (var != NULL)
    {
	if (!strcmp (var,"sum"))
	    basic_sum ();
	else if (!strcmp (var,"mean"))
	    basic_mean ();
	else if (!strcmp (var,"range"))
	    basic_range ();
	else if (!strcmp (var,"n"))
	    basic_n ();
	else if (!strcmp (var,"all"))
	    basic_all ();
	else if (!strcmp (var,"median"))
	    basic_median ();
	else if (!strcmp (var,"meand"))
	    basic_meand ();
	else if (!strcmp (var,"standd"))
	    basic_standd ();
	else if (!strcmp (var,"variance"))
	    basic_variance ();
	else if (!strcmp (var,"gmean"))
	    basic_gmean ();
	else if (!strcmp (var,"min"))
	    basic_min ();
	else if (!strcmp (var,"max"))
	    basic_max ();
	else if (!strcmp (var,"fairness"))
	    basic_fairness ();
	else 
	    basic_usage ();
	var = strtok (NULL, " ");
    }
    if (interactive)
	fprintf (out,"\n");
}
