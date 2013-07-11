# on Linux, "make" seems fine.
# on FreeBSD, "make MAKE=gmake" seems to have better luck.

all:
	./setup_userspace_src.sh
	(cd build && ${MAKE} MYGPROF=${MYGPROF})
	echo "Created libuserspace.a within the directory named build. See Application.make for an example build script to create applications."

