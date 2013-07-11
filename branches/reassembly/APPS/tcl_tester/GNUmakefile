TOPDIR=..

include $(TOPDIR)/GNUmakeconfig

#
# You should put your special stuff here to override the ones defined
# in $(TOPDIR)/GNUmakeconfig.
#
include $(TOPDIR)/GNUmakerules

LOCAL_CFLAGS=-g
#LDFLAGS= -lsctp

ifeq ($(GUESS_HOST),Darwin)
LOCAL_LIBS+= -L/usr/local/lib -L/usr/lib  -L../../KERN/usr.lib  -lreadline -lm -lpthread
else
ifeq ($(GUESS_HOST),LynxOS)
LOCAL_LIBS+= -L/usr/local/v6/lib -mthreads -lnetinet -lnsl -lm 
else
ifeq ($(GUESS_HOST),Linux)
LOCAL_LIBS+= -lnsl -lm -ltermcap -lreadline 
else
ifeq ($(GUESS_HOST),FreeBSD)
LOCAL_LIBS+= -L/usr/local/v6/lib -lm -lreadline   -lpthread
#LOCAL_LIBS+= /usr/local/v6/lib/libsctp.a -lm -lreadline
else
ifeq ($(GUESS_HOST),NetBSD)
LOCAL_LIBS+= -L/usr/local/v6/lib -lm -lreadline -ltermcap   -lpthread
else
ifeq ($(GUESS_HOST),OpenBSD)
LOCAL_LIBS+= -L/usr/local/v6/lib -lm -lreadline -ltermcap 
else
ifeq ($(GUESS_HOST),AIX)
LOCAL_LIBS+= -L/usr/local/v6/lib -lm
else
ifeq ($(GUESS_HOST),SunOS)
LOCAL_LIBS+= -L/usr/local/v6/lib -lsocket -lnsl -lm -lposix4 -lpthread -ltermcap -lreadline -lhistory 
else
LOCAL_LIBS+= -L/usr/local/v6/lib -lsocket -lnsl -lm -lposix4 -lpthread 
endif
endif
endif
endif
endif
endif
endif
endif

PROGSRCS= sctp_tcl_test_app.c


#LOCAL_LIBS+=

#PROGSRCS= sctp_test_app.c cmp_server.c cmp_client.c
#
OBJSRCS=
#

SRCS= $(OBJSRCS) $(PROGSRCS) # for creating depend 

OBJS=	$(OBJSRCS:%.c=$(OBJDIR)/%.o) 

PROGS=	$(PROGSRCS:%.c=$(EXEDIR)/%)

all::   $(OBJDIR) $(PROGS) 

include $(TOPDIR)/GNUmaketargets
