AM_CFLAGS=-g -Wall -Werror
AM_CPPFLAGS=-I.
bin_PROGRAMS = incast_client incast_server elephant_sink elephant_source \
display_ele_sink display_inc_client display_ele_client \
ele_aggregator sum_aggregator read_cap per_aggregator strip_empty_line

LDADD = -lpthread -lpcap

incast_client_SOURCES = incast_client.c incast_lib.c

incast_server_SOURCES = incast_server.c incast_lib.c

elephant_sink_SOURCES = elephant_sink.c incast_lib.c

elephant_source_SOURCES = elephant_source.c incast_lib.c

display_ele_sink_SOURCES = display_ele_sink.c incast_lib.c

display_inc_client_SOURCES = display_inc_client.c incast_lib.c

display_ele_client_SOURCES = display_ele_client.c incast_lib.c

ele_aggregator_SOURCES = ele_aggregator.c

sum_aggregator_SOURCES = sum_aggregator.c

per_aggregator_SOURCES = per_aggregator.c

read_cap_SOURCES = read_cap.c

strip_empty_line_SOURCES = strip_empty_line.c

