//
//  BDBaseTableViewController.h
//  ffi-iOSDemo
//
//  Created by zuopengliu on 30/5/2018.
//

#import <UIKit/UIKit.h>



@interface BDBaseTableViewController : UITableViewController

//
// Format as folows:
//  [
//      {@"text": @"***", @"sel": @"***"},
//      {@"text": @"***", @"sel": @"***"},
//      ...
//  ]
//  or
//  [
//      [
//          {@"text": @"***", @"sel": @"***"},
//          {@"text": @"***", @"sel": @"***"},
//          ...
//      ],
//      [
//          {@"text": @"***", @"sel": @"***"},
//          {@"text": @"***", @"sel": @"***"},
//          ...
//      ]
//  ]
//
@property (nonatomic, strong) NSArray *dataSource;

#pragma mark - overrides

- (NSString *)titleString;

- (void)setupDataSource;

#pragma mark - section

- (NSString *)titleAtSection:(NSUInteger)section;
- (CGFloat)sectionHeight; // Default is 80
@end
