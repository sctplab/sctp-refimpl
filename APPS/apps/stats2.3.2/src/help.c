
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
#include "stats.h"

#define STATS_CMDS "Commands: \n\
  base file            designate a dataset as the base set for ks test\n\
  basic [vars]         give basic statistics about the sample\n\
  cdf file             output a cumulative distribution of the sample\n\
  clear                clear the current sample\n\
  close                close an open output file\n\
  dump file            output the sample\n\
  freqtab gran \\       output a frequency table with the given granularity\n\
    [histogram]\n\
  exit                 quit the program\n\
  info                 information about the program\n\
  input f1 f2 ...      input the given data files\n\
  ks-fit               perform K-S test between data and base data set\n\
  open file            send all output to the give file\n\
  perc x               report the x-th percentile of the sample\n\
  silent               turn off feedback from the program (except when\n\
                       directly requested)\n\
  transform func       transform data by applying the given function\n"


void cmd_help ()
{
    fprintf (stdout,"\n%s\n", STATS_CMDS);
}
