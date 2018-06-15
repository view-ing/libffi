#ifdef __x86_64__

/* -----------------------------------------------------------------*-C-*-
   bdffctarget.h - Copyright (c) 2012, 2014, 2018  Anthony Green
                 Copyright (c) 1996-2003, 2010  Red Hat, Inc.
                 Copyright (C) 2008  Free Software Foundation, Inc.

   Target configuration macros for x86 and x86-64.

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   ``Software''), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED ``AS IS'', WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.

   ----------------------------------------------------------------------- */

#ifndef LIBBDFFC_TARGET_H
#define LIBBDFFC_TARGET_H

#ifndef LIBBDFFC_H
#error "Please do not include bdffctarget.h directly into your source.  Use bdffc.h instead."
#endif

/* ---- System specific configurations ----------------------------------- */

/* For code common to all platforms on x86 and x86_64. */
#define X86_ANY

#if defined (X86_64) && defined (__i386__)
#undef X86_64
#define X86
#endif

#ifdef X86_WIN64
#define BDFFC_SIZEOF_ARG 8
#define USE_BUILTIN_FFS 0 /* not yet implemented in mingw-64 */
#endif

#define BDFFC_TARGET_SPECIFIC_STACK_SPACE_ALLOCATION
#ifndef _MSC_VER
#define BDFFC_TARGET_HAS_COMPLEX_TYPE
#endif

/* ---- Generic type definitions ----------------------------------------- */

#ifndef LIBBDFFC_ASM
#ifdef X86_WIN64
#ifdef _MSC_VER
typedef unsigned __int64       bdffc_arg;
typedef __int64                bdffc_sarg;
#else
typedef unsigned long long     bdffc_arg;
typedef long long              bdffc_sarg;
#endif
#else
#if defined __x86_64__ && defined __ILP32__
#define BDFFC_SIZEOF_ARG 8
#define BDFFC_SIZEOF_JAVA_RAW  4
typedef unsigned long long     bdffc_arg;
typedef long long              bdffc_sarg;
#else
typedef unsigned long          bdffc_arg;
typedef signed long            bdffc_sarg;
#endif
#endif

typedef enum bdffc_abi {
#if defined(X86_WIN64)
  BDFFC_FIRST_ABI = 0,
  BDFFC_WIN64,            /* sizeof(long double) == 8  - microsoft compilers */
  BDFFC_GNUW64,           /* sizeof(long double) == 16 - GNU compilers */
  BDFFC_LAST_ABI,
#ifdef __GNUC__
  BDFFC_DEFAULT_ABI = BDFFC_GNUW64
#else  
  BDFFC_DEFAULT_ABI = BDFFC_WIN64
#endif  

#elif defined(X86_64) || (defined (__x86_64__) && defined (X86_DARWIN))
  BDFFC_FIRST_ABI = 1,
  BDFFC_UNIX64,
  BDFFC_WIN64,
  BDFFC_EFI64 = BDFFC_WIN64,
  BDFFC_GNUW64,
  BDFFC_LAST_ABI,
  BDFFC_DEFAULT_ABI = BDFFC_UNIX64

#elif defined(X86_WIN32)
  BDFFC_FIRST_ABI = 0,
  BDFFC_SYSV      = 1,
  BDFFC_STDCALL   = 2,
  BDFFC_THISCALL  = 3,
  BDFFC_FASTCALL  = 4,
  BDFFC_MS_CDECL  = 5,
  BDFFC_PASCAL    = 6,
  BDFFC_REGISTER  = 7,
  BDFFC_LAST_ABI,
  BDFFC_DEFAULT_ABI = BDFFC_MS_CDECL
#else
  BDFFC_FIRST_ABI = 0,
  BDFFC_SYSV      = 1,
  BDFFC_THISCALL  = 3,
  BDFFC_FASTCALL  = 4,
  BDFFC_STDCALL   = 5,
  BDFFC_PASCAL    = 6,
  BDFFC_REGISTER  = 7,
  BDFFC_MS_CDECL  = 8,
  BDFFC_LAST_ABI,
  BDFFC_DEFAULT_ABI = BDFFC_SYSV
#endif
} bdffc_abi;
#endif

/* ---- Definitions for closures ----------------------------------------- */

#define BDFFC_CLOSURES 1
#define BDFFC_GO_CLOSURES 1

#define BDFFC_TYPE_SMALL_STRUCT_1B (BDFFC_TYPE_LAST + 1)
#define BDFFC_TYPE_SMALL_STRUCT_2B (BDFFC_TYPE_LAST + 2)
#define BDFFC_TYPE_SMALL_STRUCT_4B (BDFFC_TYPE_LAST + 3)
#define BDFFC_TYPE_MS_STRUCT       (BDFFC_TYPE_LAST + 4)

#if defined (X86_64) || defined(X86_WIN64) \
    || (defined (__x86_64__) && defined (X86_DARWIN))
# define BDFFC_TRAMPOLINE_SIZE 24
# define BDFFC_NATIVE_RAW_API 0
#else
# define BDFFC_TRAMPOLINE_SIZE 12
# define BDFFC_NATIVE_RAW_API 1  /* x86 has native raw api support */
#endif

#endif



#endif
