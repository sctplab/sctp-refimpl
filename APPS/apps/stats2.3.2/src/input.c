
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

unsigned long input_count = 0;


long readdata (filename)
char *filename;
{
    FILE *ifp;
    char line [MAX_LINE];

    if (!strcmp (filename,"-"))
	ifp = stdin;
    else if ((ifp = fopen (filename,"r")) == NULL)
    {
	fprintf (stderr,"could not open %s\n", filename);
	if (script_file)
	    exit (1);
	return (-1);
    }
    while ((fgets (line,MAX_LINE,ifp) != NULL) && (input_count < max_samples))
    {
	sample [input_count] = atof (line);
	sum += sample [input_count];
	sum_sq += (sample [input_count] * sample [input_count]);
	prod *= sample [input_count++];
	if (input_count == max_samples)
	{
	    if ((sample = realloc (sample,(max_samples + MEM_SIZE_INC) *
				   sizeof (double))) == NULL)
	    {
		fprintf (stderr,"out of memory -- cannot read dataset\n");
		input_count = 0;
		sum = sum_sq = 0;
		prod = 1;
		break;
	    }
	    max_samples += MEM_SIZE_INC;
	}
    }
    if (strcmp (filename,"-"))
	fclose (ifp);
    data_changed++;
    return (input_count);
}


void cmd_input ()
{
    long n;
    unsigned long old_num_samples;
    char *filename;

    if ((filename = strtok (NULL," ")) == NULL)
    {
	fprintf (stderr,"usage: input input_file_1 ... [input_file_N]\n");
	if (script_file)
	    exit (1);
	return;
    }
    while (filename != NULL)
    {
	old_num_samples = num_samples;
	if ((n = readdata (filename)) < 0)
	    return;
	num_samples = n;
	if (!silent)
	    fprintf (stdout,"%ld sample(s) read from %s\n", 
		     (num_samples - old_num_samples), filename);
	filename = strtok (NULL," ");
    }
    if (num_samples)
	quick_sort (sample,0,num_samples - 1);
    if (!silent)
	fprintf (stdout,"%ld sample(s) total\n", num_samples);
}


void clear_input_count ()
{
    input_count = 0;
}
