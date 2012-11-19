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
echo "Will you be using FreeBSD 6.2 or 6.1 or 6.0 (62 or 61 or 60)?"
read ans
if test $ans = 7
then
echo "Don't use this tool for 7.0 and beyond - its in the tree do ./compare_srcs and copy"
exit 
elif test $ans = 60
then
echo "I will use 6.0 then, if unsure hit ctl-c else return"
read ans
BSD_PREPARE=freebsd_prepare6
BSD_PATH=freebsd6
BSD_VER=600000
elif test $ans = 61
then
echo "I will use 6.1 then, if unsure hit ctl-c else return"
read ans
BSD_PREPARE=freebsd_prepare
BSD_PATH=freebsd6_1
BSD_VER=601000
elif test $ans = 62
then
echo "I will use 6.2 then, if unsure hit ctrl-c else return"
read ans
BSD_PREPARE=freebsd_prepare
BSD_PATH=freebsd6_2
BSD_VER=602000
else
echo "Unknown release sorry"
exit
fi
cd $cvsPath/KERN/$BSD_PATH 
for j in conf/files conf/options kern/syscalls.master \
    kern/uipc_syscalls.c net/rtsock.c netinet/in_proto.c \
    netinet6/in6_proto.c netinet6/nd6_nbr.c sys/mbuf.h sys/socket.h
  do
  if test -f $cvsPath/KERN/$BSD_PATH/$j
      then
      echo Linking $cvsPath/KERN/$BSD_PATH/$j
      rm -f $srcTree/$j
      ln -s $cvsPath/KERN/$BSD_PATH/$j $srcTree/$j
  fi
done
cd $cvsPath/KERN
echo "Preparing kernel SCTP sources now"
./export_to_freebsd $BSD_VER $BSD_PREPARE
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
echo "Step 1--------------------------------------------"
echo "You may now go to your kernel source tree $srcTree"
echo "Configure a new kernel with:"
echo "options SCTP"
echo "And build and install the new kernel as normal."
echo "Step 2--------------------------------------------"
echo "Now you need to copy your new syscall references so"
echo "that libsctp can find the sctp syscalls"
echo "by performing:"
echo "cp $srcTree/sys/syscall.h /usr/include/sys/"
echo "cp $srcTree/sys/syscall.mk /usr/include/sys/"
echo "cp $srcTree/sys/sysproto.h /usr/include/sys/"
echo "Step 3--------------------------------------------"
echo "Next build libsctp by doing:"
echo "cd $cvsPath/KERN/usr.lib"
echo "make"
echo "cp libsctp* /lib"
echo "ranlib /lib/libsctp.a /lib/libsctp_p.a"
echo "Step 4--------------------------------------------"
echo "This step is optional. You may want to do it"
echo "if you want libc.a to contain the true"
echo "sctp_xxx systemcalls."
echo "Follow the instructions in $srcTree/README"
echo "to do the make buildworld and make installworld"




