//
//  TestStructModel.h
//  lli-dev
//
//  Created by zuopengliu on 10/5/2018.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>



@interface TestStructModel : NSObject

+ (void)clsMethodWithRect:(CGRect)rect;
+ (void)clsMethodWithPoint:(CGPoint)pt;
+ (void)clsMethodWithSize:(CGSize)size;
+ (void)clsMethodWithRange:(NSRange)range;

+ (CGRect)clsMethodWithReturnRect;
+ (CGPoint)clsMethodWithReturnPoint;
+ (CGSize)clsMethodWithReturnSize;
+ (NSRange)clsMethodWithReturnRange;

#pragma mark -

- (void)instMethodWithRect:(CGRect)rect;
- (void)instMethodWithPoint:(CGPoint)pt;
- (void)instMethodWithSize:(CGSize)size;
- (void)instMethodWithRange:(NSRange)range;

- (CGRect)instMethodWithReturnRect;
- (CGPoint)instMethodWithReturnPoint;
- (CGSize)instMethodWithReturnSize;
- (NSRange)instMethodWithReturnRange;

@end


FOUNDATION_EXTERN
void testStruct(void);
