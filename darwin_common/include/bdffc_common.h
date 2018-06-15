/* -----------------------------------------------------------------------
   bdffc_common.h - Copyright (C) 2011, 2012, 2013  Anthony Green
                  Copyright (C) 2007  Free Software Foundation, Inc
                  Copyright (c) 1996  Red Hat, Inc.
                  
   Common internal definitions and macros. Only necessary for building
   libbdffc.
   ----------------------------------------------------------------------- */

#ifndef BDFFC_COMMON_H
#define BDFFC_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <bdffcconfig.h>

/* Do not move this. Some versions of AIX are very picky about where
   this is positioned. */
#ifdef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
  /* mingw64 defines this already in malloc.h. */
#  ifndef alloca
#    define alloca __builtin_alloca
#  endif
# endif
# define MAYBE_UNUSED __attribute__((__unused__))
#else
# define MAYBE_UNUSED
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
#   pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
#    ifdef _MSC_VER
#     define alloca _alloca
#    else
char *alloca ();
#   endif
#  endif
# endif
# endif
#endif

/* Check for the existence of memcpy. */
#if STDC_HEADERS
# include <string.h>
#else
# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#if defined(BDFFC_DEBUG)
#include <stdio.h>
#endif

#ifdef BDFFC_DEBUG
void bdffc_assert(char *expr, char *file, int line);
void bdffc_stop_here(void);
void bdffc_type_test(bdffc_type *a, char *file, int line);

#define BDFFC_ASSERT(x) ((x) ? (void)0 : bdffc_assert(#x, __FILE__,__LINE__))
#define BDFFC_ASSERT_AT(x, f, l) ((x) ? 0 : bdffc_assert(#x, (f), (l)))
#define BDFFC_ASSERT_VALID_TYPE(x) bdffc_type_test (x, __FILE__, __LINE__)
#else
#define BDFFC_ASSERT(x)
#define BDFFC_ASSERT_AT(x, f, l)
#define BDFFC_ASSERT_VALID_TYPE(x)
#endif

/* v cast to size_t and aligned up to a multiple of a */
#define BDFFC_ALIGN(v, a)  (((((size_t) (v))-1) | ((a)-1))+1)
/* v cast to size_t and aligned down to a multiple of a */
#define ALIGN_DOWN(v, a) (((size_t) (v)) & -a)

/* Perform machine dependent cif processing */
bdffc_status bdffc_prep_cif_machdep(bdffc_cif *cif);
bdffc_status bdffc_prep_cif_machdep_var(bdffc_cif *cif,
	 unsigned int nfixedargs, unsigned int ntotalargs);


#if HAVE_LONG_DOUBLE_VARIANT
/* Used to adjust size/alignment of bdffc types.  */
void bdffc_prep_types (bdffc_abi abi);
#endif

/* Used internally, but overridden by some architectures */
bdffc_status bdffc_prep_cif_core(bdffc_cif *cif,
			     bdffc_abi abi,
			     unsigned int isvariadic,
			     unsigned int nfixedargs,
			     unsigned int ntotalargs,
			     bdffc_type *rtype,
			     bdffc_type **atypes);

/* Extended cif, used in callback from assembly routine */
typedef struct
{
  bdffc_cif *cif;
  void *rvalue;
  void **avalue;
} extended_cif;

/* Terse sized type definitions.  */
#if defined(_MSC_VER) || defined(__sgi) || defined(__SUNPRO_C)
typedef unsigned char UINT8;
typedef signed char   SINT8;
typedef unsigned short UINT16;
typedef signed short   SINT16;
typedef unsigned int UINT32;
typedef signed int   SINT32;
# ifdef _MSC_VER
typedef unsigned __int64 UINT64;
typedef signed __int64   SINT64;
# else
# include <inttypes.h>
typedef uint64_t UINT64;
typedef int64_t  SINT64;
# endif
#else
typedef unsigned int UINT8  __attribute__((__mode__(__QI__)));
typedef signed int   SINT8  __attribute__((__mode__(__QI__)));
typedef unsigned int UINT16 __attribute__((__mode__(__HI__)));
typedef signed int   SINT16 __attribute__((__mode__(__HI__)));
typedef unsigned int UINT32 __attribute__((__mode__(__SI__)));
typedef signed int   SINT32 __attribute__((__mode__(__SI__)));
typedef unsigned int UINT64 __attribute__((__mode__(__DI__)));
typedef signed int   SINT64 __attribute__((__mode__(__DI__)));
#endif

typedef float FLOAT32;

#ifndef __GNUC__
#define __builtin_expect(x, expected_value) (x)
#endif
#define LIKELY(x)    __builtin_expect(!!(x),1)
#define UNLIKELY(x)  __builtin_expect((x)!=0,0)

#ifdef __cplusplus
}
#endif

#endif
