# This is the top level of make. We only deal with the common
# directories.

DIRS= baselib user debug_tools apps tcl_tester apitester

all depend clean clobber release:
	@for i in $(DIRS); do \
		 echo making $@ in $$i; \
		 (cd $$i; $(MAKE) $@); \
		 if [ $$? != 0 ]; then \
			echo Failed to make $@ in $$i; \
			exit 1; \
		 fi; \
	done

install:
	@echo You need to make $@ in the application directory.
