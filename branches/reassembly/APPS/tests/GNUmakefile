TOPDIR=..

include $(TOPDIR)/GNUmakeconfig

#
# You should put your special stuff here to override the ones defined
# in $(TOPDIR)/GNUmakeconfig.
#
include $(TOPDIR)/GNUmakerules

LOCAL_CFLAGS=
LDFLAGS=

ifeq ($(GUESS_HOST),Darwin)
LOCAL_LIBS+= -L/usr/lib -L/sw/lib -L../../KERN/usr.lib -lm -lsctp
else
ifeq ($(GUESS_HOST),LynxOS)
LOCAL_LIBS+= -L/usr/local/v6/lib -mthreads -lnetinet -lnsl -lm -lsctp
else
ifeq ($(GUESS_HOST),Linux)
LOCAL_LIBS+= -L/usr/local/v6/lib -lnsl -lm -ltermcap -lreadline -lsctp
else
ifeq ($(GUESS_HOST),FreeBSD)
LOCAL_LIBS+= -L/usr/local/v6/lib -lm -lreadline -lsctp
#LOCAL_LIBS+= /usr/local/v6/lib/libsctp.a -lm -lreadline
else
ifeq ($(GUESS_HOST),NetBSD)
LOCAL_LIBS+= -L/usr/local/v6/lib -lm -lreadline -ltermcap -lsctp
else
ifeq ($(GUESS_HOST),OpenBSD)
LOCAL_LIBS+= -L/usr/local/v6/lib -lm -lreadline -ltermcap -lsctp
else
ifeq ($(GUESS_HOST),AIX)
LOCAL_LIBS+= -L/usr/local/v6/lib -lm
else
ifeq ($(GUESS_HOST),SunOS)
LOCAL_LIBS+= -L/usr/local/v6/lib -lsocket -lnsl -lm -lposix4 -lpthread -ltermcap -lreadline -lhistory -lsctp
else
LOCAL_LIBS+= -L/usr/local/v6/lib -lsocket -lnsl -lm -lposix4 -lpthread -lsctp
endif
endif
endif
endif
endif
endif
endif
endif

PROGSRCS= sctpClientV4.c  sctpClientV6.c  sctpServerV6.c sctpServerV4.c


#LOCAL_LIBS+=-lsctp

#PROGSRCS= sctp_test_app.c cmp_server.c cmp_client.c
#
#OBJSRCS=
#

SRCS= $(OBJSRCS) $(PROGSRCS) # for creating depend 

OBJS=	$(OBJSRCS:%.c=$(OBJDIR)/%.o) 

PROGS=	$(PROGSRCS:%.c=$(EXEDIR)/%)

all::   $(OBJDIR) $(PROGS) 

include $(TOPDIR)/GNUmaketargets
