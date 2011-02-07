AM_CFLAGS=-g -Wall -Werror
AM_CPPFLAGS=-I.
bin_PROGRAMS = incast_client incast_server elephant_sink elephant_source watch_intf display_ele_sink display_inc_client

LDADD = -lpthread

incast_client_SOURCES = incast_client.c incast_lib.c

incast_server_SOURCES = incast_server.c incast_lib.c

elephant_sink_SOURCES = elephant_sink.c incast_lib.c

elephant_source_SOURCES = elephant_source.c incast_lib.c

display_ele_sink_SOURCES = display_ele_sink.c incast_lib.c

display_inc_client_SOURCES = display_inc_client.c incast_lib.c

watch_intf_SOURCES = watch_intf.c

