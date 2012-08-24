PEFIX?=	/usr/local
SYS_INCLUDES?=	#-I/usr/src/sys-altq
SYS_DEFINES?=	#-DINET6

SRCS= 	sctp_sys_calls.c
CFLAGS=		-g -Wall
CFLAGS+=	-Wpointer-arith -Wmissing-prototypes -ansi
DEFINES=	$(SYS_DEFINES)
INCLUDES=	-I. $(SYS_INCLUDES)
SHLIB_MAJOR=1
SHLIB_MINOR=1
LIB=sctp

.include <bsd.lib.mk>
