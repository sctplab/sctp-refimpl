TOPDIR=..

include $(TOPDIR)/GNUmakeconfig

#     
# You should put your special stuff here to override the ones defined 
# in $(TOPDIR)/GNUmakeconfig.
#     
include $(TOPDIR)/GNUmakerules

LOCAL_CFLAGS=


SRCS= dlist.c llist.c HashedTbl.c distributor.c StateMachine.c \
	msgDirector.c udpDist.c sctp_csum.c byte_work.c hlist.c \
	memcheck.c printMessageHandler.c processObj.c signalCleanup.c



OBJS= $(SRCS:%.c=$(OBJDIR)/%.o)

all:: $(OBJDIR) $(OBJDIR)/$(LIBBASE)($(OBJS)) 

include $(TOPDIR)/GNUmaketargets


