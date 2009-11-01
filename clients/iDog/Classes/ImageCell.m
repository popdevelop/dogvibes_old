//
//  ImageCell.m
//  testjson
//
//  Created by Mike Zupan on 8/5/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "ImageCell.h"


@implementation ImageCell

// we need to synthesize the two labels
@synthesize titleLabel, urlLabel;
@synthesize itemID;

- (id)initWithFrame:(CGRect)frame reuseIdentifier:(NSString *)reuseIdentifier {
	if (self = [super initWithFrame:frame reuseIdentifier:reuseIdentifier]) {
		// Initialization code
		
		self.itemID = 0;
		
		// we need a view to place our labels on.
		UIView *myContentView = self.contentView;
		
		/*
		 init the title label. 
		 set the text alignment to align on the left
		 add the label to the subview
		 release the memory
		*/
		self.titleLabel = [self newLabelWithPrimaryColor:[UIColor blackColor] selectedColor:[UIColor whiteColor] fontSize:14.0 bold:YES]; 
		self.titleLabel.textAlignment = UITextAlignmentLeft; // default
		[myContentView addSubview:self.titleLabel];
		[self.titleLabel release];
		
		/*
		 init the url label. (you will see a difference in the font color and size here!
		 set the text alignment to align on the left
		 add the label to the subview
		 release the memory
		 */
        self.urlLabel = [self newLabelWithPrimaryColor:[UIColor blackColor] selectedColor:[UIColor lightGrayColor] fontSize:10.0 bold:NO];
		self.urlLabel.textAlignment = UITextAlignmentLeft; // default
		[myContentView addSubview:self.urlLabel];
		[self.urlLabel release];		
	}
	
	return self;
}


- (void)setSelected:(BOOL)selected animated:(BOOL)animated {

	[super setSelected:selected animated:animated];

	// Configure the view for the selected state
}

/*
 this function gets in data from another area in the code
 you can see it takes a NSDictionary object
 it then will set the label text
*/
-(void)setData:(NSDictionary *)dict {
	self.titleLabel.text = [dict objectForKey:@"title"];
	self.urlLabel.text = [dict objectForKey:@"img"];
	self.itemID = (NSInteger)[dict objectForKey:@"id"];
}

-(NSInteger)getID {
	return self.itemID;
}

/*
 this function will layout the subviews for the cell
 if the cell is not in editing mode we want to position them
*/
- (void)layoutSubviews {

    [super layoutSubviews];
	
	// getting the cell size
    CGRect contentRect = self.contentView.bounds;
	
	// In this example we will never be editing, but this illustrates the appropriate pattern
    if (!self.editing) {
		
		// get the X pixel spot
        CGFloat boundsX = contentRect.origin.x;
		CGRect frame;
        
        /*
		 Place the title label.
		 place the label whatever the current X is plus 10 pixels from the left
		 place the label 4 pixels from the top
		 make the label 200 pixels wide
		 make the label 20 pixels high
		*/
		frame = CGRectMake(boundsX + 10, 4, 200, 20);
		self.titleLabel.frame = frame;
        
		// place the url label
		frame = CGRectMake(boundsX + 10, 28, 200, 14);
		self.urlLabel.frame = frame;
	}
}


/*
 this function was taken from an XML example
 provided by Apple
 
 I can take no credit in this
*/
- (UILabel *)newLabelWithPrimaryColor:(UIColor *)primaryColor selectedColor:(UIColor *)selectedColor fontSize:(CGFloat)fontSize bold:(BOOL)bold
{
	/*
	 Create and configure a label.
	 */
	
    UIFont *font;
    if (bold) {
        font = [UIFont boldSystemFontOfSize:fontSize];
    } else {
        font = [UIFont systemFontOfSize:fontSize];
    }
    
    /*
	 Views are drawn most efficiently when they are opaque and do not have a clear background, so set these defaults.  To show selection properly, however, the views need to be transparent (so that the selection color shows through).  This is handled in setSelected:animated:.
	 */
	UILabel *newLabel = [[UILabel alloc] initWithFrame:CGRectZero];
	newLabel.backgroundColor = [UIColor whiteColor];
	newLabel.opaque = YES;
	newLabel.textColor = primaryColor;
	newLabel.highlightedTextColor = selectedColor;
	newLabel.font = font;
	
	return newLabel;
}


- (void)dealloc {
	// make sure you free the memory
	[titleLabel dealloc];
	[urlLabel dealloc];
	[super dealloc];
}


@end
