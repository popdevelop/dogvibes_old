//
//  ImageCell.h
//  testjson
//
//  Created by Mike Zupan on 8/5/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface ImageCell : UITableViewCell {
	UILabel *titleLabel;
	UILabel *urlLabel;
	NSInteger *itemID; // added this
}

// these are the functions we will create in the .m file

-(void)setData:(NSDictionary *)dict;
-(NSInteger)getID; // added this

-(UILabel *)newLabelWithPrimaryColor:(UIColor *)primaryColor selectedColor:(UIColor *)selectedColor fontSize:(CGFloat)fontSize bold:(BOOL)bold;

@property (nonatomic, retain) UILabel *titleLabel;
@property (nonatomic, retain) UILabel *urlLabel;
@property (nonatomic, assign) NSInteger *itemID; // added this
 
@end
