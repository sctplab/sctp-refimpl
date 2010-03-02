#!/bin/sh

OSTYPE=`uname`
ARCHTYPE=`uname -p`


cp userspace_malloc.make userspace.make

if [ $OSTYPE = FreeBSD ]; then

# build with gmake
    echo "... is FreeBSD."
    echo "Using gmake ..."
    make -f allplatform.make MAKE=gmake

else

# build with make
    echo "Using make ..."
    make -f allplatform.make

fi



