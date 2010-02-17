# on Linux, "make" seems fine.
# on FreeBSD, "make MAKE=gmake" seems to have better luck.

all:
	./setup_userspace_src.sh
	(cd build && ${MAKE})
	echo "Created libuserspace.a within the directory named build. See Application.make for an example build script to create applications."

clean:
#	rm -rf build
	(cd build && ls -1 | grep -v CVS | xargs rm -rf)

destroy: clean
	(cd umem-1.0.1 && make distclean)
	(cd umem-1.0.1 && rm -rf autom4te.cache)
#	(cd ../../libatomic_ops-1.2 && make distclean)
#	(cd ../../libatomic_ops-1.1 && make distclean)
