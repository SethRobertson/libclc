/*
 * (c) Copyright 1993 by Panagiotis Tsirigotis
 * All rights reserved.  The file named COPYRIGHT specifies the terms 
 * and conditions for redistribution.
 */

static const char RCSid[] = "$Id: dict.c,v 1.6 2003/04/01 04:40:56 seth Exp $";
static const char version[] = VERSION;

#include "clchack.h"
#include "dictimpl.h"
#include "fsma.h"

int dict_errno ;				/* Only for errors before we have a context */

struct name_value
{
  int nv_value ;
  char *nv_name ;
} ;


static struct name_value error_codes[] =
{
  {	DICT_ENOMEM,		"Out of Memory"					},
  {	DICT_ENOTFOUND,		"Object Not Found"				},
  {	DICT_ENOOOCOMP,		"Object-Object Comparison Function Missing"	},
  {	DICT_ENOKOCOMP,		"Key-Object Comparison Function Missing"	},
  {	DICT_ENULLOBJECT,	"Object is NULL"				},
  {	DICT_EEXISTS,		"Object Exists"					},
  {	DICT_ENOHVFUNC,		"Hashvalue Extraction Function Missing"	},
  {	DICT_EBADOBJECT,	"Object Does Not Exist"				},
  {	DICT_EBADORDER,		"Bad Order Flag"				},
  {	DICT_EORDER,		"Specified Order Not Supported"			},
  {	DICT_ENOERROR,		"No Error"					},
  {	-1,			NULL						}
} ;



/*
 * Errors before we have a context to HANDLE them directly
 */
dict_h __dict_create_error(char *caller, int flags, int error_code)
{
  dict_errno = error_code;
  return(NULL);
}



/*
 * Internal routine to verify create argument sanity
 */
int __dict_args_ok(char *caller, int flags, dict_function oo_comp, dict_function ko_comp, int allowed_orders, int *fsma_flags)
{
  int requested_order ;

  if (fsma_flags)
    *fsma_flags = 0;

  if ( BAD_ORDER( flags ) )
  {
    (void) __dict_create_error( caller, flags, DICT_EBADORDER ) ;
    return( FALSE ) ;
  }

  /*
   * If the user provided an object-object comparator, we can pretend
   * that the library supports the DICT_UNORDERED flag.
   */
  if ( oo_comp )
    allowed_orders |= DICT_UNORDERED ;

  requested_order = ( flags & ORDER_FLAGS ) ;
  if ( requested_order && ! ( allowed_orders & requested_order ) )
  {
    (void) __dict_create_error( caller, flags, DICT_EORDER ) ;
    return( FALSE ) ;
  }

  /*
   * An object-object comparator is required if
   *		order is requested,
   * or
   *		key uniqueness is requested
   * or
   *		the library requires it.
   */
  if ( oo_comp == NULL_FUNC && ( flags & (DICT_ORDERED | DICT_UNIQUE_KEYS) ) )
  {
    (void) __dict_create_error( caller, flags, DICT_ENOOOCOMP ) ;
    return( FALSE ) ;
  }

#ifdef notdef
  if ( ko_comp == NULL )
  {
    (void) __dict_create_error( caller, flags, DICT_ENOKOCOMP ) ;
    return( FALSE ) ;
  }
#endif

#ifdef HAVE_PTHREADS
  // Do not need both NOCOALESCE and THREADED_MEMORY
  if ( (flags & DICT_NOCOALESCE) && (flags & DICT_THREADED_MEMORY))
    flags &= ~DICT_THREADED_MEMORY;

  // Must have either NOCOALESCE or THREADED_MEMORY on with THREADED_SAFE
  if ((flags & DICT_THREADED_SAFE) && !((flags & DICT_NOCOALESCE) || (flags & DICT_THREADED_MEMORY)))
    flags |= DICT_THREADED_MEMORY;

  if (fsma_flags && (flags & DICT_THREADED_MEMORY))
    *fsma_flags = FSM_THREADED;
#endif /* HAVE_PTHREADS */

  if (fsma_flags && (flags & DICT_NOCOALESCE))
    *fsma_flags = FSM_NOCOALESCE;

  return( TRUE ) ;
}



/*
 * Internal routine to initialize some headers
 */
void __dict_init_header(dheader_s *dhp, dict_function oo_comp, dict_function ko_comp, int flags)
{
	dhp->oo_comp = oo_comp ;
	dhp->ko_comp = ko_comp ;
	dhp->flags = flags ;
	dhp->dicterrno = DICT_ENOERROR ;
}



/*
 * Internal routine to return text error string for an error
 */
char *__dict_error_reason(int dicterrno)
{
  int ctr;
  char *ret;

  for(ctr = 0; error_codes[ctr].nv_name && dicterrno != error_codes[ctr].nv_value; ctr++) ;

  if ((ret = error_codes[ctr].nv_name))
    return(ret);

  return("Unknown CLC error");
}
