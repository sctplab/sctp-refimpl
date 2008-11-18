CC = gcc
OSTYPE = $(shell uname)
CFLAGS	= -g -U__FreeBSD__ -U__APPLE__ -U__Panda__ -U__Windows__ -D__Userspace__ -D__Userspace_os_$(OSTYPE) -Wall
CPPFLAGS = -I.  -I./user_include
DEFS = 
INCLUDES = 
LDFLAGS = -L./user_lib -L.
LIBS =  -lumem -luserspace -lcrypto -lpthread
# -latomic_ops 
C_COMPILE       = $(CC) $(DEFS) $(INCLUDES) $(CFLAGS) $(CPPFLAGS)

#
#  The following illustrates how to use libuserspace.a to build 
#   example applications called ring_head and ring_node.  
#
#  To run, ensure that your LD_LIBRARY_PATH is set correctly to the
#   full path of build/user_lib . 
#

all: softlinks apps

softlinks:
	sh locate_libraries
apps:
	$(C_COMPILE) -o discard_server discard_server.c $(LDFLAGS) $(LIBS)
	$(C_COMPILE) -o discard_client discard_client.c $(LDFLAGS) $(LIBS)
	$(C_COMPILE) -o discard_app discard_app.c $(LDFLAGS) $(LIBS)


clean: 
	rm -f discard_server discard_client discard_app

destroy: clean
	rm -f user_include user_lib netinet libuserspace.a

