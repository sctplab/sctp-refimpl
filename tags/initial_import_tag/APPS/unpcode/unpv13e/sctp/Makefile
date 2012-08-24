include ../Make.defines

PROGS =	sctpserv01 sctpclient01 sctpserv02 sctpserv03 sctpclient02 sctpserv04 \
sctpserv05 sctpclient03 sctpserv06 sctpserv07 sctpclient04 sctpserv_fork

LIBS+= -L/usr/local/v6/lib -lm -lsctp

.c.lc:
	../../troff/fixcode.sh $< > $@
.h.lh:
	../../troff/fixcode.sh $< > $@

CFLAGS+= -g
all:	${PROGS}

sctpserv01:	sctpserv01.o sctp_wrapper.o sctp_getnostrm.o sctp_addr_to_associd.o
		${CC} ${CFLAGS} -o $@ sctpserv01.o sctp_wrapper.o sctp_getnostrm.o sctp_addr_to_associd.o  ${LIBS}

sctp_wrapper:	sctp_wrapper.o
		${CC} ${CFLAGS} -c -o $@ sctp_wrapper.o ${LIBS}


sctpclient01:   sctpclient01.o sctp_strcli.o sctp_strcliecho.o sctp_wrapper.o sctp_getnostrm.o sctp_addr_to_associd.o
		${CC} ${CFLAGS} -o $@ sctpclient01.o sctp_wrapper.o sctp_getnostrm.o sctp_addr_to_associd.o sctp_strcli.o sctp_strcliecho.o ${LIBS}


sctpserv02:	sctpserv02.o sctp_getnostrm.o sctp_addr_to_associd.o  sctp_wrapper.o
		${CC} ${CFLAGS} -o $@ sctpserv02.o sctp_getnostrm.o sctp_addr_to_associd.o  sctp_wrapper.o ${LIBS}

sctpserv03:	sctpserv03.o sctp_getnostrm.o sctp_addr_to_associd.o  sctp_wrapper.o
		${CC} ${CFLAGS} -o $@ sctpserv03.o sctp_getnostrm.o sctp_addr_to_associd.o  sctp_wrapper.o ${LIBS}

sctpserv04:	sctpserv04.o sctp_getnostrm.o sctp_addr_to_associd.o  sctp_wrapper.o
		${CC} ${CFLAGS} -o $@ sctpserv04.o sctp_getnostrm.o sctp_addr_to_associd.o  sctp_wrapper.o ${LIBS}

sctpserv05:	sctpserv05.o sctp_pdapircv.o sctp_getnostrm.o sctp_addr_to_associd.o  sctp_wrapper.o
		${CC} ${CFLAGS} -o $@ sctpserv05.o sctp_pdapircv.o  sctp_getnostrm.o sctp_addr_to_associd.o  sctp_wrapper.o ${LIBS}

sctpserv06:	sctpserv06.o sctp_displayevents.o sctp_getnostrm.o sctp_addr_to_associd.o sctp_wrapper.o
		${CC} ${CFLAGS} -o $@ sctpserv06.o sctp_displayevents.o  sctp_getnostrm.o sctp_addr_to_associd.o  sctp_wrapper.o ${LIBS}

sctpclient02:	sctpclient02.o sctp_strcli.o sctp_strcliecho.o sctp_getnostrm.o sctp_addr_to_associd.o  sctp_wrapper.o
		${CC} ${CFLAGS} -o $@ sctpclient02.o sctp_strcli.o sctp_strcliecho.o sctp_getnostrm.o sctp_addr_to_associd.o  sctp_wrapper.o ${LIBS}

sctpclient03:  sctpclient01.o sctp_strcli_un.o sctp_strcliecho.o sctp_getnostrm.o sctp_addr_to_associd.o  sctp_wrapper.o
		${CC} ${CFLAGS} -o $@ sctpclient01.o sctp_strcli_un.o sctp_strcliecho.o sctp_getnostrm.o sctp_addr_to_associd.o  sctp_wrapper.o ${LIBS}

sctpserv07:	sctpserv07.o sctp_displayevents.o sctp_bindargs.o sctp_getnostrm.o sctp_addr_to_associd.o  sctp_wrapper.o
		${CC} ${CFLAGS} -o $@ sctpserv07.o sctp_displayevents.o  sctp_bindargs.o sctp_getnostrm.o sctp_addr_to_associd.o  sctp_wrapper.o ${LIBS}


sctpclient04:	sctpclient04.o sctp_strcli1.o sctp_check_notify.o sctp_print_addrs.o sctp_modify_hb.o sctp_getnostrm.o sctp_addr_to_associd.o  sctp_wrapper.o
		${CC} ${CFLAGS} -o $@ sctpclient04.o sctp_strcli1.o sctp_check_notify.o sctp_print_addrs.o sctp_getnostrm.o sctp_addr_to_associd.o  sctp_wrapper.o ${LIBS}

sctpserv_fork:  sctpserv_fork.o sctp_addr_to_associd.o  sctp_wrapper.o
		${CC} ${CFLAGS} -o $@ sctpserv_fork.o sctp_addr_to_associd.o  sctp_wrapper.o ${LIBS}

clean:
		rm -f ${PROGS} ${CLEANFILES}
