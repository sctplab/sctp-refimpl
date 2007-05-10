TEST_SOURCES                = test_sctp_sendmsg.c \
                              test_sctp_sockopts.c
bin_PROGRAMS                = make_register_tests api_tests
make_register_tests_SOURCES = make_register_tests.c
api_tests_SOURCES           = api_tests.c api_tests.h \
                              sctp_utilities.c sctp_utilities.h \
                              register_tests.c \
                              $(TEST_SOURCES)
api_tests_DEPENDENCIES      = make_register_tests
register_tests.c: $(TEST_SOURCES)
	$(srcdir)/make_register_tests
