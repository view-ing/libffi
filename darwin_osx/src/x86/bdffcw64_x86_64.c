#ifdef __x86_64__

/* -----------------------------------------------------------------------
   bdffcw64.c - Copyright (c) 2018 Anthony Green
              Copyright (c) 2014 Red Hat, Inc.

   x86 win64 Foreign Function Interface

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

#include <bdffc.h>
#include <bdffc_common.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef X86_WIN64
#define EFI64(name) name
#else
#define EFI64(name) name##_efi64
#endif

struct win64_call_frame
{
  UINT64 rbp;		/* 0 */
  UINT64 retaddr;	/* 8 */
  UINT64 fn;		/* 16 */
  UINT64 flags;		/* 24 */
  UINT64 rvalue;	/* 32 */
};

extern void bdffc_call_win64 (void *stack, struct win64_call_frame *,
			    void *closure) BDFFC_HIDDEN;

bdffc_status
EFI64(bdffc_prep_cif_machdep)(bdffc_cif *cif)
{
  int flags, n;

  switch (cif->abi)
    {
    case BDFFC_WIN64:
    case BDFFC_GNUW64:
      break;
    default:
      return BDFFC_BAD_ABI;
    }

  flags = cif->rtype->type;
  switch (flags)
    {
    default:
      break;
    case BDFFC_TYPE_LONGDOUBLE:
      /* GCC returns long double values by reference, like a struct */
      if (cif->abi == BDFFC_GNUW64)
	flags = BDFFC_TYPE_STRUCT;
      break;
    case BDFFC_TYPE_COMPLEX:
      flags = BDFFC_TYPE_STRUCT;
      /* FALLTHRU */
    case BDFFC_TYPE_STRUCT:
      switch (cif->rtype->size)
	{
	case 8:
	  flags = BDFFC_TYPE_UINT64;
	  break;
	case 4:
	  flags = BDFFC_TYPE_SMALL_STRUCT_4B;
	  break;
	case 2:
	  flags = BDFFC_TYPE_SMALL_STRUCT_2B;
	  break;
	case 1:
	  flags = BDFFC_TYPE_SMALL_STRUCT_1B;
	  break;
	}
      break;
    }
  cif->flags = flags;

  /* Each argument either fits in a register, an 8 byte slot, or is
     passed by reference with the pointer in the 8 byte slot.  */
  n = cif->nargs;
  n += (flags == BDFFC_TYPE_STRUCT);
  if (n < 4)
    n = 4;
  cif->bytes = n * 8;

  return BDFFC_OK;
}

static void
bdffc_call_int (bdffc_cif *cif, void (*fn)(void), void *rvalue,
	      void **avalue, void *closure)
{
  int i, j, n, flags;
  UINT64 *stack;
  size_t rsize;
  struct win64_call_frame *frame;

  BDFFC_ASSERT(cif->abi == BDFFC_GNUW64 || cif->abi == BDFFC_WIN64);

  flags = cif->flags;
  rsize = 0;

  /* If we have no return value for a structure, we need to create one.
     Otherwise we can ignore the return type entirely.  */
  if (rvalue == NULL)
    {
      if (flags == BDFFC_TYPE_STRUCT)
	rsize = cif->rtype->size;
      else
	flags = BDFFC_TYPE_VOID;
    }

  stack = alloca(cif->bytes + sizeof(struct win64_call_frame) + rsize);
  frame = (struct win64_call_frame *)((char *)stack + cif->bytes);
  if (rsize)
    rvalue = frame + 1;

  frame->fn = (uintptr_t)fn;
  frame->flags = flags;
  frame->rvalue = (uintptr_t)rvalue;

  j = 0;
  if (flags == BDFFC_TYPE_STRUCT)
    {
      stack[0] = (uintptr_t)rvalue;
      j = 1;
    }

  for (i = 0, n = cif->nargs; i < n; ++i, ++j)
    {
      switch (cif->arg_types[i]->size)
	{
	case 8:
	  stack[j] = *(UINT64 *)avalue[i];
	  break;
	case 4:
	  stack[j] = *(UINT32 *)avalue[i];
	  break;
	case 2:
	  stack[j] = *(UINT16 *)avalue[i];
	  break;
	case 1:
	  stack[j] = *(UINT8 *)avalue[i];
	  break;
	default:
	  stack[j] = (uintptr_t)avalue[i];
	  break;
	}
    }

  bdffc_call_win64 (stack, frame, closure);
}

void
EFI64(bdffc_call)(bdffc_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  bdffc_call_int (cif, fn, rvalue, avalue, NULL);
}

void
EFI64(bdffc_call_go)(bdffc_cif *cif, void (*fn)(void), void *rvalue,
	     void **avalue, void *closure)
{
  bdffc_call_int (cif, fn, rvalue, avalue, closure);
}


extern void bdffc_closure_win64(void) BDFFC_HIDDEN;
extern void bdffc_go_closure_win64(void) BDFFC_HIDDEN;

bdffc_status
EFI64(bdffc_prep_closure_loc)(bdffc_closure* closure,
		      bdffc_cif* cif,
		      void (*fun)(bdffc_cif*, void*, void**, void*),
		      void *user_data,
		      void *codeloc)
{
  static const unsigned char trampoline[16] = {
    /* leaq  -0x7(%rip),%r10   # 0x0  */
    0x4c, 0x8d, 0x15, 0xf9, 0xff, 0xff, 0xff,
    /* jmpq  *0x3(%rip)        # 0x10 */
    0xff, 0x25, 0x03, 0x00, 0x00, 0x00,
    /* nopl  (%rax) */
    0x0f, 0x1f, 0x00
  };
  char *tramp = closure->tramp;

  switch (cif->abi)
    {
    case BDFFC_WIN64:
    case BDFFC_GNUW64:
      break;
    default:
      return BDFFC_BAD_ABI;
    }

  memcpy (tramp, trampoline, sizeof(trampoline));
  *(UINT64 *)(tramp + 16) = (uintptr_t)bdffc_closure_win64;

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  return BDFFC_OK;
}

bdffc_status
EFI64(bdffc_prep_go_closure)(bdffc_go_closure* closure, bdffc_cif* cif,
		     void (*fun)(bdffc_cif*, void*, void**, void*))
{
  switch (cif->abi)
    {
    case BDFFC_WIN64:
    case BDFFC_GNUW64:
      break;
    default:
      return BDFFC_BAD_ABI;
    }

  closure->tramp = bdffc_go_closure_win64;
  closure->cif = cif;
  closure->fun = fun;

  return BDFFC_OK;
}

struct win64_closure_frame
{
  UINT64 rvalue[2];
  UINT64 fargs[4];
  UINT64 retaddr;
  UINT64 args[];
};

/* Force the inner function to use the MS ABI.  When compiling on win64
   this is a nop.  When compiling on unix, this simplifies the assembly,
   and places the burden of saving the extra call-saved registers on
   the compiler.  */
int BDFFC_HIDDEN __attribute__((ms_abi))
bdffc_closure_win64_inner(bdffc_cif *cif,
			void (*fun)(bdffc_cif*, void*, void**, void*),
			void *user_data,
			struct win64_closure_frame *frame)
{
  void **avalue;
  void *rvalue;
  int i, n, nreg, flags;

  avalue = alloca(cif->nargs * sizeof(void *));
  rvalue = frame->rvalue;
  nreg = 0;

  /* When returning a structure, the address is in the first argument.
     We must also be prepared to return the same address in eax, so
     install that address in the frame and pretend we return a pointer.  */
  flags = cif->flags;
  if (flags == BDFFC_TYPE_STRUCT)
    {
      rvalue = (void *)(uintptr_t)frame->args[0];
      frame->rvalue[0] = frame->args[0];
      nreg = 1;
    }

  for (i = 0, n = cif->nargs; i < n; ++i, ++nreg)
    {
      size_t size = cif->arg_types[i]->size;
      size_t type = cif->arg_types[i]->type;
      void *a;

      if (type == BDFFC_TYPE_DOUBLE || type == BDFFC_TYPE_FLOAT)
	{
	  if (nreg < 4)
	    a = &frame->fargs[nreg];
	  else
	    a = &frame->args[nreg];
	}
      else if (size == 1 || size == 2 || size == 4 || size == 8)
	a = &frame->args[nreg];
      else
	a = (void *)(uintptr_t)frame->args[nreg];

      avalue[i] = a;
    }

  /* Invoke the closure.  */
  fun (cif, rvalue, avalue, user_data);
  return flags;
}


#endif
