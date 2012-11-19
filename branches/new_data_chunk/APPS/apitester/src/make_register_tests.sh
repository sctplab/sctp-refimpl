 #-
 # Copyright (c) 2001-2007 by Cisco Systems, Inc. All rights reserved.
 # Copyright (c) 2001-2007, by Michael Tuexen, tuexen@fh-muenster.de. All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions are met:
 #
 # a) Redistributions of source code must retain the above copyright notice,
 #   this list of conditions and the following disclaimer.
 #
 # b) Redistributions in binary form must reproduce the above copyright
 #    notice, this list of conditions and the following disclaimer in
 #   the documentation and/or other materials provided with the distribution.
 #
 # c) Neither the name of Cisco Systems, Inc. nor the names of its
 #    contributors may be used to endorse or promote products derived
 #    from this software without specific prior written permission.
 #
 # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 # "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 # THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 # ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 # LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 # CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 # SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 # INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 # CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 # ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 # THE POSSIBILITY OF SUCH DAMAGE.
 #
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
