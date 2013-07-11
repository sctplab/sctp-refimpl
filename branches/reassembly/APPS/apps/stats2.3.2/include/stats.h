
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

#define FALSE                  0
#define TRUE                   1
#define MEM_SIZE_INC           50000
#define MAX_LINE               80
#define DEFAULT_FT_GRAN        100
#define WIDTH                  40

#define PROMPT                 "STATS> "

#define LOGE                   1
#define LOG10                  2
#define SCALE                  3
#define INCR                   4
#define POWER                  5

#define PRINT_LABEL(x) if (basic_labels) \
		fprintf (out,"%20s: ", x)

extern unsigned long num_samples;
extern unsigned long num_base;
extern unsigned short verbose;
extern unsigned short silent;
extern unsigned short interactive;
extern double *sample;
extern double *base;
extern unsigned long max_samples;
extern unsigned long max_base;
extern double sum, sum_sq;
extern double prod;
extern unsigned long data_changed;
extern FILE *out;

extern unsigned short batch_cdf;
extern unsigned short batch_basic;
extern unsigned short batch_freqtab;
extern double freqtab_gran;
extern unsigned short freqtab_hist;
extern char *batch_outfile;

extern char *script_file;

/* base.c */

void cmd_base ();
long readbase ();
void clear_base_count ();

/* basic.c */

unsigned short basic_labels;

void basic_usage ();
void basic_sum ();
void basic_mean ();
void basic_range ();
void basic_all ();
void basic_n ();
void basic_median ();
void basic_meand ();
void basic_variance ();
void basic_standd ();
void basic_gmean ();
void basic_min ();
void basic_max ();
void basic_fairness ();
void cmd_basic ();

/* batch.c */

char *batch_basic_args;

void batch ();

/* cdf.c */

void print_cdf ();
void cmd_cdf ();

/* clear.c */

void cmd_clear ();

/* cmd.c */

void do_cmd ();

/* dump.c */

void dump_data ();
void cmd_dump ();

/* freq.c */

void cmd_freqtab ();

/* help.c */

void cmd_help ();

/* input.c */

long readdata ();
void cmd_input ();
void clear_input_count ();

/* interactive.c */

void do_interactive ();
void print_info ();

/* ks-fit.c */

void ks_func_fit_usage ();
void cmd_ks_func_fit ();
void ks_dist_fit_usage ();
void cmd_ks_dist_fit ();

/* kstwo.c */

void kstwo ();

/* output.c */

void cmd_open ();
void cmd_close ();

/* percentile.c */

void cmd_perc ();

/* probks.c */

float probks ();

/* qsort.h */

void quick_sort ();
unsigned long partition ();

/* script.c */

void script_open ();
void script_close ();
void script_proc ();

/* silent.c */

void cmd_silent ();

/* stats.c */

void usage ();
void parseargs ();
int main ();

/* transform.c */

void cmd_transform ();
