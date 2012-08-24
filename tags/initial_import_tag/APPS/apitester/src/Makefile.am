# Please add additional files containing test cases here
TEST_SOURCES                = test_sctp_recvmsg.c \
                              test_sctp_sendmsg.c \
                              test_sctp_sockopts.c \
                              test_connect.c \
                              test_bind.c \
                              test_sctp_bindx.c \
                              test_shutdown.c \
                              test_socket.c \
                              test_sctp_peeloff.c
# Plese do not edit anything below
bin_PROGRAMS                = api_tests
api_tests_SOURCES           = api_tests.c api_tests.h \
                              sctp_utilities.c sctp_utilities.h \
                              register_tests.c \
                              $(TEST_SOURCES)
EXTRA_DIST                  = make_register_tests.sh \
                              get_descriptions \
                              stress-test
CLEANFILES                  = register_tests.c \
                              test_descriptions.txt
register_tests.c: $(TEST_SOURCES) make_register_tests.sh
	sh make_register_tests.sh register_tests.c $(TEST_SOURCES)
dist-hook:
	@rm -f $(distdir)/register_tests.c
