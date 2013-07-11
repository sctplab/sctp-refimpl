#!/bin/sh

if test $# -gt 0
then
    echo "Usage: $0"
    exit 1
fi
exportUserspace=${PWD}
buildDir=${PWD}/build

# if already built, we can exit gracefully...
if test -e $buildDir/user_lib/libuserspace.a
then
    echo "libuserspace.a already built in $buildDir/user_lib"
    exit 0
fi

# we need to build $buildDir/user_lib/libuserspace.a 
       
cd $exportUserspace


# copy files into structure that build understands

# create build directory
if test ! -d $buildDir
    then
    mkdir $buildDir
fi

# create the buildDir's user_lib directory (we'll copy this to 
#   $buildDir/../user_lib , later)
#if test ! -d user_lib
#    then
#    mkdir user_lib
#fi

# copy "normal" SCTP files
cd $buildDir
if test ! -d netinet
    then
    mkdir netinet
fi
cp $exportUserspace/../netinet/*.c .
cp $exportUserspace/../netinet/*.h netinet/.
if test ! -d netinet6
    then
    mkdir netinet6
fi
cp $exportUserspace/../netinet6/sctp6_var.h netinet6/.

# copy userspace-specific files
cp $exportUserspace/user_src/*.c .
cp -r $exportUserspace/user_src/user_include .

# copy Makefile to buildDir
cp $exportUserspace/userspace.make Makefile

# copy build scripts
cp $exportUserspace/scripts/* .

# copy example application Makefile
cp $exportUserspace/userspace_app.make Application.make

echo "If you called setup_userspace_src.sh directly, now start your make."
