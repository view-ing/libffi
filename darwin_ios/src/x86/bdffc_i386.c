#ifdef __i386__

/* -----------------------------------------------------------------------
   bdffc.c - Copyright (c) 2017  Anthony Green
           Copyright (c) 1996, 1998, 1999, 2001, 2007, 2008  Red Hat, Inc.
           Copyright (c) 2002  Ranjit Mathew
           Copyright (c) 2002  Bo Thorsen
           Copyright (c) 2002  Roger Sayle
           Copyright (C) 2008, 2010  Free Software Foundation, Inc.

   x86 Foreign Function Interface

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

#ifndef __x86_64__
#include <bdffc.h>
#include <bdffc_common.h>
#include <stdint.h>
#include <stdlib.h>
#include "internal.h"

/* Force BDFFC_TYPE_LONGDOUBLE to be different than BDFFC_TYPE_DOUBLE;
   all further uses in this file will refer to the 80-bit type.  */
#if BDFFC_TYPE_LONGDOUBLE != BDFFC_TYPE_DOUBLE
# if BDFFC_TYPE_LONGDOUBLE != 4
#  error BDFFC_TYPE_LONGDOUBLE out of date
# endif
#else
# undef BDFFC_TYPE_LONGDOUBLE
# define BDFFC_TYPE_LONGDOUBLE 4
#endif

#if defined(__GNUC__) && !defined(__declspec)
# define __declspec(x)  __attribute__((x))
#endif

/* Perform machine dependent cif processing.  */
bdffc_status BDFFC_HIDDEN
bdffc_prep_cif_machdep(bdffc_cif *cif)
{
  size_t bytes = 0;
  int i, n, flags, cabi = cif->abi;

  switch (cabi)
    {
    case BDFFC_SYSV:
    case BDFFC_STDCALL:
    case BDFFC_THISCALL:
    case BDFFC_FASTCALL:
    case BDFFC_MS_CDECL:
    case BDFFC_PASCAL:
    case BDFFC_REGISTER:
      break;
    default:
      return BDFFC_BAD_ABI;
    }

  switch (cif->rtype->type)
    {
    case BDFFC_TYPE_VOID:
      flags = X86_RET_VOID;
      break;
    case BDFFC_TYPE_FLOAT:
      flags = X86_RET_FLOAT;
      break;
    case BDFFC_TYPE_DOUBLE:
      flags = X86_RET_DOUBLE;
      break;
    case BDFFC_TYPE_LONGDOUBLE:
      flags = X86_RET_LDOUBLE;
      break;
    case BDFFC_TYPE_UINT8:
      flags = X86_RET_UINT8;
      break;
    case BDFFC_TYPE_UINT16:
      flags = X86_RET_UINT16;
      break;
    case BDFFC_TYPE_SINT8:
      flags = X86_RET_SINT8;
      break;
    case BDFFC_TYPE_SINT16:
      flags = X86_RET_SINT16;
      break;
    case BDFFC_TYPE_INT:
    case BDFFC_TYPE_SINT32:
    case BDFFC_TYPE_UINT32:
    case BDFFC_TYPE_POINTER:
      flags = X86_RET_INT32;
      break;
    case BDFFC_TYPE_SINT64:
    case BDFFC_TYPE_UINT64:
      flags = X86_RET_INT64;
      break;
    case BDFFC_TYPE_STRUCT:
#ifndef X86
      /* ??? This should be a different ABI rather than an ifdef.  */
      if (cif->rtype->size == 1)
	flags = X86_RET_STRUCT_1B;
      else if (cif->rtype->size == 2)
	flags = X86_RET_STRUCT_2B;
      else if (cif->rtype->size == 4)
	flags = X86_RET_INT32;
      else if (cif->rtype->size == 8)
	flags = X86_RET_INT64;
      else
#endif
	{
	do_struct:
	  switch (cabi)
	    {
	    case BDFFC_THISCALL:
	    case BDFFC_FASTCALL:
	    case BDFFC_STDCALL:
	    case BDFFC_MS_CDECL:
	      flags = X86_RET_STRUCTARG;
	      break;
	    default:
	      flags = X86_RET_STRUCTPOP;
	      break;
	    }
	  /* Allocate space for return value pointer.  */
	  bytes += BDFFC_ALIGN (sizeof(void*), BDFFC_SIZEOF_ARG);
	}
      break;
    case BDFFC_TYPE_COMPLEX:
      switch (cif->rtype->elements[0]->type)
	{
	case BDFFC_TYPE_DOUBLE:
	case BDFFC_TYPE_LONGDOUBLE:
	case BDFFC_TYPE_SINT64:
	case BDFFC_TYPE_UINT64:
	  goto do_struct;
	case BDFFC_TYPE_FLOAT:
	case BDFFC_TYPE_INT:
	case BDFFC_TYPE_SINT32:
	case BDFFC_TYPE_UINT32:
	  flags = X86_RET_INT64;
	  break;
	case BDFFC_TYPE_SINT16:
	case BDFFC_TYPE_UINT16:
	  flags = X86_RET_INT32;
	  break;
	case BDFFC_TYPE_SINT8:
	case BDFFC_TYPE_UINT8:
	  flags = X86_RET_STRUCT_2B;
	  break;
	default:
	  return BDFFC_BAD_TYPEDEF;
	}
      break;
    default:
      return BDFFC_BAD_TYPEDEF;
    }
  cif->flags = flags;

  for (i = 0, n = cif->nargs; i < n; i++)
    {
      bdffc_type *t = cif->arg_types[i];

      bytes = BDFFC_ALIGN (bytes, t->alignment);
      bytes += BDFFC_ALIGN (t->size, BDFFC_SIZEOF_ARG);
    }
  cif->bytes = BDFFC_ALIGN (bytes, 16);

  return BDFFC_OK;
}

static bdffc_arg
extend_basic_type(void *arg, int type)
{
  switch (type)
    {
    case BDFFC_TYPE_SINT8:
      return *(SINT8 *)arg;
    case BDFFC_TYPE_UINT8:
      return *(UINT8 *)arg;
    case BDFFC_TYPE_SINT16:
      return *(SINT16 *)arg;
    case BDFFC_TYPE_UINT16:
      return *(UINT16 *)arg;

    case BDFFC_TYPE_SINT32:
    case BDFFC_TYPE_UINT32:
    case BDFFC_TYPE_POINTER:
    case BDFFC_TYPE_FLOAT:
      return *(UINT32 *)arg;

    default:
      abort();
    }
}

struct call_frame
{
  void *ebp;		/* 0 */
  void *retaddr;	/* 4 */
  void (*fn)(void);	/* 8 */
  int flags;		/* 12 */
  void *rvalue;		/* 16 */
  unsigned regs[3];	/* 20-28 */
};

struct abi_params
{
  int dir;		/* parameter growth direction */
  int static_chain;	/* the static chain register used by gcc */
  int nregs;		/* number of register parameters */
  int regs[3];
};

static const struct abi_params abi_params[BDFFC_LAST_ABI] = {
  [BDFFC_SYSV] = { 1, R_ECX, 0 },
  [BDFFC_THISCALL] = { 1, R_EAX, 1, { R_ECX } },
  [BDFFC_FASTCALL] = { 1, R_EAX, 2, { R_ECX, R_EDX } },
  [BDFFC_STDCALL] = { 1, R_ECX, 0 },
  [BDFFC_PASCAL] = { -1, R_ECX, 0 },
  /* ??? No defined static chain; gcc does not support REGISTER.  */
  [BDFFC_REGISTER] = { -1, R_ECX, 3, { R_EAX, R_EDX, R_ECX } },
  [BDFFC_MS_CDECL] = { 1, R_ECX, 0 }
};

#ifdef HAVE_FASTCALL
  #ifdef _MSC_VER
    #define BDFFC_DECLARE_FASTCALL __fastcall
  #else
    #define BDFFC_DECLARE_FASTCALL __declspec(fastcall)
  #endif
#else
  #define BDFFC_DECLARE_FASTCALL
#endif

extern void BDFFC_DECLARE_FASTCALL bdffc_call_i386(struct call_frame *, char *) BDFFC_HIDDEN;

static void
bdffc_call_int (bdffc_cif *cif, void (*fn)(void), void *rvalue,
	      void **avalue, void *closure)
{
  size_t rsize, bytes;
  struct call_frame *frame;
  char *stack, *argp;
  bdffc_type **arg_types;
  int flags, cabi, i, n, dir, narg_reg;
  const struct abi_params *pabi;

  flags = cif->flags;
  cabi = cif->abi;
  pabi = &abi_params[cabi];
  dir = pabi->dir;

  rsize = 0;
  if (rvalue == NULL)
    {
      switch (flags)
	{
	case X86_RET_FLOAT:
	case X86_RET_DOUBLE:
	case X86_RET_LDOUBLE:
	case X86_RET_STRUCTPOP:
	case X86_RET_STRUCTARG:
	  /* The float cases need to pop the 387 stack.
	     The struct cases need to pass a valid pointer to the callee.  */
	  rsize = cif->rtype->size;
	  break;
	default:
	  /* We can pretend that the callee returns nothing.  */
	  flags = X86_RET_VOID;
	  break;
	}
    }

  bytes = cif->bytes;
  stack = alloca(bytes + sizeof(*frame) + rsize);
  argp = (dir < 0 ? stack + bytes : stack);
  frame = (struct call_frame *)(stack + bytes);
  if (rsize)
    rvalue = frame + 1;

  frame->fn = fn;
  frame->flags = flags;
  frame->rvalue = rvalue;
  frame->regs[pabi->static_chain] = (unsigned)closure;

  narg_reg = 0;
  switch (flags)
    {
    case X86_RET_STRUCTARG:
      /* The pointer is passed as the first argument.  */
      if (pabi->nregs > 0)
	{
	  frame->regs[pabi->regs[0]] = (unsigned)rvalue;
	  narg_reg = 1;
	  break;
	}
      /* fallthru */
    case X86_RET_STRUCTPOP:
      *(void **)argp = rvalue;
      argp += sizeof(void *);
      break;
    }

  arg_types = cif->arg_types;
  for (i = 0, n = cif->nargs; i < n; i++)
    {
      bdffc_type *ty = arg_types[i];
      void *valp = avalue[i];
      size_t z = ty->size;
      int t = ty->type;

      if (z <= BDFFC_SIZEOF_ARG && t != BDFFC_TYPE_STRUCT)
        {
	  bdffc_arg val = extend_basic_type (valp, t);

	  if (t != BDFFC_TYPE_FLOAT && narg_reg < pabi->nregs)
	    frame->regs[pabi->regs[narg_reg++]] = val;
	  else if (dir < 0)
	    {
	      argp -= 4;
	      *(bdffc_arg *)argp = val;
	    }
	  else
	    {
	      *(bdffc_arg *)argp = val;
	      argp += 4;
	    }
	}
      else
	{
	  size_t za = BDFFC_ALIGN (z, BDFFC_SIZEOF_ARG);
	  size_t align = BDFFC_SIZEOF_ARG;

	  /* Alignment rules for arguments are quite complex.  Vectors and
	     structures with 16 byte alignment get it.  Note that long double
	     on Darwin does have 16 byte alignment, and does not get this
	     alignment if passed directly; a structure with a long double
	     inside, however, would get 16 byte alignment.  Since libbdffc does
	     not support vectors, we need non concern ourselves with other
	     cases.  */
	  if (t == BDFFC_TYPE_STRUCT && ty->alignment >= 16)
	    align = 16;
	    
	  if (dir < 0)
	    {
	      /* ??? These reverse argument ABIs are probably too old
		 to have cared about alignment.  Someone should check.  */
	      argp -= za;
	      memcpy (argp, valp, z);
	    }
	  else
	    {
	      argp = (char *)BDFFC_ALIGN (argp, align);
	      memcpy (argp, valp, z);
	      argp += za;
	    }
	}
    }
  BDFFC_ASSERT (dir > 0 || argp == stack);

  bdffc_call_i386 (frame, stack);
}

void
bdffc_call (bdffc_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  bdffc_call_int (cif, fn, rvalue, avalue, NULL);
}

void
bdffc_call_go (bdffc_cif *cif, void (*fn)(void), void *rvalue,
	     void **avalue, void *closure)
{
  bdffc_call_int (cif, fn, rvalue, avalue, closure);
}

/** private members **/

void BDFFC_HIDDEN bdffc_closure_i386(void);
void BDFFC_HIDDEN bdffc_closure_STDCALL(void);
void BDFFC_HIDDEN bdffc_closure_REGISTER(void);

struct closure_frame
{
  unsigned rettemp[4];				/* 0 */
  unsigned regs[3];				/* 16-24 */
  bdffc_cif *cif;					/* 28 */
  void (*fun)(bdffc_cif*,void*,void**,void*);	/* 32 */
  void *user_data;				/* 36 */
};

int BDFFC_HIDDEN BDFFC_DECLARE_FASTCALL
bdffc_closure_inner (struct closure_frame *frame, char *stack)
{
  bdffc_cif *cif = frame->cif;
  int cabi, i, n, flags, dir, narg_reg;
  const struct abi_params *pabi;
  bdffc_type **arg_types;
  char *argp;
  void *rvalue;
  void **avalue;

  cabi = cif->abi;
  flags = cif->flags;
  narg_reg = 0;
  rvalue = frame->rettemp;
  pabi = &abi_params[cabi];
  dir = pabi->dir;
  argp = (dir < 0 ? stack + cif->bytes : stack);

  switch (flags)
    {
    case X86_RET_STRUCTARG:
      if (pabi->nregs > 0)
	{
	  rvalue = (void *)frame->regs[pabi->regs[0]];
	  narg_reg = 1;
	  frame->rettemp[0] = (unsigned)rvalue;
	  break;
	}
      /* fallthru */
    case X86_RET_STRUCTPOP:
      rvalue = *(void **)argp;
      argp += sizeof(void *);
      frame->rettemp[0] = (unsigned)rvalue;
      break;
    }

  n = cif->nargs;
  avalue = alloca(sizeof(void *) * n);

  arg_types = cif->arg_types;
  for (i = 0; i < n; ++i)
    {
      bdffc_type *ty = arg_types[i];
      size_t z = ty->size;
      int t = ty->type;
      void *valp;

      if (z <= BDFFC_SIZEOF_ARG && t != BDFFC_TYPE_STRUCT)
	{
	  if (t != BDFFC_TYPE_FLOAT && narg_reg < pabi->nregs)
	    valp = &frame->regs[pabi->regs[narg_reg++]];
	  else if (dir < 0)
	    {
	      argp -= 4;
	      valp = argp;
	    }
	  else
	    {
	      valp = argp;
	      argp += 4;
	    }
	}
      else
	{
	  size_t za = BDFFC_ALIGN (z, BDFFC_SIZEOF_ARG);
	  size_t align = BDFFC_SIZEOF_ARG;

	  /* See the comment in bdffc_call_int.  */
	  if (t == BDFFC_TYPE_STRUCT && ty->alignment >= 16)
	    align = 16;

	  if (dir < 0)
	    {
	      /* ??? These reverse argument ABIs are probably too old
		 to have cared about alignment.  Someone should check.  */
	      argp -= za;
	      valp = argp;
	    }
	  else
	    {
	      argp = (char *)BDFFC_ALIGN (argp, align);
	      valp = argp;
	      argp += za;
	    }
	}

      avalue[i] = valp;
    }

  frame->fun (cif, rvalue, avalue, frame->user_data);

  if (cabi == BDFFC_STDCALL)
    return flags + (cif->bytes << X86_RET_POP_SHIFT);
  else
    return flags;
}

bdffc_status
bdffc_prep_closure_loc (bdffc_closure* closure,
                      bdffc_cif* cif,
                      void (*fun)(bdffc_cif*,void*,void**,void*),
                      void *user_data,
                      void *codeloc)
{
  char *tramp = closure->tramp;
  void (*dest)(void);
  int op = 0xb8;  /* movl imm, %eax */

  switch (cif->abi)
    {
    case BDFFC_SYSV:
    case BDFFC_THISCALL:
    case BDFFC_FASTCALL:
    case BDFFC_MS_CDECL:
      dest = bdffc_closure_i386;
      break;
    case BDFFC_STDCALL:
    case BDFFC_PASCAL:
      dest = bdffc_closure_STDCALL;
      break;
    case BDFFC_REGISTER:
      dest = bdffc_closure_REGISTER;
      op = 0x68;  /* pushl imm */
    default:
      return BDFFC_BAD_ABI;
    }

  /* movl or pushl immediate.  */
  tramp[0] = op;
  *(void **)(tramp + 1) = codeloc;

  /* jmp dest */
  tramp[5] = 0xe9;
  *(unsigned *)(tramp + 6) = (unsigned)dest - ((unsigned)codeloc + 10);

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  return BDFFC_OK;
}

void BDFFC_HIDDEN bdffc_go_closure_EAX(void);
void BDFFC_HIDDEN bdffc_go_closure_ECX(void);
void BDFFC_HIDDEN bdffc_go_closure_STDCALL(void);

bdffc_status
bdffc_prep_go_closure (bdffc_go_closure* closure, bdffc_cif* cif,
		     void (*fun)(bdffc_cif*,void*,void**,void*))
{
  void (*dest)(void);

  switch (cif->abi)
    {
    case BDFFC_SYSV:
    case BDFFC_MS_CDECL:
      dest = bdffc_go_closure_ECX;
      break;
    case BDFFC_THISCALL:
    case BDFFC_FASTCALL:
      dest = bdffc_go_closure_EAX;
      break;
    case BDFFC_STDCALL:
    case BDFFC_PASCAL:
      dest = bdffc_go_closure_STDCALL;
      break;
    case BDFFC_REGISTER:
    default:
      return BDFFC_BAD_ABI;
    }

  closure->tramp = dest;
  closure->cif = cif;
  closure->fun = fun;

  return BDFFC_OK;
}

/* ------- Native raw API support -------------------------------- */

#if !BDFFC_NO_RAW_API

void BDFFC_HIDDEN bdffc_closure_raw_SYSV(void);
void BDFFC_HIDDEN bdffc_closure_raw_THISCALL(void);

bdffc_status
bdffc_prep_raw_closure_loc (bdffc_raw_closure *closure,
                          bdffc_cif *cif,
                          void (*fun)(bdffc_cif*,void*,bdffc_raw*,void*),
                          void *user_data,
                          void *codeloc)
{
  char *tramp = closure->tramp;
  void (*dest)(void);
  int i;

  /* We currently don't support certain kinds of arguments for raw
     closures.  This should be implemented by a separate assembly
     language routine, since it would require argument processing,
     something we don't do now for performance.  */
  for (i = cif->nargs-1; i >= 0; i--)
    switch (cif->arg_types[i]->type)
      {
      case BDFFC_TYPE_STRUCT:
      case BDFFC_TYPE_LONGDOUBLE:
	return BDFFC_BAD_TYPEDEF;
      }

  switch (cif->abi)
    {
    case BDFFC_THISCALL:
      dest = bdffc_closure_raw_THISCALL;
      break;
    case BDFFC_SYSV:
      dest = bdffc_closure_raw_SYSV;
      break;
    default:
      return BDFFC_BAD_ABI;
    }

  /* movl imm, %eax.  */
  tramp[0] = 0xb8;
  *(void **)(tramp + 1) = codeloc;

  /* jmp dest */
  tramp[5] = 0xe9;
  *(unsigned *)(tramp + 6) = (unsigned)dest - ((unsigned)codeloc + 10);

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  return BDFFC_OK;
}

void
bdffc_raw_call(bdffc_cif *cif, void (*fn)(void), void *rvalue, bdffc_raw *avalue)
{
  size_t rsize, bytes;
  struct call_frame *frame;
  char *stack, *argp;
  bdffc_type **arg_types;
  int flags, cabi, i, n, narg_reg;
  const struct abi_params *pabi;

  flags = cif->flags;
  cabi = cif->abi;
  pabi = &abi_params[cabi];

  rsize = 0;
  if (rvalue == NULL)
    {
      switch (flags)
	{
	case X86_RET_FLOAT:
	case X86_RET_DOUBLE:
	case X86_RET_LDOUBLE:
	case X86_RET_STRUCTPOP:
	case X86_RET_STRUCTARG:
	  /* The float cases need to pop the 387 stack.
	     The struct cases need to pass a valid pointer to the callee.  */
	  rsize = cif->rtype->size;
	  break;
	default:
	  /* We can pretend that the callee returns nothing.  */
	  flags = X86_RET_VOID;
	  break;
	}
    }

  bytes = cif->bytes;
  argp = stack =
      (void *)((uintptr_t)alloca(bytes + sizeof(*frame) + rsize + 15) & ~16);
  frame = (struct call_frame *)(stack + bytes);
  if (rsize)
    rvalue = frame + 1;

  frame->fn = fn;
  frame->flags = flags;
  frame->rvalue = rvalue;

  narg_reg = 0;
  switch (flags)
    {
    case X86_RET_STRUCTARG:
      /* The pointer is passed as the first argument.  */
      if (pabi->nregs > 0)
	{
	  frame->regs[pabi->regs[0]] = (unsigned)rvalue;
	  narg_reg = 1;
	  break;
	}
      /* fallthru */
    case X86_RET_STRUCTPOP:
      *(void **)argp = rvalue;
      argp += sizeof(void *);
      bytes -= sizeof(void *);
      break;
    }

  arg_types = cif->arg_types;
  for (i = 0, n = cif->nargs; narg_reg < pabi->nregs && i < n; i++)
    {
      bdffc_type *ty = arg_types[i];
      size_t z = ty->size;
      int t = ty->type;

      if (z <= BDFFC_SIZEOF_ARG && t != BDFFC_TYPE_STRUCT && t != BDFFC_TYPE_FLOAT)
	{
	  bdffc_arg val = extend_basic_type (avalue, t);
	  frame->regs[pabi->regs[narg_reg++]] = val;
	  z = BDFFC_SIZEOF_ARG;
	}
      else
	{
	  memcpy (argp, avalue, z);
	  z = BDFFC_ALIGN (z, BDFFC_SIZEOF_ARG);
	  argp += z;
	}
      avalue += z;
      bytes -= z;
    }
  if (i < n)
    memcpy (argp, avalue, bytes);

  bdffc_call_i386 (frame, stack);
}
#endif /* !BDFFC_NO_RAW_API */
#endif /* !__x86_64__ */


#endif
