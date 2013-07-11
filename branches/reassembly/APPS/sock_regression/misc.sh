ntest=1

expect()
{
	e="${1}"
	shift
	r=`$* 2>/dev/null | tail -1`
	echo "${r}" | egrep '^'${e}'$' >/dev/null 2>&1
	if [ $? -eq 0 ]; then
		echo "ok ${ntest}"
	else
	echo "not ok ${ntest}"
	fi
	ntest=`expr ${ntest} + 1`
}

