
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


unsigned long partition (a,p,r)
double a [];
unsigned long p, r;
{
    double x, tmp;
    unsigned long i, j;

    x = a [p];
    i = p - 1;
    j = r + 1;
    while (1)
    {
	for (j--; a [j] > x; j--);
	for (i++; a [i] < x; i++);
	if (i < j)
	{
	    tmp = a [i];
	    a [i] = a [j];
	    a [j] = tmp;
	}
	else
	    return (j);
    }
}


void quick_sort (a,p,r)
double a [];
unsigned long p, r;
{
    unsigned long q;

    if (p < r)
    {
	q = partition (a,p,r);
	quick_sort (a,p,q);
	quick_sort (a,q + 1,r);
    }
}
