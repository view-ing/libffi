#ifdef __arm64__

/* -----------------------------------------------------------------*-C-*-
   libbdffc 3.3-rc0 - Copyright (c) 2011, 2014 Anthony Green
                    - Copyright (c) 1996-2003, 2007, 2008 Red Hat, Inc.

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the ``Software''), to deal in the Software without
   restriction, including without limitation the rights to use, copy,
   modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED ``AS IS'', WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.

   ----------------------------------------------------------------------- */

/* -------------------------------------------------------------------
   Most of the API is documented in doc/libbdffc.texi.

   The raw API is designed to bypass some of the argument packing and
   unpacking on architectures for which it can be avoided.  Routines
   are provided to emulate the raw API if the underlying platform
   doesn't allow faster implementation.

   More details on the raw API can be found in:

   http://gcc.gnu.org/ml/java/1999-q3/msg00138.html

   and

   http://gcc.gnu.org/ml/java/1999-q3/msg00174.html
   -------------------------------------------------------------------- */

#ifndef LIBBDFFC_H
#define LIBBDFFC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Specify which architecture libbdffc is configured for. */
#ifndef AARCH64
#define AARCH64
#endif

/* ---- System configuration information --------------------------------- */

#include <bdffctarget.h>

#ifndef LIBBDFFC_ASM

#if defined(_MSC_VER) && !defined(__clang__)
#define __attribute__(X)
#endif

#include <stddef.h>
#include <limits.h>

/* LONG_LONG_MAX is not always defined (not if STRICT_ANSI, for example).
   But we can find it either under the correct ANSI name, or under GNU
   C's internal name.  */

#define BDFFC_64_BIT_MAX 9223372036854775807

#ifdef LONG_LONG_MAX
# define BDFFC_LONG_LONG_MAX LONG_LONG_MAX
#else
# ifdef LLONG_MAX
#  define BDFFC_LONG_LONG_MAX LLONG_MAX
#  ifdef _AIX52 /* or newer has C99 LLONG_MAX */
#   undef BDFFC_64_BIT_MAX
#   define BDFFC_64_BIT_MAX 9223372036854775807LL
#  endif /* _AIX52 or newer */
# else
#  ifdef __GNUC__
#   define BDFFC_LONG_LONG_MAX __LONG_LONG_MAX__
#  endif
#  ifdef _AIX /* AIX 5.1 and earlier have LONGLONG_MAX */
#   ifndef __PPC64__
#    if defined (__IBMC__) || defined (__IBMCPP__)
#     define BDFFC_LONG_LONG_MAX LONGLONG_MAX
#    endif
#   endif /* __PPC64__ */
#   undef  BDFFC_64_BIT_MAX
#   define BDFFC_64_BIT_MAX 9223372036854775807LL
#  endif
# endif
#endif

/* The closure code assumes that this works on pointers, i.e. a size_t
   can hold a pointer.  */

typedef struct _bdffc_type
{
  size_t size;
  unsigned short alignment;
  unsigned short type;
  struct _bdffc_type **elements;
} bdffc_type;

/* Need minimal decorations for DLLs to work on Windows.  GCC has
   autoimport and autoexport.  Always mark externally visible symbols
   as dllimport for MSVC clients, even if it means an extra indirection
   when using the static version of the library.
   Besides, as a workaround, they can define BDFFC_BUILDING if they
   *know* they are going to link with the static library.  */
#if defined _MSC_VER
# if defined BDFFC_BUILDING_DLL /* Building libbdffc.DLL with msvcc.sh */
#  define BDFFC_API __declspec(dllexport)
# elif !defined BDFFC_BUILDING  /* Importing libbdffc.DLL */
#  define BDFFC_API __declspec(dllimport)
# else                        /* Building/linking static library */
#  define BDFFC_API
# endif
#else
# define BDFFC_API
#endif

/* The externally visible type declarations also need the MSVC DLL
   decorations, or they will not be exported from the object file.  */
#if defined LIBBDFFC_HIDE_BASIC_TYPES
# define BDFFC_EXTERN BDFFC_API
#else
# define BDFFC_EXTERN extern BDFFC_API
#endif

#ifndef LIBBDFFC_HIDE_BASIC_TYPES
#if SCHAR_MAX == 127
# define bdffc_type_uchar                bdffc_type_uint8
# define bdffc_type_schar                bdffc_type_sint8
#else
 #error "char size not supported"
#endif

#if SHRT_MAX == 32767
# define bdffc_type_ushort       bdffc_type_uint16
# define bdffc_type_sshort       bdffc_type_sint16
#elif SHRT_MAX == 2147483647
# define bdffc_type_ushort       bdffc_type_uint32
# define bdffc_type_sshort       bdffc_type_sint32
#else
 #error "short size not supported"
#endif

#if INT_MAX == 32767
# define bdffc_type_uint         bdffc_type_uint16
# define bdffc_type_sint         bdffc_type_sint16
#elif INT_MAX == 2147483647
# define bdffc_type_uint         bdffc_type_uint32
# define bdffc_type_sint         bdffc_type_sint32
#elif INT_MAX == 9223372036854775807
# define bdffc_type_uint         bdffc_type_uint64
# define bdffc_type_sint         bdffc_type_sint64
#else
 #error "int size not supported"
#endif

#if LONG_MAX == 2147483647
# if BDFFC_LONG_LONG_MAX != BDFFC_64_BIT_MAX
 #error "no 64-bit data type supported"
# endif
#elif LONG_MAX != BDFFC_64_BIT_MAX
 #error "long size not supported"
#endif

#if LONG_MAX == 2147483647
# define bdffc_type_ulong        bdffc_type_uint32
# define bdffc_type_slong        bdffc_type_sint32
#elif LONG_MAX == BDFFC_64_BIT_MAX
# define bdffc_type_ulong        bdffc_type_uint64
# define bdffc_type_slong        bdffc_type_sint64
#else
 #error "long size not supported"
#endif

/* These are defined in types.c.  */
BDFFC_EXTERN bdffc_type bdffc_type_void;
BDFFC_EXTERN bdffc_type bdffc_type_uint8;
BDFFC_EXTERN bdffc_type bdffc_type_sint8;
BDFFC_EXTERN bdffc_type bdffc_type_uint16;
BDFFC_EXTERN bdffc_type bdffc_type_sint16;
BDFFC_EXTERN bdffc_type bdffc_type_uint32;
BDFFC_EXTERN bdffc_type bdffc_type_sint32;
BDFFC_EXTERN bdffc_type bdffc_type_uint64;
BDFFC_EXTERN bdffc_type bdffc_type_sint64;
BDFFC_EXTERN bdffc_type bdffc_type_float;
BDFFC_EXTERN bdffc_type bdffc_type_double;
BDFFC_EXTERN bdffc_type bdffc_type_pointer;

#if 0
BDFFC_EXTERN bdffc_type bdffc_type_longdouble;
#else
#define bdffc_type_longdouble bdffc_type_double
#endif

#ifdef BDFFC_TARGET_HAS_COMPLEX_TYPE
BDFFC_EXTERN bdffc_type bdffc_type_complex_float;
BDFFC_EXTERN bdffc_type bdffc_type_complex_double;
#if 0
BDFFC_EXTERN bdffc_type bdffc_type_complex_longdouble;
#else
#define bdffc_type_complex_longdouble bdffc_type_complex_double
#endif
#endif
#endif /* LIBBDFFC_HIDE_BASIC_TYPES */

typedef enum {
  BDFFC_OK = 0,
  BDFFC_BAD_TYPEDEF,
  BDFFC_BAD_ABI
} bdffc_status;

typedef struct {
  bdffc_abi abi;
  unsigned nargs;
  bdffc_type **arg_types;
  bdffc_type *rtype;
  unsigned bytes;
  unsigned flags;
#ifdef BDFFC_EXTRA_CIF_FIELDS
  BDFFC_EXTRA_CIF_FIELDS;
#endif
} bdffc_cif;

/* ---- Definitions for the raw API -------------------------------------- */

#ifndef BDFFC_SIZEOF_ARG
# if LONG_MAX == 2147483647
#  define BDFFC_SIZEOF_ARG        4
# elif LONG_MAX == BDFFC_64_BIT_MAX
#  define BDFFC_SIZEOF_ARG        8
# endif
#endif

#ifndef BDFFC_SIZEOF_JAVA_RAW
#  define BDFFC_SIZEOF_JAVA_RAW BDFFC_SIZEOF_ARG
#endif

typedef union {
  bdffc_sarg  sint;
  bdffc_arg   uint;
  float	    flt;
  char      data[BDFFC_SIZEOF_ARG];
  void*     ptr;
} bdffc_raw;

#if BDFFC_SIZEOF_JAVA_RAW == 4 && BDFFC_SIZEOF_ARG == 8
/* This is a special case for mips64/n32 ABI (and perhaps others) where
   sizeof(void *) is 4 and BDFFC_SIZEOF_ARG is 8.  */
typedef union {
  signed int	sint;
  unsigned int	uint;
  float		flt;
  char		data[BDFFC_SIZEOF_JAVA_RAW];
  void*		ptr;
} bdffc_java_raw;
#else
typedef bdffc_raw bdffc_java_raw;
#endif


BDFFC_API 
void bdffc_raw_call (bdffc_cif *cif,
		   void (*fn)(void),
		   void *rvalue,
		   bdffc_raw *avalue);

BDFFC_API void bdffc_ptrarray_to_raw (bdffc_cif *cif, void **args, bdffc_raw *raw);
BDFFC_API void bdffc_raw_to_ptrarray (bdffc_cif *cif, bdffc_raw *raw, void **args);
BDFFC_API size_t bdffc_raw_size (bdffc_cif *cif);

/* This is analogous to the raw API, except it uses Java parameter
   packing, even on 64-bit machines.  I.e. on 64-bit machines longs
   and doubles are followed by an empty 64-bit word.  */

BDFFC_API
void bdffc_java_raw_call (bdffc_cif *cif,
			void (*fn)(void),
			void *rvalue,
			bdffc_java_raw *avalue);

BDFFC_API
void bdffc_java_ptrarray_to_raw (bdffc_cif *cif, void **args, bdffc_java_raw *raw);
BDFFC_API
void bdffc_java_raw_to_ptrarray (bdffc_cif *cif, bdffc_java_raw *raw, void **args);
BDFFC_API
size_t bdffc_java_raw_size (bdffc_cif *cif);

/* ---- Definitions for closures ----------------------------------------- */

#if BDFFC_CLOSURES

#ifdef _MSC_VER
__declspec(align(8))
#endif
typedef struct {
#if 1
  void *trampoline_table;
  void *trampoline_table_entry;
#else
  char tramp[BDFFC_TRAMPOLINE_SIZE];
#endif
  bdffc_cif   *cif;
  void     (*fun)(bdffc_cif*,void*,void**,void*);
  void      *user_data;
} bdffc_closure
#ifdef __GNUC__
    __attribute__((aligned (8)))
#endif
    ;

#ifndef __GNUC__
# ifdef __sgi
#  pragma pack 0
# endif
#endif

BDFFC_API void *bdffc_closure_alloc (size_t size, void **code);
BDFFC_API void bdffc_closure_free (void *);

BDFFC_API bdffc_status
bdffc_prep_closure (bdffc_closure*,
		  bdffc_cif *,
		  void (*fun)(bdffc_cif*,void*,void**,void*),
		  void *user_data)
#if defined(__GNUC__) && (((__GNUC__ * 100) + __GNUC_MINOR__) >= 405)
  __attribute__((deprecated ("use bdffc_prep_closure_loc instead")))
#elif defined(__GNUC__) && __GNUC__ >= 3
  __attribute__((deprecated))
#endif
  ;

BDFFC_API bdffc_status
bdffc_prep_closure_loc (bdffc_closure*,
		      bdffc_cif *,
		      void (*fun)(bdffc_cif*,void*,void**,void*),
		      void *user_data,
		      void*codeloc);

#ifdef __sgi
# pragma pack 8
#endif
typedef struct {
#if 1
  void *trampoline_table;
  void *trampoline_table_entry;
#else
  char tramp[BDFFC_TRAMPOLINE_SIZE];
#endif
  bdffc_cif   *cif;

#if !BDFFC_NATIVE_RAW_API

  /* If this is enabled, then a raw closure has the same layout 
     as a regular closure.  We use this to install an intermediate 
     handler to do the transaltion, void** -> bdffc_raw*.  */

  void     (*translate_args)(bdffc_cif*,void*,void**,void*);
  void      *this_closure;

#endif

  void     (*fun)(bdffc_cif*,void*,bdffc_raw*,void*);
  void      *user_data;

} bdffc_raw_closure;

typedef struct {
#if 1
  void *trampoline_table;
  void *trampoline_table_entry;
#else
  char tramp[BDFFC_TRAMPOLINE_SIZE];
#endif

  bdffc_cif   *cif;

#if !BDFFC_NATIVE_RAW_API

  /* If this is enabled, then a raw closure has the same layout 
     as a regular closure.  We use this to install an intermediate 
     handler to do the translation, void** -> bdffc_raw*.  */

  void     (*translate_args)(bdffc_cif*,void*,void**,void*);
  void      *this_closure;

#endif

  void     (*fun)(bdffc_cif*,void*,bdffc_java_raw*,void*);
  void      *user_data;

} bdffc_java_raw_closure;

BDFFC_API bdffc_status
bdffc_prep_raw_closure (bdffc_raw_closure*,
		      bdffc_cif *cif,
		      void (*fun)(bdffc_cif*,void*,bdffc_raw*,void*),
		      void *user_data);

BDFFC_API bdffc_status
bdffc_prep_raw_closure_loc (bdffc_raw_closure*,
			  bdffc_cif *cif,
			  void (*fun)(bdffc_cif*,void*,bdffc_raw*,void*),
			  void *user_data,
			  void *codeloc);

BDFFC_API bdffc_status
bdffc_prep_java_raw_closure (bdffc_java_raw_closure*,
		           bdffc_cif *cif,
		           void (*fun)(bdffc_cif*,void*,bdffc_java_raw*,void*),
		           void *user_data);

BDFFC_API bdffc_status
bdffc_prep_java_raw_closure_loc (bdffc_java_raw_closure*,
			       bdffc_cif *cif,
			       void (*fun)(bdffc_cif*,void*,bdffc_java_raw*,void*),
			       void *user_data,
			       void *codeloc);

#endif /* BDFFC_CLOSURES */

#if BDFFC_GO_CLOSURES

typedef struct {
  void      *tramp;
  bdffc_cif   *cif;
  void     (*fun)(bdffc_cif*,void*,void**,void*);
} bdffc_go_closure;

BDFFC_API bdffc_status bdffc_prep_go_closure (bdffc_go_closure*, bdffc_cif *,
				void (*fun)(bdffc_cif*,void*,void**,void*));

BDFFC_API void bdffc_call_go (bdffc_cif *cif, void (*fn)(void), void *rvalue,
		  void **avalue, void *closure);

#endif /* BDFFC_GO_CLOSURES */

/* ---- Public interface definition -------------------------------------- */

BDFFC_API 
bdffc_status bdffc_prep_cif(bdffc_cif *cif,
			bdffc_abi abi,
			unsigned int nargs,
			bdffc_type *rtype,
			bdffc_type **atypes);

BDFFC_API
bdffc_status bdffc_prep_cif_var(bdffc_cif *cif,
			    bdffc_abi abi,
			    unsigned int nfixedargs,
			    unsigned int ntotalargs,
			    bdffc_type *rtype,
			    bdffc_type **atypes);

BDFFC_API
void bdffc_call(bdffc_cif *cif,
	      void (*fn)(void),
	      void *rvalue,
	      void **avalue);

BDFFC_API
bdffc_status bdffc_get_struct_offsets (bdffc_abi abi, bdffc_type *struct_type,
				   size_t *offsets);

/* Useful for eliminating compiler warnings.  */
#define BDFFC_FN(f) ((void (*)(void))f)

/* ---- Definitions shared with assembly code ---------------------------- */

#endif

/* If these change, update src/mips/bdffctarget.h. */
#define BDFFC_TYPE_VOID       0    
#define BDFFC_TYPE_INT        1
#define BDFFC_TYPE_FLOAT      2    
#define BDFFC_TYPE_DOUBLE     3
#if 0
#define BDFFC_TYPE_LONGDOUBLE 4
#else
#define BDFFC_TYPE_LONGDOUBLE BDFFC_TYPE_DOUBLE
#endif
#define BDFFC_TYPE_UINT8      5   
#define BDFFC_TYPE_SINT8      6
#define BDFFC_TYPE_UINT16     7 
#define BDFFC_TYPE_SINT16     8
#define BDFFC_TYPE_UINT32     9
#define BDFFC_TYPE_SINT32     10
#define BDFFC_TYPE_UINT64     11
#define BDFFC_TYPE_SINT64     12
#define BDFFC_TYPE_STRUCT     13
#define BDFFC_TYPE_POINTER    14
#define BDFFC_TYPE_COMPLEX    15

/* This should always refer to the last type code (for sanity checks).  */
#define BDFFC_TYPE_LAST       BDFFC_TYPE_COMPLEX

#ifdef __cplusplus
}
#endif

#endif


#endif
