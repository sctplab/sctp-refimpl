# all platforms should just be built with "make". These scripts figure it out.

all:
	./buildall.sh

clean:
	rm -f userspace.make scripts/prepare_umem
	(cd build && ls -1 | grep -v CVS | xargs rm -rf)

destroy: clean
	./destroyall.sh
