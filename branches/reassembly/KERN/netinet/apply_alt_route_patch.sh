#!/bin/sh
cwd=`pwd`
if test -f $cwd/alternate_route_patch
    then
    echo "Found alternate_route_patch file"
else
    echo "Sorry can't find alternate route patch file"
    echo "it must be in your starting directory $cwd"
    exit
fi

echo "Where is your kame stack installed"
read kame
if test -d  $kame/kame/sys/netinet
then
    echo "I have found target directory in kame"
    cd $kame/kame/sys/netinet
    cat $cwd/alternate_route_patch | patch
    echo "patch applied. results are above"
else
    echo "Sorry can't locate directory $kame/kame/sys/netinet"
    echo "Patch did NOT install"
fi

    
