#ifndef _CLCHACK_H
#define _CLCHACK_H

#ifdef HAVE_PTHREADS
#define _REENTRANT
#include <pthread.h>
#endif /* HAVE_PTHREADS */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#ifdef DEBUG
#include <stdio.h>
#endif

#ifdef USING_DMALLOC
#include <dmalloc.h>
#endif

#endif /* _CLCHACK_H */
