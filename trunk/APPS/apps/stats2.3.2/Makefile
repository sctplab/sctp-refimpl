
# Copyright (c) 2001 BBNT Solutions LLC
#
# Permission to use, copy, modify, and distribute this software
# and its documentation for any purpose is hereby granted without
# fee, provided that the above copyright notice and this permission
# appear in all copies and in supporting documentation, and that the
# name of BBN Technologies not be used in advertising or publicity
# pertaining to distribution of the software without specific,
# written prior permission.  BBN makes no representations about the
# suitability of this software for any purposes.  It is provided "AS
# IS" without express or implied warranties.
#
# Mark Allman (mallman@ir.bbn.com)
# BBN Technologies
#
# Last Updated: Tue Feb 13, 2001

# General variables...

CC=gcc
CFLAGS=-g -Wall -Werror -I/usr/include/readline -Iinclude
CFLAGS=-g -Wall -Werror -I/usr/local/include -Iinclude
LD=$(CC)
LDFLAGS=$(CFLAGS)
LIBS=-lreadline -ltermcap -lm
LIBS=-L/usr/local/lib $(OTHER_LIB) -lreadline -ltermcap -lm
EXE=stats
VERS=2.3.2
VERSION=\"$(VERS)\"
OBJS=base.o basic.o batch.o cdf.o clear.o cmd.o dump.o freq.o help.o input.o \
	interactive.o ks-fit.o output.o percentile.o qsort.o script.o \
	silent.o stats.o transform.o
HDR=include/stats.h

# Compilation rules

all: $(OBJS)
	$(LD) $(LDFLAGS) -o $(EXE) $(OBJS) $(LIBS)

base.o: src/base.c $(HDR)
	$(CC) $(CFLAGS) -c src/base.c

basic.o: src/basic.c $(HDR)
	$(CC) $(CFLAGS) -c src/basic.c

batch.o: src/batch.c $(HDR)
	$(CC) $(CFLAGS) -c src/batch.c

cdf.o: src/cdf.c $(HDR)
	$(CC) $(CFLAGS) -c src/cdf.c

clear.o: src/clear.c $(HDR)
	$(CC) $(CFLAGS) -c src/clear.c

cmd.o: src/cmd.c $(HDR)
	$(CC) $(CFLAGS) -c src/cmd.c

dump.o: src/dump.c $(HDR)
	$(CC) $(CFLAGS) -c src/dump.c

freq.o: src/freq.c $(HDR)
	$(CC) $(CFLAGS) -c src/freq.c

help.o: src/help.c $(HDR)
	$(CC) $(CFLAGS) -c src/help.c

input.o: src/input.c $(HDR)
	$(CC) $(CFLAGS) -c src/input.c

interactive.o: src/interactive.c $(HDR)
	$(CC) $(CFLAGS) -DVERSION=$(VERSION) -c src/interactive.c

ks-fit.o: src/ks-fit.c $(HDR)
	$(CC) $(CFLAGS) -c src/ks-fit.c

output.o: src/output.c $(HDR)
	$(CC) $(CFLAGS) -c src/output.c

percentile.o: src/percentile.c $(HDR)
	$(CC) $(CFLAGS) -c src/percentile.c

qsort.o: src/qsort.c $(HDR)
	$(CC) $(CFLAGS) -c src/qsort.c

script.o: src/script.c $(HDR)
	$(CC) $(CFLAGS) -c src/script.c

silent.o: src/silent.c $(HDR)
	$(CC) $(CFLAGS) -c src/silent.c

stats.o: src/stats.c $(HDR)
	$(CC) $(CFLAGS) -c src/stats.c

transform.o: src/transform.c $(HDR)
	$(CC) $(CFLAGS) -c src/transform.c

# make release tarballs

tar: 
	$(MAKE) clobber
	tar cf stats$(VERS).tar --exclude CVS Makefile README src/ include/
	mkdir stats$(VERS)
	cd stats$(VERS); tar xf ../stats$(VERS).tar; cd ..
	rm -f stats$(VERS).tar
	tar cf stats$(VERS).tar stats$(VERS)/
	rm -rf stats$(VERS)
	gzip stats$(VERS).tar

# Cleaning rules...

clean:
	rm -f *.o lex.yy.c y.tab.c y.tab.h y.output core

clobber: clean
	rm -f $(EXE)
