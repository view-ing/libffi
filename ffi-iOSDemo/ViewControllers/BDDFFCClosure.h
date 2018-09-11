//
//  BDDFFCClosure.h
//  ffi-iOSDemo
//
//  Created by zuopengliu on 21/6/2018.
//

#ifndef BDDFFCClosure_h
#define BDDFFCClosure_h

#include <stdio.h>
#include <ffi.h>



class BDFFCClosureCallContext {
private:
    ffi_cif *cif;
    ffi_type *ffiRetTy;
    ffi_type **ffiArgTys;
    ffi_closure *closure;
    
public:
    BDFFCClosureCallContext();
    ~BDFFCClosureCallContext();
    
    bool bind(ffi_type *retTy,
              ffi_type **argTys,
              void (*fun)(ffi_cif*, void*, void**, void*),
              void *userdata);
    
    void *closure_loc() const {
        return (void *)closure;
    }
    
private:
    void init();
    void destory();
};



#endif /* BDDFFCClosure_h */
