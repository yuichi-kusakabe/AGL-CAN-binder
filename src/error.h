#ifndef _ERROR_H_
#define _ERROR_H_
#ifndef OWN_TEST
#include "af-canivi-binding.h"
#endif
#include <errno.h>
#ifndef ERRMSG
#define ERRMSG(msg,...) fprintf(stderr,"[Err] " msg "\n",##__VA_ARGS__)
#endif /*ERRMSG */
#ifndef NOTICEMSG
#define NOTICEMSG(msg,...) fprintf(stderr,"[Notice] " msg "\n",##__VA_ARGS__)
#endif /*ERRMSG */
#ifdef DEBUG
#ifndef DBGMSG
#define DBGMSG(msg,...) printf("[DBG] " msg "\n",##__VA_ARGS__)
#endif
#else /* !DEBUG */
#define DBGMSG(msg,...) 
#endif /* DEBUG */
#endif

