//
//  BDRootViewController.m
//  bdffc-iOSDemo
//
//  Created by zuopengliu on 30/5/2018.
//

#import "BDRootViewController.h"
#import <ffi.h>
#import <dlfcn.h>
#import "BDDFFCClosure.h"
#import "TestStructModel.h"
#import "macho_dlfcn.h"



void passCGRect(CGRect rect)
{
    NSLog(@"%@", NSStringFromCGRect(rect));
}

void passCGRectPtr(CGRect *rect)
{
    if (!rect) return;
    NSLog(@"%@", NSStringFromCGRect(*rect));
}

CGRect getCGRect()
{
    return CGRectMake(1, 1, 2, 3);
}

CGRect* getCGRectPtr()
{
    CGRect *rect = (CGRect *)malloc(sizeof(CGRect));
    rect->origin = CGPointMake(2, 3);
    rect->size   = CGSizeMake(39, 59);
    return rect;
}



@implementation BDRootViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    NSRange r = NSMakeRange(10, 10);
    NSLog(@"r = %@", NSStringFromRange(r));
    [self.class printRange:r];
}

+ (void)printRange:(NSRange)range
{
    NSLog(@"range = %@", NSStringFromRange(range));
}

- (void)setupDataSource
{
    self.dataSource = @[
                        // Section0
                        @[
                            @{
                                @"text": @"Test ffi",
                                @"sel":  NSStringFromSelector(@selector(testFFI)),
                                },
                            @{
                                @"text": @"Test ffi closure",
                                @"sel":  NSStringFromSelector(@selector(testFFIClosure)),
                                },
                            @{
                                @"text": @"Test ffi closure CallFrame (Memory at heap)",
                                @"sel":  NSStringFromSelector(@selector(testFFIClosureCallFrame)),
                                },
                            @{
                                @"text": @"Test ffi closure CallFrame (Memory at stack)",
                                @"sel":  NSStringFromSelector(@selector(testFFIClosureFrameAtStack)),
                                },
                            
                            ],
                        // Section1
                        @[
                            @{
                                @"text": @"Test CGRect Params",
                                @"sel":  NSStringFromSelector(@selector(testCGRectParam)),
                                },
                            @{
                                @"text": @"Test CGRect Pointer Params",
                                @"sel":  NSStringFromSelector(@selector(testCGRectPtrParam)),
                                },
                            
                            @{
                                @"text": @"Test CGRect Return Value",
                                @"sel":  NSStringFromSelector(@selector(testReturnCGRectVal)),
                                },
                            @{
                                @"text": @"Test CGRect Return Pointer Value",
                                @"sel":  NSStringFromSelector(@selector(testReturnCGRectPtrVal)),
                                },
                            @{
                                @"text": @"Test CGRectMake function",
                                @"sel":  NSStringFromSelector(@selector(testCGRectMake)),
                                },
                            @{
                                @"text": @"Test CGRectMake (32bit) function",
                                @"sel":  NSStringFromSelector(@selector(testCGRectMake_32)),
                                },
                            
                            @{
                                @"text": @"Test CGRectMake (32bit) function --2--",
                                @"sel":  NSStringFromSelector(@selector(testCGRectMake_32_2)),
                                },
                            ]
                        ];
}

- (NSString *)titleAtSection:(NSUInteger)section
{
    if (section == 0) {
        return @"Testing libffi ...";
    } else if (section == 1) {
        return @"Testing libffi Struct ...";
    }
    return nil;
}

#pragma mark - section0

- (void)testFFI
{
    
}

/* Acts like puts with the file given at time of enclosure. */
void puts_binding(ffi_cif *cif, void *ret, void* args[],
                  void *stream)
{
    *(ffi_arg *)ret = fputs(*(char **)args[0], (FILE *)stream);
}

typedef int (*puts_t)(char *);

- (void)testFFIClosure
{
    ffi_cif cif;
    ffi_type *args[1];
    ffi_closure *closure;
    
    void *bound_puts;
    int rc;
    
    /* Allocate closure and bound_puts */
    closure = (ffi_closure *)ffi_closure_alloc(sizeof(ffi_closure), &bound_puts);
    
    if (closure)
    {
        /* Initialize the argument info vectors */
        args[0] = &ffi_type_pointer;
        
        /* Initialize the cif */
        if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1,
                         &ffi_type_sint, args) == FFI_OK)
        {
            /* Initialize the closure, setting stream to stdout */
            if (ffi_prep_closure_loc(closure, &cif, puts_binding,
                                     stdout, bound_puts) == FFI_OK)
            {
                rc = ((puts_t)bound_puts)("Hello World!\n\n");
                /* rc now holds the result of the call to fputs */
            }
        }
    }
    
    if (bound_puts) {
        rc = ((puts_t)bound_puts)("Hello World!\n");
    }
    
    /* Deallocate both closure, and bound_puts */
    ffi_closure_free(closure);
}

- (void)testFFIClosureCallFrame
{
    BDFFCClosureCallContext *closureFrame = new BDFFCClosureCallContext;
    
    ffi_type *retTy = &ffi_type_sint;
    ffi_type **argTys;
    argTys = (ffi_type **)malloc(1 * sizeof(ffi_type *));
    argTys[0] = &ffi_type_pointer;
    
    closureFrame->bind(retTy, argTys, puts_binding, stdout);
    
    void *fnPtr = closureFrame->closure_loc();
    if (fnPtr) {
        int rc = ((puts_t)fnPtr)("Hello World!\n\n");
        rc = ((puts_t)fnPtr)("Hello World!\n\n");
    }
    
    if (closureFrame) delete closureFrame;
}

- (void)testFFIClosureFrameAtStack
{
    BDFFCClosureCallContext closureFrame;
    
    ffi_type *retTy = &ffi_type_sint;
    ffi_type **argTys;
    argTys = (ffi_type **)malloc(1 * sizeof(ffi_type *));
    argTys[0] = &ffi_type_pointer;
    
    closureFrame.bind(retTy, argTys, puts_binding, stdout);
    
    void *fnPtr = closureFrame.closure_loc();
    if (fnPtr) {
        int rc = ((puts_t)fnPtr)("Hello World!\n\n");
        rc = ((puts_t)fnPtr)("Hello World!\n\n");
    }
}

#pragma mark - section1

- (void)testCGRectParam
{
    ffi_cif cif;
    ffi_type *retTy = &ffi_type_void;
    ffi_type *argTys[1];
    
    /* Initialize the argument info vectors */
    ffi_type *rectTy = (ffi_type *)malloc(sizeof(ffi_type));
    rectTy->type = FFI_TYPE_STRUCT;
    rectTy->alignment = 0;
    rectTy->size = 0;
    rectTy->elements = (ffi_type **)malloc(sizeof(ffi_type *) * (4 + 1));
    rectTy->elements[0] = &ffi_type_double;
    rectTy->elements[1] = &ffi_type_double;
    rectTy->elements[2] = &ffi_type_double;
    rectTy->elements[3] = &ffi_type_double;
    rectTy->elements[4] = NULL;
    
//    argTys[0] = rectTy;//&ffi_type_pointer;
    argTys[0] = &ffi_type_pointer;
    
    
    ffi_arg rc;
    void *values[1];
    
    CGRect rect = CGRectMake(10, 10, 10, 10);
    void *rectPtr = (CGRect *)&rect;
    values[0] = rectPtr; // &rect
    
    
    /* Initialize the cif */
    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, retTy, argTys) == FFI_OK) {
        void *fn = (void *)passCGRect;
        ffi_call(&cif, FFI_FN(fn), &rc, values);
    }
    
    
    if (rectTy) free(rectTy);
}

- (void)testCGRectPtrParam
{
    ffi_cif cif;
    ffi_type *retTy = &ffi_type_void;
    ffi_type *argTys[1];
    
    /* Initialize the argument info vectors */
    ffi_type *rectTy = (ffi_type *)malloc(sizeof(ffi_type));
    rectTy->type = FFI_TYPE_STRUCT;
    rectTy->alignment = 0;
    rectTy->size = 0;
    rectTy->elements = (ffi_type **)malloc(sizeof(ffi_type *) * (4 + 1));
    rectTy->elements[0] = &ffi_type_double;
    rectTy->elements[1] = &ffi_type_double;
    rectTy->elements[2] = &ffi_type_double;
    rectTy->elements[3] = &ffi_type_double;
    rectTy->elements[4] = NULL;
    
    argTys[0] = &ffi_type_pointer;
    
    
    ffi_arg rc;
    void *values[1];
    
    CGRect rect = CGRectMake(10, 10, 10, 10);
    void *rectPtr = (CGRect *)&rect;
    values[0] = &rectPtr;
    
    
    /* Initialize the cif */
    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, retTy, argTys) == FFI_OK) {
        void *fn = (void *)passCGRectPtr;
        ffi_call(&cif, FFI_FN(fn), &rc, values);
        
        fn = (void *)passCGRectPtr;
        values[0] = rectPtr;
        ffi_call(&cif, FFI_FN(fn), &rc, values);
    }
    
    
    if (rectTy) free(rectTy);
}

- (void)testReturnCGRectVal
{
    [self testReturnCGRectVal_zero];
    
    ffi_cif cif;
    ffi_type *retTy = NULL;
    ffi_type *argTys[1];
    
    /* Initialize the argument info vectors */
    ffi_type *rectTy = (ffi_type *)malloc(sizeof(ffi_type));
    rectTy->type = FFI_TYPE_STRUCT;
    rectTy->alignment = 0;
    rectTy->size = 0;
    rectTy->elements = (ffi_type **)malloc(sizeof(ffi_type *) * (4 + 1));
    rectTy->elements[0] = &ffi_type_double;
    rectTy->elements[1] = &ffi_type_double;
    rectTy->elements[2] = &ffi_type_double;
    rectTy->elements[3] = &ffi_type_double;
    rectTy->elements[4] = NULL;
    
    retTy = rectTy;
    argTys[0] = &ffi_type_void;
    
    
    ffi_arg argValue;
    void *values[1];
    values[0] = &argValue;
    
    CGRect retValue; // = (CGRect *)malloc(sizeof(16));
    
    /* Initialize the cif */
    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 0, retTy, argTys) == FFI_OK) {
        void *fn = (void *)getCGRect;
        ffi_call(&cif, FFI_FN(fn), /*&retValue*/&retValue, values);
    }
    
    if (rectTy) free(rectTy);
}

- (void)testReturnCGRectVal_zero
{
    ffi_cif cif;
    ffi_type *retTy = NULL;
    ffi_type *argTys[1];
    
    /* Initialize the argument info vectors */
    ffi_type *rectTy = (ffi_type *)malloc(sizeof(ffi_type));
    rectTy->type = FFI_TYPE_STRUCT;
    rectTy->alignment = 0;
    rectTy->size = 0;
    rectTy->elements = (ffi_type **)malloc(sizeof(ffi_type *) * (4 + 1));
    rectTy->elements[0] = &ffi_type_double;
    rectTy->elements[1] = &ffi_type_double;
    rectTy->elements[2] = &ffi_type_double;
    rectTy->elements[3] = &ffi_type_double;
    rectTy->elements[4] = NULL;
    
    retTy = &ffi_type_void;
    argTys[0] = &ffi_type_pointer; //rectTy;
    
    
    __unused ffi_arg argValue;
    CGRect argRect;
    CGRect *argRectPtr;
    void *values[1];
    values[0] = &argRectPtr;
    
    __unused CGRect retRectValue; // = (CGRect *)malloc(sizeof(16));
    __unused ffi_arg retValue;
    void *retValuePtr = &retRectValue;
    
    /* Initialize the cif */
    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, retTy, argTys) == FFI_OK) {
        void *fn = (void *)getCGRect;
        ffi_call(&cif, FFI_FN(fn), retValuePtr, values);
    }
    
    if (rectTy) free(rectTy);
}

- (void)testReturnCGRectPtrVal
{
    ffi_cif cif;
    ffi_type *retTy = NULL;
    ffi_type *argTys[1];
    
    /* Initialize the argument info vectors */
    ffi_type *rectTy = (ffi_type *)malloc(sizeof(ffi_type));
    rectTy->type = FFI_TYPE_STRUCT;
    rectTy->alignment = 0;
    rectTy->size = 0;
    rectTy->elements = (ffi_type **)malloc(sizeof(ffi_type *) * (4 + 1));
    rectTy->elements[0] = &ffi_type_double;
    rectTy->elements[1] = &ffi_type_double;
    rectTy->elements[2] = &ffi_type_double;
    rectTy->elements[3] = &ffi_type_double;
    rectTy->elements[4] = NULL;
    
    retTy = &ffi_type_pointer;
    argTys[0] = &ffi_type_void;
    
    
    ffi_arg argValue;
    void *values[1];
    values[0] = &argValue;
    
    CGRect *retValuePtr;
    
    /* Initialize the cif */
    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, retTy, argTys) == FFI_OK) {
        void *fn = (void *)getCGRectPtr;
        ffi_call(&cif, FFI_FN(fn), &retValuePtr, values);
    }
    
    if (rectTy) free(rectTy);
    if (retValuePtr) free(retValuePtr);
}

- (void)testCGRectMake
{
    const int nargs = 5;
    ffi_cif cif;
    ffi_type *retTy = NULL;
    ffi_type *argTys[nargs];
    
    /* Initialize the argument info vectors */
    ffi_type *rectTy = (ffi_type *)malloc(sizeof(ffi_type));
    rectTy->type = FFI_TYPE_STRUCT;
    rectTy->alignment = 0;
    rectTy->size = 0;
    rectTy->elements = (ffi_type **)malloc(sizeof(ffi_type *) * (4 + 1));
    rectTy->elements[0] = &ffi_type_double;
    rectTy->elements[1] = &ffi_type_double;
    rectTy->elements[2] = &ffi_type_double;
    rectTy->elements[3] = &ffi_type_double;
    rectTy->elements[4] = NULL;
    
    retTy = &ffi_type_void;
    // argTys[0] = rectTy;
    argTys[0] = &ffi_type_pointer; //rectTy;
    argTys[1] = &ffi_type_double;
    argTys[2] = &ffi_type_double;
    argTys[3] = &ffi_type_double;
    argTys[4] = &ffi_type_double;
    
    
    CGRect arg0;
    CGRect *arg0_prt = &arg0;
    double arg1 = 10;
    double arg2 = 20;
    double arg3 = 30;
    double arg4 = 40;

    void *values[nargs];
    values[0] = &arg0_prt;
    values[1] = &arg1;
    values[2] = &arg2;
    values[3] = &arg3;
    values[4] = &arg4;

    void *retValuePtr = NULL;
    
    /* Initialize the cif */
    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, nargs, retTy, argTys) == FFI_OK) {
        void *fn = (void *)CGRectMake;
        ffi_call(&cif, FFI_FN(fn), retValuePtr, values);
        
        void *hmdFn = (void *)NSMachO::macho_lookup_sym(RTLD_DEFAULT, "CGRectMake");
        ffi_call(&cif, FFI_FN(hmdFn), retValuePtr, values);
        printf("");
    }
    
    if (rectTy) free(rectTy);
    
    [self testCGRectMake_bak];
}

- (void)testCGRectMake_bak
{
    const int nargs = 5;
    ffi_cif cif;
    ffi_type *retTy = NULL;
    ffi_type *argTys[nargs];
    
    /* Initialize the argument info vectors */
    ffi_type *rectTy = (ffi_type *)malloc(sizeof(ffi_type));
    rectTy->type = FFI_TYPE_STRUCT;
    rectTy->alignment = 0;
    rectTy->size = 0;
    rectTy->elements = (ffi_type **)malloc(sizeof(ffi_type *) * (4 + 1));
    rectTy->elements[0] = &ffi_type_double;
    rectTy->elements[1] = &ffi_type_double;
    rectTy->elements[2] = &ffi_type_double;
    rectTy->elements[3] = &ffi_type_double;
    rectTy->elements[4] = NULL;
    
    
    retTy = &ffi_type_void;
    argTys[0] = &ffi_type_pointer;
    argTys[1] = &ffi_type_double;
    argTys[2] = &ffi_type_double;
    argTys[3] = &ffi_type_double;
    argTys[4] = &ffi_type_double;
    
    
    double arg1 = 10;
    double arg2 = 20;
    double arg3 = 30;
    double arg4 = 40;
    
    
    unsigned step = sizeof(CGFloat);
    unsigned base = sizeof(void *);//4 * sizeof(CGFloat);
    unsigned bytes = base + step * 4;
    void *ptr = malloc(bytes);
    bzero(ptr, bytes);
    char *cursor     = (char *)ptr;
    CGRect *arg0Ptr  = (CGRect *)ptr;
    CGFloat *arg1Ptr = (CGFloat *)(cursor + base + step * 0);
    CGFloat *arg2Ptr = (CGFloat *)(cursor + base + step * 1);
    CGFloat *arg3Ptr = (CGFloat *)(cursor + base + step * 2);
    CGFloat *arg4Ptr = (CGFloat *)(cursor + base + step * 3);
    
    *(CGFloat *)arg1Ptr = arg1;
    *(CGFloat *)arg2Ptr = arg2;
    *(CGFloat *)arg3Ptr = arg3;
    *(CGFloat *)arg4Ptr = arg4;
    
    void *values[nargs];
    values[0] = &arg0Ptr;
    values[1] = arg1Ptr;
    values[2] = arg2Ptr;
    values[3] = arg3Ptr;
    values[4] = arg4Ptr;
    
    void *retValuePtr = NULL;
    
    /* Initialize the cif */
    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, nargs, retTy, argTys) == FFI_OK) {
        void *fn = (void *)CGRectMake;
        ffi_call(&cif, FFI_FN(fn), retValuePtr, values);
        
        void *hmdFn = (void *)NSMachO::macho_lookup_sym(RTLD_DEFAULT, "CGRectMake");
        ffi_call(&cif, FFI_FN(hmdFn), retValuePtr, values);
        printf("");
    }
    
    if (rectTy) free(rectTy);
}

- (void)testCGRectMake_32
{
    const int nargs = 5;
    ffi_cif cif;
    ffi_type *retTy = NULL;
    ffi_type *argTys[nargs];
    
    /* Initialize the argument info vectors */
    ffi_type *rectTy = (ffi_type *)malloc(sizeof(ffi_type));
    rectTy->type = FFI_TYPE_STRUCT;
    rectTy->alignment = 0;
    rectTy->size = 0;
    rectTy->elements = (ffi_type **)malloc(sizeof(ffi_type *) * (4 + 1));
    rectTy->elements[0] = &ffi_type_float;
    rectTy->elements[1] = &ffi_type_float;
    rectTy->elements[2] = &ffi_type_float;
    rectTy->elements[3] = &ffi_type_float;
    rectTy->elements[4] = NULL;
    
    retTy = &ffi_type_void;
    // argTys[0] = rectTy;
    argTys[0] = &ffi_type_pointer;
    argTys[1] = &ffi_type_float;
    argTys[2] = &ffi_type_float;
    argTys[3] = &ffi_type_float;
    argTys[4] = &ffi_type_float;
    
    
    CGRect arg0;
    CGRect *arg0_prt = &arg0;
    float arg1 = 10;
    float arg2 = 20;
    float arg3 = 30;
    float arg4 = 40;
    
    void *values[nargs];
    values[0] = &arg0_prt;
    values[1] = &arg1;
    values[2] = &arg2;
    values[3] = &arg3;
    values[4] = &arg4;
    
    void *retValuePtr = NULL;
    
    /* Initialize the cif */
    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, nargs, retTy, argTys) == FFI_OK) {
        void *fn = (void *)CGRectMake;
        ffi_call(&cif, FFI_FN(fn), retValuePtr, values);
        
        void *hmdFn = (void *)NSMachO::macho_lookup_sym(RTLD_DEFAULT, "CGRectMake");
        //ffi_call(&cif, FFI_FN(hmdFn), retValuePtr, values);
        printf("");
    }
    
    if (rectTy) free(rectTy);
    
    [self testCGRectMake_32_bak];
}

- (void)testCGRectMake_32_bak
{
    const int nargs = 5;
    ffi_cif cif;
    ffi_type *retTy = NULL;
    ffi_type *argTys[nargs];

    
    /* Initialize the argument info vectors */
    ffi_type *rectTy = (ffi_type *)malloc(sizeof(ffi_type));
    rectTy->type = FFI_TYPE_STRUCT;
    rectTy->alignment = 0;
    rectTy->size = 0;
    rectTy->elements = (ffi_type **)malloc(sizeof(ffi_type *) * (4 + 1));
    rectTy->elements[0] = &ffi_type_float;
    rectTy->elements[1] = &ffi_type_float;
    rectTy->elements[2] = &ffi_type_float;
    rectTy->elements[3] = &ffi_type_float;
    rectTy->elements[4] = NULL;

    
    retTy = &ffi_type_void;
    // argTys[0] = rectTy;
    argTys[0] = &ffi_type_pointer;
    argTys[1] = &ffi_type_float;
    argTys[2] = &ffi_type_float;
    argTys[3] = &ffi_type_float;
    argTys[4] = &ffi_type_float;


    CGFloat arg1 = 10;
    CGFloat arg2 = 20;
    CGFloat arg3 = 30;
    CGFloat arg4 = 40;


    unsigned step = sizeof(CGFloat);
    unsigned base = 4 * sizeof(CGFloat);
    unsigned bytes = base + step * 4;
    void *ptr = malloc(bytes);
    bzero(ptr, bytes);
    char *cursor   = (char *)ptr;
    CGRect *arg0Ptr  = (CGRect *)ptr;
    CGFloat *arg1Ptr = (CGFloat *)(cursor + base + step * 0);
    CGFloat *arg2Ptr = (CGFloat *)(cursor + base + step * 1);
    CGFloat *arg3Ptr = (CGFloat *)(cursor + base + step * 2);
    CGFloat *arg4Ptr = (CGFloat *)(cursor + base + step * 3);

    
    *(CGFloat *)arg1Ptr = arg1;
    *(CGFloat *)arg2Ptr = arg2;
    *(CGFloat *)arg3Ptr = arg3;
    *(CGFloat *)arg4Ptr = arg4;

    
    void *values[nargs];
    
    values[0] = &arg0Ptr;
//    memcpy(&values[0], arg0Ptr, sizeof(void *));
    values[1] = &arg1; //arg1Ptr;
    values[2] = &arg2; //arg2Ptr;
    values[3] = &arg3; //arg3Ptr;
    values[4] = &arg4; //arg4Ptr;

    void *retValuePtr = NULL;

    /* Initialize the cif */
    CGRect rect = CGRectMake(1, 4, 3, 0);
    NSLog(@"%@", NSStringFromCGRect(rect));
    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, nargs, retTy, argTys) == FFI_OK) {
        void *fn = (void *)CGRectMake;
        ffi_call(&cif, FFI_FN(fn), retValuePtr, values);
        
        void *hmdFn = (void *)NSMachO::macho_lookup_sym(RTLD_DEFAULT, "CGRectMake");
        ffi_call(&cif, FFI_FN(hmdFn), retValuePtr, values);
        printf("");
    }

    if (rectTy) free(rectTy);
}

- (void)testCGRectMake_32_2
{
    const int nargs = 4;
    ffi_cif cif;
    ffi_type *retTy = NULL;
    ffi_type *argTys[nargs];
    
    /* Initialize the argument info vectors */
    ffi_type *rectTy = (ffi_type *)malloc(sizeof(ffi_type));
    rectTy->type = FFI_TYPE_STRUCT;
    rectTy->alignment = 0;
    rectTy->size = 0;
    rectTy->elements = (ffi_type **)malloc(sizeof(ffi_type *) * (4 + 1));
    rectTy->elements[0] = &ffi_type_float;
    rectTy->elements[1] = &ffi_type_float;
    rectTy->elements[2] = &ffi_type_float;
    rectTy->elements[3] = &ffi_type_float;
    rectTy->elements[4] = NULL;
    
    retTy = rectTy;
    argTys[0] = &ffi_type_float;
    argTys[1] = &ffi_type_float;
    argTys[2] = &ffi_type_float;
    argTys[3] = &ffi_type_float;
    
    
    CGRect arg0;
    CGRect *arg0_ptr = &arg0;
    CGFloat arg1 = 10;
    CGFloat arg2 = 20;
    CGFloat arg3 = 30;
    CGFloat arg4 = 40;
    
    void *values[nargs];
    values[0] = &arg1;
    values[1] = &arg2;
    values[2] = &arg3;
    values[3] = &arg4;
    
    void *retValuePtr = &arg0;
    
    /* Initialize the cif */
    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, nargs, retTy, argTys) == FFI_OK) {
        void *fn = (void *)CGRectMake;
        ffi_call(&cif, FFI_FN(fn), retValuePtr, values);
    } else {
        
    }
    
    if (rectTy) free(rectTy);
}

@end
