#!/bin/sh

OSTYPE=`uname`
ARCHTYPE=`uname -p`

if [ $OSTYPE = FreeBSD ]; then
# use umem-1.0
    echo "... is FreeBSD."
    echo "Destroying umem-1.0 ..."

    (cd umem-1.0 && make distclean)
    (cd umem-1.0 && rm -rf autom4te.cache)

elif [ $ARCHTYPE = powerpc ]; then
# use umem-1.0
    echo "... is powerpc."
    echo "Destroying umem-1.0 ..."

    (cd umem-1.0 && make distclean)
    (cd umem-1.0 && rm -rf autom4te.cache)

else
# use umem-1.0.1
    echo "... is NOT FreeBSD."
    echo "Destroying umem-1.0.1 ..."

    (cd umem-1.0.1 && make distclean)
    (cd umem-1.0.1 && rm -rf autom4te.cache)

fi



