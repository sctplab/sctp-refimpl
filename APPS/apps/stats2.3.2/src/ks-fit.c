
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

#define EPSILON1 0.001
#define EPSILON2 1.0e-8


void ks_dist_fit_usage ()
{
    fprintf (stderr,"usage: ks-dist-fit\n");
    fprintf (stderr," both a normal sample and a base sample must be\n");
    fprintf (stderr," loaded\n");
    if (script_file)
	exit (1);
}


void run_ks (data1,n1,data2,n2,d,pr)
double data1 [];
unsigned long n1;
double data2 [];
unsigned long n2;
double *d, *pr;
{
    unsigned short pr_set = FALSE;
    unsigned long i = 1, j = 1;
    double d1, d2, dt;
    double tmp;
    double pos1 = 0.0, pos2 = 0.0;
    double a1, a2;
    double term, term_last = 0.0;
    float factor = 2.0;

    *d = 0.0;
    while (i <= n1 && j <= n2) 
    {
	d1 = data1 [i];
	d2 = data2 [j];
	if (d1 <= d2)
	    pos1 = i++ / (double)n1;
	if (d2 <= d1)
	    pos2 = j++ / (double)n2;
	if ((dt = fabs (pos2 - pos1)) > *d) 
	    *d = dt;
    }
    tmp = sqrt ((double)((n1 * n2) / (n1 + n2)));
    a1 = ((tmp + 0.12 + 0.11) / tmp) * (*d);
    a2 = a1 * a1 * -2.0;
    for (i = 1; i <= 100; i++)
    {
	term = factor * exp (i * i * a2);
	factor = -factor;
	sum += term;
	if ((fabs (term) <= EPSILON1 * term_last) || 
	    (fabs (term) <= EPSILON2 * sum))
	{
	    pr_set = TRUE;
	    *pr = sum;
	    break;
	}
	term_last = fabs (term);
    }
    if (!pr_set)
	*pr = 1.0;
}


void cmd_ks_dist_fit ()
{
    double d, prob;

    if ((num_samples == 0) || (num_base == 0))
    {
	ks_dist_fit_usage ();
	return;
    }
    run_ks (base,num_base,sample,num_samples,&d,&prob);
    fprintf (out,"     d: %f\n", d);
    fprintf (out,"  prob: %f\n", prob);
}
