/* $Id: setenv.h,v 1.1.1.1 2004-06-23 13:07:28 randall Exp $ */

#ifndef _BSD_SETENV_H
#define _BSD_SETENV_H

#include "config.h"

#ifndef HAVE_SETENV

int setenv(register const char *name, register const char *value, int rewrite);

#endif /* !HAVE_SETENV */

#endif /* _BSD_SETENV_H */
