/* SCTP reference Implementation Copyright (C) 2001 Cisco

This file is part of the SCTP reference Implementation

Version:4.0.5
$Header: /usr/sctpCVS/rserpool/lib/return_status.h,v 1.1 2006-05-30 18:02:03 randall Exp $


The SCTP reference implementation  is free software; 
you can redistribute it and/or modify it under the terms of 
the GNU Library General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

the SCTP reference implementation  is distributed in the hope that it 
will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
                ************************
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Library General Public License for more details.

You should have received a copy of the GNU Library General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  

Please send any bug reports or fixes you make to one of the following email
addresses:

rrs@cisco.com
kmorneau@cisco.com

Any bugs reported given to us we will try to fix... any fixes shared will
be incorperated into the next SCTP release.

There are still LOTS of bugs in this code... I always run on the motto
"it is a wonder any code ever works :)"


*/


#ifndef __return_status_h__
#define __return_status_h__

#define LIB_STATUS_GOOD  0
#define LIB_STATUS_BAD  -1
#define LIB_STATUS_BAD1 -2
#define LIB_STATUS_BAD2 -3
#define LIB_STATUS_BAD3 -4
#define STATUS_BAD(status) (status < 0)

#endif
