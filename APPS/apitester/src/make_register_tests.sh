#! /bin/sh

outfile="$1"
shift

echo '/* This files is created automatically by the Makefile. */' > ${outfile}
echo '' >> ${outfile}
echo '#include "api_tests.h"' >> ${outfile}
echo '' >> ${outfile}
for infile in "$@"
do
grep -h DEFINE $infile | sed -e 's/DEFINE_APITEST/DECLARE_APITEST/' -e 's/)/);/' >> ${outfile}
done
echo '' >> ${outfile}
echo 'struct test all_tests[] = {' >> ${outfile}
for infile in "$@"
do
grep -h DEFINE $infile | sed -e 's/DEFINE_APITEST/    REGISTER_APITEST/' -e 's/)/),/' >> ${outfile}
done
echo '};' >> ${outfile}
echo '' >> ${outfile}
echo 'unsigned int number_of_tests = sizeof(all_tests) / sizeof(all_tests[0]);' >> ${outfile}
echo '' >> ${outfile}