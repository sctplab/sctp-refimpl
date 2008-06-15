CC = gcc
OSTYPE = $(shell uname)
CFLAGS	= -g -U__FreeBSD__ -D__Userspace__ -D__Userspace_os_$(OSTYPE) -Wall
CPPFLAGS = -I.  -I./user_include
DEFS = 
INCLUDES = 
LDFLAGS = -L./user_lib -L. 
LIBS =  -lumem -luserspace -lcrypto -latomic_ops -lpthread
C_COMPILE       = $(CC) $(DEFS) $(INCLUDES) $(CFLAGS) $(CPPFLAGS)

#
#  The following illustrates how to use libuserspace.a to build 
#   example applications called ring_head and ring_node.  
#
#  To run, ensure that your LD_LIBRARY_PATH is set correctly to the
#   full path of build/user_lib . 
#

all:
	$(C_COMPILE) -o ring_head ring_head.c $(LDFLAGS) $(LIBS)
	$(C_COMPILE) -o ring_node ring_node.c $(LDFLAGS) $(LIBS)


clean: 
	rm -f ring_node ring_head

