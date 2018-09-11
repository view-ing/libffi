//
//  BDDFFCClosureCall.cpp
//  ffi-iOSDemo
//
//  Created by zuopengliu on 21/6/2018.
//

#include "BDDFFCClosure.h"
#include <memory>



BDFFCClosureCallContext::BDFFCClosureCallContext()
{
    this->init();
}

BDFFCClosureCallContext::~BDFFCClosureCallContext()
{
    this->destory();
}

void BDFFCClosureCallContext::init()
{
    this->cif       = NULL;
    this->ffiRetTy  = NULL;
    this->ffiArgTys = NULL;
    this->closure   = NULL;
}

void BDFFCClosureCallContext::destory()
{
    if (this->closure)    ffi_closure_free(this->closure);
    if (this->ffiArgTys)  free(this->ffiArgTys);
    if (this->cif)        free(this->cif);
    
    this->init();
}

bool BDFFCClosureCallContext::bind(ffi_type *retTy, ffi_type **argTys, void (*fun)(ffi_cif *, void *, void **, void *), void *userdata)
{
    assert(retTy && argTys && "BDFFC return type or argument types are null");
    
    this->destory();
    
    if (!retTy) retTy = &ffi_type_void;
    if (!argTys) return false;
    
    this->cif = (ffi_cif *)malloc(sizeof(ffi_cif));
    this->ffiRetTy  = retTy;
    this->ffiArgTys = argTys;
    
    /* Allocate closure and bound_puts */
    void *closurePtr = NULL;
    this->closure = (ffi_closure *)ffi_closure_alloc(sizeof(ffi_closure), &closurePtr);
    if (!this->closure) return false;
    
    /* Initialize the cif */
    if (ffi_prep_cif(this->cif, FFI_DEFAULT_ABI, 1, this->ffiRetTy, this->ffiArgTys) == FFI_OK) {
        
        /* Initialize the closure */
        if (ffi_prep_closure_loc(this->closure, this->cif, fun, userdata, closurePtr) == FFI_OK) {
            /* rc now holds the result of the call to fputs */
            return true;
        }
    }
    
    return false;
}


