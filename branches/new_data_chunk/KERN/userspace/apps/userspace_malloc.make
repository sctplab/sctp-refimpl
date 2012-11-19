#EDIT THIS! LEAVE NO SPACE AT THE BACK
USERSPACELOC=/home/penoff/tmp/KERN/userspace/build

CC = gcc
OSTYPE = $(shell uname)
ARCHTYPE = $(shell uname -p | sed 's/386/486/' | sed 's/amd/x86-/')

# FreeBSD needs -march for gcc atomics, but that breaks 64bit x86 Darwin
ifeq ($(OSTYPE),FreeBSD)
CFLAGS  = -g $(MYGPROF) -DSCTP_USERMODE -U__FreeBSD__ -U__APPLE__ -U__Panda__ -U__Windows__ -D__Userspace__ -D__Userspace_os_$(OSTYPE) -DSCTP_SIMPLE_ALLOCATOR -Wall -march=$(ARCHTYPE)
else
CFLAGS  = -g $(MYGPROF) -DSCTP_USERMODE -U__FreeBSD__ -U__APPLE__ -U__Panda__ -U__Windows__ -D__Userspace__ -D__Userspace_os_$(OSTYPE) -DSCTP_SIMPLE_ALLOCATOR -Wall
endif

CPPFLAGS =  -I$(USERSPACELOC)/user_include -I$(USERSPACELOC) -I.
DEFS =
INCLUDES =
LDFLAGS = -L$(USERSPACELOC)/user_lib -L$(USERSPACELOC)
LIBS =  -lm -luserspace -lcrypto -lpthread 
C_COMPILE       = $(CC) $(DEFS) $(INCLUDES) $(CFLAGS) $(CPPFLAGS)


all:
	$(C_COMPILE) -o tsctp tsctp.c $(LDFLAGS) $(LIBS)

clean:
	rm tsctp 
