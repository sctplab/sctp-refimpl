CC = gcc
OSTYPE = $(shell uname)
#ARCHTYPE = $(shell uname -p | sed 's/386/486/')
ARCHTYPE = $(shell uname -p | sed 's/386/486/' | sed 's/amd/x86-/')
UMEMMAKE = $(shell ls 2> /dev/null ../umem-bsd64/Makefile)
ATOMICMAKE = $(shell ls 2> /dev/null ../libatomic_ops-1.1/Makefile)

# FreeBSD needs -march for gcc atomics, but that breaks 64bit x86 Darwin
ifeq ($(OSTYPE),FreeBSD)
CFLAGS  = -g $(MYGPROF) -U__FreeBSD__ -U__APPLE__ -U__Panda__ -U__Windows__ -D__Userspace__ -D__Userspace_os_$(OSTYPE) -Wall -march=$(ARCHTYPE)
else
CFLAGS  = -g $(MYGPROF) -U__FreeBSD__ -U__APPLE__ -U__Panda__ -U__Windows__ -D__Userspace__ -D__Userspace_os_$(OSTYPE) -Wall 
endif
# this breaks powerPC builds -march=i486

CPPFLAGS = -I.  -I./user_include
DEFS = 
INCLUDES = 
LDFLAGS = -L./user_lib -L. 
LIBS =  -lumem -luserspace -lcrypto -lpthread # -latomic_ops  
C_COMPILE       = $(CC) $(DEFS) $(INCLUDES) $(CFLAGS) $(CPPFLAGS)

SOURCES	=	sctp_input.c \
sctputil.c \
sctp_timer.c \
sctp_auth.c \
sctp_output.c \
sctp_indata.c \
sctp_usrreq.c \
user_mbuf.c \
sctp_sysctl.c \
sctp_pcb.c \
sctp_bsd_addr.c \
sctp_cc_functions.c \
user_environment.c \
sctp_crc32.c \
user_sctp_callout.c \
user_sctp_timer_iterate.c \
sctp_asconf.c \
user_recv_thread.c \
user_socket.c

# might need to add sctp_sha1.c to FreeBSD system (libssl-dev on Ubunutu 
#  redefines SHA1_* functions...)
# sctp_sha1.c \

OBJS	= ${SOURCES:.c=.o}



.SUFFIXES:
.SUFFIXES: .c .o


.c.o:
	$(C_COMPILE) -c $<

# I can't figure out how to get this prefix to work...
#.c:
#	$(C_COMPILE) -o $* $< $(LDFLAGS) $(LIBS)



all:    umem  all-obj # atomic_ops

all-obj:  $(OBJS)
	ar rvs libuserspace.a $(OBJS)

umem:
	sh ./prepare_umem

atomic_ops:
	sh ./prepare_atomic_ops


clean: clean-obj

destroy: clean-umem clean-atomic clean-obj
	rm -rf user_lib

clean-obj:
	rm -rf *.o *.a

clean-umem:
	rm -rf umem-bsd64-prefix
	rm -f user_lib/libumem*
	rm -f user_include/umem.h user_include/sys/vmem.h
ifeq ($(UMEMMAKE),../umem-bsd64/Makefile)
	(cd ../umem-bsd64 && make distclean)
	rm -rf ../umem-bsd64/autom4te.cache ../umem-bsd64/configure
endif

clean-atomic:
	rm -rf atomic_ops-1.1-prefix
	rm -rf atomic_ops-1.2-prefix
	rm -f user_lib/libatomic_ops*
	rm -f user_include/atomic_ops*
ifeq ($(ATOMICMAKE),../libatomic_ops-1.1/Makefile)
	(cd ../libatomic_ops-1.1 && make distclean)
#	rm -rf ../libatomic_ops-1.1/autom4te.cache
endif
ifeq ($(ATOMICMAKE),../libatomic_ops-1.2/Makefile)
	(cd ../libatomic_ops-1.2 && make distclean)
#	rm -rf ../libatomic_ops-1.2/autom4te.cache
endif


