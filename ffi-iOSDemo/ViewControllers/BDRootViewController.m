//
//  BDRootViewController.m
//  ffi-iOSDemo
//
//  Created by zuopengliu on 30/5/2018.
//

#import "BDRootViewController.h"



@interface BDRootViewController ()

@end

@implementation BDRootViewController

- (void)setupDataSource
{
    self.dataSource = @[
                        @[
                            @{
                                @"text": @"Test ffi",
                                @"sel":  NSStringFromSelector(@selector(testFFI)),
                                },
                            ]
                        ];
}

- (NSString *)titleAtSection:(NSUInteger)section
{
    if (section == 0) {
        return @"Testing libffi ...";
    }
    return nil;
}

- (void)testFFI
{
    
}

@end
