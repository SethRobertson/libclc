#ifndef _CLCHACK_H
#define _CLCHACK_H

#ifdef USING_DMALLOC
#include <dmalloc.h>
#endif

/* Prevent DEAD_CODE(noeffect) Insight warnings for va_end() */
#if defined(__INSIGHT__)&&defined(__i386__)&&defined(__GNUC__)&&defined(va_end)
#undef va_end
#define va_end(AP)
#endif

#endif /* _CLCHACK_H */
