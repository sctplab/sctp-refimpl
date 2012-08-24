TOPDIR= ../../APPS

include $(TOPDIR)/GNUmakeconfig

#
# You should put your special stuff here to override the ones defined
# in $(TOPDIR)/GNUmakeconfig.
#
include $(TOPDIR)/GNUmakerules

REFIMP_DIR=$(TOPDIR)/user
REFIMP_INCLUDE=-I$(REFIMP_DIR)
LIBREFIMP=libsctp.a

LOCAL_CFLAGS= $(REFIMP_INCLUDE)
LDFLAGS=

APPLICATION_LIBS+= $(LIBREFIMP) 

ifeq ($(GUESS_HOST),Darwin)
LOCAL_LIBS+= -lc -lm
LOCAL_CFLAGS+= -DHAVE_SCTP_PEELOFF_SOCKOPT
else
ifeq ($(GUESS_HOST),FreeBSD) 
LOCAL_LIBS+=  -lc -lm 
else
ifeq ($(GUESS_HOST),LynxOS)
LOCAL_LIBS+= -lnetinet -lnsl -lm
else
ifeq ($(GUESS_HOST),Linux)
LOCAL_LIBS+=  -lnsl -lm 
else
ifeq ($(GUESS_HOST),OSF1)
LOCAL_LIBS+=  -lm
else
ifeq ($(GUESS_HOST),SunOS)
LOCAL_CFLAGS+=-D_XOPEN_SOURCE=1 -D_XOPEN_SOURCE_EXTENDED=1 -D__EXTENSIONS__
LOCAL_LIBS+=  -lsocket -lnsl -lm -lposix4 
else
# Solaris??
LOCAL_LIBS+=  -lsocket -lnsl -lm -lposix4 
endif
endif
endif
endif
endif
endif
#
# local library functions
#
OBJSRCS= 
COBJSRCS= sctp_sys_calls.c 

#
# runables
# 
PROGSRCS= 


COBJS=	$(COBJSRCS:%.c=$(OBJDIR)/%.o) 

PROGS=	$(PROGSRCS:%.c=$(EXEDIR)/%)

all:: $(OBJDIR) $(LIBREFIMP)($(COBJS))

all:: $(PROGS) 

include $(TOPDIR)/GNUmaketargets

