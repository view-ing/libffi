//
//  BDBaseTableViewController.m
//  bdffc-iOSDemo
//
//  Created by zuopengliu on 30/5/2018.
//

#import "BDBaseTableViewController.h"



@implementation BDBaseTableViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.title = [self titleString] ? : @"libbdffc-iOSDemo!";
    
    [self setupDataSource];
}

#pragma mark -

- (NSString *)titleString
{
    return nil;
}

- (void)setupDataSource
{
    //    self.dataSource = @[
    //                      @{
    //                          @"text": @"Aspect Class",
    //                          @"sel":  NSStringFromSelector(@selector(hookClass)),
    //                          },
    //                      @{
    //                          @"text": @"Aspect MetaClass",
    //                          @"sel":  NSStringFromSelector(@selector(hookMetaclass)),
    //                          },
    //                      @{
    //                          @"text": @"Aspect Object",
    //                          @"sel":  NSStringFromSelector(@selector(hookInstance)),
    //                          },
    //                      ];
}

#pragma mark - UITableViewDataSource | UITableViewDelegate

- (BOOL)isMultipleSections
{
    if ([self.dataSource count] < 1) return NO;
    NSArray *firstOne = [self.dataSource firstObject];
    return [firstOne isKindOfClass:[NSArray class]];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    if ([self isMultipleSections]) {
        return [self.dataSource count];
    }
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    if (![self isMultipleSections]) {
        return [self.dataSource count];
    }
    
    NSArray *one = (((NSUInteger)section < [self.dataSource count])
                    ? [self.dataSource objectAtIndex:section] : nil);
    return ([one isKindOfClass:[NSArray class]] ?  [one count] : 0);
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section
{
    return MAX(0, [self sectionHeight]);
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
    return 50;
}

- (CGFloat)sectionHeight
{
    return 70;
}

- (NSString *)sectionTitle
{
    return nil;
}

- (NSString *)titleAtSection:(NSUInteger)section
{
    return nil;
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section
{
    UIView *aView = [UIView new];
    aView.backgroundColor = [UIColor colorWithRed:0x47/255.f green:0x4e/255.f blue:0x60/255.f alpha:1];
    aView.frame = CGRectMake(0, 0, self.view.frame.size.width, [self sectionHeight]);
    
    NSString *title = [self titleAtSection:section] ? : [self sectionTitle];
    UILabel *aLabel = [UILabel new];
    aLabel.font = [UIFont boldSystemFontOfSize:28];
    aLabel.numberOfLines = 0;
    aLabel.textColor = [UIColor whiteColor];
    aLabel.text = title ? : @"None";
    [aLabel sizeToFit];
    aLabel.frame = CGRectInset(aView.frame, 20, 0);
    [aView addSubview:aLabel];
    
    return aView;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString *cellIdentifier = @"reuse";
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:cellIdentifier];
    if (!cell) cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:cellIdentifier];
    
    
    NSArray *rowDataSource;
    if (![self isMultipleSections]) {
        rowDataSource = _dataSource;
    } else {
        rowDataSource = _dataSource[indexPath.section];
    }
    
    NSDictionary *cellModel = (indexPath.row < (NSInteger)rowDataSource.count) ? rowDataSource[indexPath.row] : nil;
    NSString *text = cellModel[@"text"];
    NSString *selname = cellModel[@"sel"];
    if (text.length == 0) text = [NSString stringWithFormat:@"Row: %ld", (long)indexPath.row];
    if (indexPath.row == (NSInteger)rowDataSource.count) { text = @"Peek"; selname = @"peek"; }
    
    cell.textLabel.font = [UIFont boldSystemFontOfSize:20];
    cell.textLabel.text = text;
    cell.detailTextLabel.text = selname ? [NSString stringWithFormat:@"SEL: %@", selname] : nil;
    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    
    NSArray *rowDataSource;
    if (![self isMultipleSections]) {
        rowDataSource = _dataSource;
    } else {
        rowDataSource = _dataSource[indexPath.section];
    }
    
    NSDictionary *cellModel = (indexPath.row < (NSInteger)rowDataSource.count) ? rowDataSource[indexPath.row] : nil;
    NSString *selname = cellModel[@"sel"];
    SEL sel = NSSelectorFromString(selname);
    if ([self respondsToSelector:sel]) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
        [self performSelector:sel];
#pragma clang diagnostic pop
    }
}

@end
