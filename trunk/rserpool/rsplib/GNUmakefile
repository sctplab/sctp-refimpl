TOPDIR=..

include $(TOPDIR)/GNUmakeconfig

#     
# You should put your special stuff here to override the ones defined 
# in $(TOPDIR)/GNUmakeconfig.
#     
include $(TOPDIR)/GNUmakerules

LOCAL_CFLAGS=


SRCS= rsp_lib.c rsp_timer.c rsp_enrp.c rsp_asap.c


OBJS= $(SRCS:%.c=$(OBJDIR)/%.o)

all:: $(OBJDIR) $(OBJDIR)/$(LIBRSERPOOL)($(OBJS)) 

include $(TOPDIR)/GNUmaketargets


