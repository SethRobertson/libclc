#ifndef _CLCHACK_H
#define _CLCHACK_H

#ifdef BK_USING_PTHREADS
#include <pthread.h>
#endif /* BK_USING_PTHREADS */

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
