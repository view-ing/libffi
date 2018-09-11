//
//  TestStructModel.m
//  lli-dev
//
//  Created by zuopengliu on 10/5/2018.
//

#import "TestStructModel.h"



@implementation TestStructModel

+ (void)testCGRectMake
{
    CGRect rect = CGRectMake(0, 0, 200, 300);
    NSLog(@"%@", NSStringFromCGRect(rect));
}

+ (void)clsMethodWithRect:(CGRect)rect
{
    NSLog(@"%@", NSStringFromCGRect(rect));
}

+ (void)clsMethodWithPoint:(CGPoint)pt
{
    NSLog(@"%@", NSStringFromCGPoint(pt));
}

+ (void)clsMethodWithSize:(CGSize)size
{
    NSLog(@"%@", NSStringFromCGSize(size));
}

+ (void)clsMethodWithRange:(NSRange)range
{
    NSLog(@"%@", NSStringFromRange(range));
}

+ (CGRect)clsMethodWithReturnRect
{
    return CGRectZero;
}

+ (CGPoint)clsMethodWithReturnPoint
{
    return CGPointZero;
}

+ (CGSize)clsMethodWithReturnSize
{
    return CGSizeZero;
}

+ (NSRange)clsMethodWithReturnRange
{
    return NSMakeRange(0, 0);
}

- (void)instMethodWithRect:(CGRect)rect
{
    NSLog(@"%@", NSStringFromCGRect(rect));
}

- (void)instMethodWithPoint:(CGPoint)pt
{
    NSLog(@"%@", NSStringFromCGPoint(pt));
}

- (void)instMethodWithSize:(CGSize)size
{
    NSLog(@"%@", NSStringFromCGSize(size));
}

- (void)instMethodWithRange:(NSRange)range
{
    NSLog(@"%@", NSStringFromRange(range));
}

- (CGRect)instMethodWithReturnRect
{
    return CGRectMake(1, 1, 1, 1);
}

- (CGPoint)instMethodWithReturnPoint
{
    return CGPointMake(1, 1);
}

- (CGSize)instMethodWithReturnSize
{
    return CGSizeMake(1, 1);
}

- (NSRange)instMethodWithReturnRange
{
    return NSMakeRange(1, 1);
}

@end

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

void testStruct(void)
{
    [TestStructModel testCGRectMake];
    
    CGRect gcrect = getCGRect();
    NSCParameterAssert(CGRectEqualToRect(gcrect, CGRectMake(1, 1, 2, 3)));
    
    CGRect *gcrectPtr = getCGRectPtr();
    NSCParameterAssert(gcrectPtr);
    if (gcrectPtr) {
        NSCParameterAssert(CGRectEqualToRect(*gcrectPtr, CGRectMake(2, 3, 39, 59)));
    }
    
    [TestStructModel clsMethodWithRect:CGRectMake(100, 100, 100, 100)];
    [TestStructModel clsMethodWithPoint:CGPointMake(100, 100)];
    [TestStructModel clsMethodWithSize:CGSizeMake(100, 100)];
    [TestStructModel clsMethodWithRange:NSMakeRange(100, 100)];
    
    CGRect rect = [TestStructModel clsMethodWithReturnRect];
    NSCParameterAssert(CGRectEqualToRect(rect, CGRectZero));
    
    CGPoint point = [TestStructModel clsMethodWithReturnPoint];
    NSCParameterAssert(CGPointEqualToPoint(point, CGPointZero));
    
    CGSize size = [TestStructModel clsMethodWithReturnSize];
    NSCParameterAssert(CGSizeEqualToSize(size, CGSizeZero));
    
    NSRange range = [TestStructModel clsMethodWithReturnRange];
    NSCParameterAssert(range.length == 0 && range.location == 0);
    
    
    TestStructModel *aStructMdl = [TestStructModel new];
    [aStructMdl instMethodWithRect:CGRectMake(100, 100, 100, 100)];
    [aStructMdl instMethodWithPoint:CGPointMake(100, 100)];
    [aStructMdl instMethodWithSize:CGSizeMake(100, 100)];
    [aStructMdl instMethodWithRange:NSMakeRange(100, 100)];
    
    CGRect rect1 = [aStructMdl instMethodWithReturnRect];
    NSCParameterAssert(CGRectEqualToRect(rect1, CGRectMake(1, 1, 1, 1)));
    
    CGPoint point1 = [aStructMdl instMethodWithReturnPoint];
    NSCParameterAssert(CGPointEqualToPoint(point1, CGPointMake(1, 1)));
    
    CGSize size1 = [aStructMdl instMethodWithReturnSize];
    NSCParameterAssert(CGSizeEqualToSize(size1, CGSizeMake(1, 1)));
    
    NSRange range1 = [aStructMdl instMethodWithReturnRange];
    NSCParameterAssert(range1.length == 1 && range1.location == 1);
}
