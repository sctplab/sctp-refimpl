CC = gcc
OSTYPE = $(shell uname)
ARCHTYPE = $(shell uname -p | sed 's/386/486/' | sed 's/amd/x86-/')

# FreeBSD needs -march for gcc atomics, but that breaks 64bit x86 Darwin
ifeq ($(OSTYPE),FreeBSD)
CFLAGS	= -g $(MYGPROF) -U__FreeBSD__ -U__APPLE__ -U__Panda__ -U__Windows__ -D__Userspace__ -D__Userspace_os_$(OSTYPE) -DSCTP_SIMPLE_ALLOCATOR -Wall -march=$(ARCHTYPE)
else
CFLAGS	= -g $(MYGPROF) -U__FreeBSD__ -U__APPLE__ -U__Panda__ -U__Windows__ -D__Userspace__ -D__Userspace_os_$(OSTYPE) -DSCTP_SIMPLE_ALLOCATOR -Wall 
endif
# this breaks powerPC builds -march=i486 

CPPFLAGS = -I.  -I./user_include
DEFS = 
INCLUDES = 
LDFLAGS = -L. 
LIBS =  -luserspace -lcrypto -lpthread # -latomic_ops  
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



all:  $(OBJS)
	ar rvs libuserspace.a $(OBJS)

clean: clean-obj

destroy: clean-obj

clean-obj:
	rm -rf *.o *.a

