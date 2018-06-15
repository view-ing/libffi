//
//  BDRootViewController.m
//  bdffc-iOSDemo
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
                                @"text": @"Test bdffc",
                                @"sel":  NSStringFromSelector(@selector(testBDFFC)),
                                },
                            ]
                        ];
}

- (NSString *)titleAtSection:(NSUInteger)section
{
    if (section == 0) {
        return @"Testing libbdffc ...";
    }
    return nil;
}

- (void)testBDFFC
{
    
}

@end
