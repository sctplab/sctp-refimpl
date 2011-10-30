#!/bin/sh

source=~/Documents/KERN/darwin
destination=~/Documents
kernel=xnu-1699.24.8

files='bsd/conf/MASTER
       bsd/conf/files
       bsd/net/rtsock.c
       bsd/netinet/in_proto.c
       bsd/netinet6/in6_proto.c
       bsd/sys/mbuf.h
       bsd/sys/socket.h'

for file in $files
do
	echo "Copying " $file
	cp $source/$kernel/$file $destination/$kernel/$file
done
