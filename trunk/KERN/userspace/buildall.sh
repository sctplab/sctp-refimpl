#!/bin/sh

OSTYPE=`uname`
ARCHTYPE=`uname -p`

if [ $OSTYPE = FreeBSD ]; then

    echo "... is FreeBSD."
  if [ $ARCHTYPE = amd64 ]; then
# use umem-1.0.1
      echo "... is amd64."
      echo "64-bit bug with libumem. Build with make malloc instead."
      exit 1 
#      echo "Using umem-1.0.1 ..."
#      cp userspace_umem1.0.1.make userspace.make
#      cp scripts/prepare_umem101 scripts/prepare_umem
  else
# use umem-1.0
    echo "Using umem-1.0 ..."
    cp userspace_umem1.0.make userspace.make
    cp scripts/prepare_umem10 scripts/prepare_umem
  fi

# build with gmake
    echo "Using gmake ..."
    make -f allplatform.make MAKE=gmake

elif [ $ARCHTYPE = powerpc ]; then

# use umem-1.0
    echo "... is powerpc."
    echo "Using umem-1.0 ..."
    cp userspace_umem1.0.make userspace.make
    cp scripts/prepare_umem10 scripts/prepare_umem

# build with make
    echo "Using make ..."
    make -f allplatform.make

else

# use umem-1.0.1
    echo "... is NOT FreeBSD or NOT powerpc."
    echo "Using umem-1.0.1 ..."
    cp userspace_umem1.0.1.make userspace.make
    cp scripts/prepare_umem101 scripts/prepare_umem

# build with make
    echo "Using make ..."
    make -f allplatform.make

fi



