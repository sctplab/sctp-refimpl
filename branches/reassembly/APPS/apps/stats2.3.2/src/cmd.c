
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


void cmd_usage (cmd)
char *cmd;
{
    fprintf (stderr,"Invalid command: %s\n", cmd);
    cmd_help ();
    if (script_file)
	exit (1);
}


void do_cmd (input_line)
char *input_line;
{
    char *cmd;

    if (!strcmp (input_line,"exit"))
	exit (0);
    else if (!strcmp (input_line,"clear"))
	cmd_clear ();
    else if (!strcmp (input_line,"help"))
	cmd_help ();
    else if (!strcmp (input_line,"info"))
	print_info ();
    else if (!strcmp (input_line,"silent"))
	cmd_silent ();
    else if (!strcmp (input_line,"close"))
	cmd_close ();
    else
    {
	cmd = strtok (input_line," ");
	if (!strcmp (cmd,"input"))
	    cmd_input ();
	else if (!strcmp (cmd,"base"))
	    cmd_base ();
	else if (!strcmp (cmd,"basic"))
	    cmd_basic (strtok (NULL," ")," ");
	else if (!strcmp (cmd,"dump"))
	    cmd_dump ();
	else if (!strcmp (cmd,"cdf"))
	    cmd_cdf ();
	else if (!strcmp (cmd,"ks-fit"))
	    cmd_ks_dist_fit ();
	else if (!strcmp (cmd,"open"))
	    cmd_open ();
	else if (!strcmp (cmd,"transform"))
	    cmd_transform ();
	else if (!strcmp (cmd,"freqtab"))
	    cmd_freqtab (0,0);
	else if (!strcmp (cmd,"perc"))
	    cmd_perc ();
	else
	    cmd_usage (input_line);
    }
}
