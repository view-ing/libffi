#ifdef __arm64__

/* Copyright (c) 2009, 2010, 2011, 2012 ARM Ltd.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
``Software''), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED ``AS IS'', WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

#ifndef LIBBDFFC_TARGET_H
#define LIBBDFFC_TARGET_H

#ifndef LIBBDFFC_H
#error "Please do not include bdffctarget.h directly into your source.  Use bdffc.h instead."
#endif

#ifndef LIBBDFFC_ASM
#ifdef __ILP32__
#define BDFFC_SIZEOF_ARG 8
#define BDFFC_SIZEOF_JAVA_RAW  4
typedef unsigned long long bdffc_arg;
typedef signed long long bdffc_sarg;
#else
typedef unsigned long bdffc_arg;
typedef signed long bdffc_sarg;
#endif

typedef enum bdffc_abi
  {
    BDFFC_FIRST_ABI = 0,
    BDFFC_SYSV,
    BDFFC_LAST_ABI,
    BDFFC_DEFAULT_ABI = BDFFC_SYSV
  } bdffc_abi;
#endif

/* ---- Definitions for closures ----------------------------------------- */

#define BDFFC_CLOSURES 1
#define BDFFC_NATIVE_RAW_API 0

#if defined (BDFFC_EXEC_TRAMPOLINE_TABLE) && BDFFC_EXEC_TRAMPOLINE_TABLE

#ifdef __MACH__
#define BDFFC_TRAMPOLINE_SIZE 16
#define BDFFC_TRAMPOLINE_CLOSURE_OFFSET 16
#else
#error "No trampoline table implementation"
#endif

#else
#define BDFFC_TRAMPOLINE_SIZE 24
#define BDFFC_TRAMPOLINE_CLOSURE_OFFSET BDFFC_TRAMPOLINE_SIZE
#endif

/* ---- Internal ---- */

#if defined (__APPLE__)
#define BDFFC_TARGET_SPECIFIC_VARIADIC
#define BDFFC_EXTRA_CIF_FIELDS unsigned aarch64_nfixedargs
#else
/* iOS reserves x18 for the system.  Disable Go closures until
   a new static chain is chosen.  */
#define BDFFC_GO_CLOSURES 1
#endif

#define BDFFC_TARGET_HAS_COMPLEX_TYPE

#endif


#endif
