#!/bin/sh

dir=`dirname $0`
. ${dir}/../misc.sh

echo "1..1"
expect 0 ${dir}/ut_006_sctp_sendmsg
