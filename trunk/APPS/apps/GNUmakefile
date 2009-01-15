TOPDIR=..

include $(TOPDIR)/GNUmakeconfig

#
# You should put your special stuff here to override the ones defined
# in $(TOPDIR)/GNUmakeconfig.
#
include $(TOPDIR)/GNUmakerules

LOCAL_CFLAGS=-g
LDFLAGS= 

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

PROGSRCS= block_test.c sctp_test_app.c cmp_server.c cmp_client.c \
m_cmp_client.c data_sorter.c client_manyconn.c \
server.c interest_client.c interest_server.c \
tcp_server.c tcp_client.c  peel_client.c peel_server.c tsctp.c  \
cmp_sendfile.c cmp_recvfile.c sctp_dynamic_prim.c tcp_non_read.c \
test_send.c

#LOCAL_LIBS+=

#PROGSRCS= sctp_test_app.c cmp_server.c cmp_client.c
#
OBJSRCS=hashWalk.c primelist.c stdintest.c
#

SRCS= $(OBJSRCS) $(PROGSRCS) # for creating depend 

OBJS=	$(OBJSRCS:%.c=$(OBJDIR)/%.o) 

PROGS=	$(PROGSRCS:%.c=$(EXEDIR)/%)

all::   $(OBJDIR) $(PROGS) 

include $(TOPDIR)/GNUmaketargets
