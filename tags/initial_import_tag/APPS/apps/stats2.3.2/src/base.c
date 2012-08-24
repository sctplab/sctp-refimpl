
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

unsigned long base_count = 0;


long readbase (filename)
char *filename;
{
    FILE *ifp;
    char line [MAX_LINE];

    if ((ifp = fopen (filename,"r")) == NULL)
    {
	fprintf (stderr,"could not open %s\n", filename);
	if (script_file)
	    exit (1);
	return (-1);
    }
    while ((fgets (line,MAX_LINE,ifp) != NULL) && (base_count < max_base))
    {
	base [base_count++] = atof (line);
	if (base_count == max_base)
	{
	    if ((base = realloc (base, (max_base + MEM_SIZE_INC) * 
				 sizeof (double))) == NULL)
	    {
		fprintf (stderr,"out of memory -- cannot read dataset\n");
		base_count = 0;
		break;
	    }
	    max_base += MEM_SIZE_INC;
	}
    }
    fclose (ifp);
    return (base_count);
}


void cmd_base ()
{
    long n;
    unsigned long old_num_base;
    char *filename;

    if ((filename = strtok (NULL," ")) == NULL)
    {
	fprintf (stderr,"usage: base input_file_1 ... [input_file_N]\n");
	if (script_file)
	    exit (1);
	return;
    }
    while (filename != NULL)
    {
	old_num_base = num_base;
	if ((n = readbase (filename)) < 0)
	    return;
	num_base = n;
	if (!silent)
	    fprintf (stdout,"%ld sample(s) read from %s\n", 
		     (num_base - old_num_base), filename);
	filename = strtok (NULL," ");
    }
    if (num_base)
	quick_sort (base,0,num_base - 1);
    if (!silent)
	fprintf (stdout,"%ld sample(s) total\n", num_base);
}


void clear_base_count ()
{
    base_count = 0;
}
