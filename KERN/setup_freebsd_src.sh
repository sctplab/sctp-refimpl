#!/bin/sh
echo "Where is your cvs path (e.g. /usr/rrs/sctpCVS)?"
read cvsPath
cd $cvsPath/KERN
if test ! -d netinet
	then
	echo "sorry I cannot see the netinet directory"
	exit
fi
echo "Where is your kernel source code that you are using (e.g. /usr/src/sys)?"
read srcTree
if test ! -d $srcTree/dev
    then
    echo "I cant see the dev directory there sorry"
    exit
fi
cd $cvsPath/KERN/freebsd7 
for j in conf/files conf/options kern/syscalls.master \
    kern/uipc_socket.c kern/uipc_syscalls.c net/rtsock.c netinet/in_proto.c \
    netinet6/in6_proto.c sys/mbuf.h sys/protosw.h sys/socket.h
  do
  rm -f $srcTree/$j
  ln -s $cvsPath/KERN/freebsd7/$j $srcTree/$j
done
cd $cvsPath/KERN
echo "Preparing kernel SCTP sources now"
./export_to_freebsd
echo "Linking in SCTP sources"
cd $cvsPath/KERN/export_freebsd/netinet
for j in `ls`
do
rm -f $srcTree/netinet/$j
ln -s $cvsPath/KERN/export_freebsd/netinet/$j $srcTree/netinet/$j
done
cd $cvsPath/KERN/export_freebsd/netinet6
for j in `ls`
do
rm -f $srcTree/netinet6/$j
ln -s $cvsPath/KERN/export_freebsd/netinet6/$j $srcTree/netinet6/$j
done
cd $srcTree/kern
touch syscalls.master
make sysent
echo "You may now go to your kernel source tree $srcTree"
echo "Configure a new kernel with:"
echo "options SCTP"
echo "And build as normal"
echo "This script as already did a 
echo "'make sysent'"
echo "In $srcTree/kern so the syscalls are present"
echo "in the kernel. However you probably need to
echo "copy these to /usr/include/sys and also
echo "do a  make buildworld and make installworld"
echo "which will get the sctp_xxx() calls into libc"


