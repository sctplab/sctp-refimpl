/* $Id: auth2-pam.h,v 1.1.1.1 2004-06-23 13:07:28 randall Exp $ */

#include "includes.h"
#ifdef USE_PAM

int	auth2_pam(Authctxt *authctxt);

#endif /* USE_PAM */
