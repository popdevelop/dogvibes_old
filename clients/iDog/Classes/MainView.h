//
//  MainView.h
//  iDog
//
//  Created by Johan Nyström on 2010-05-11.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>


@interface MainView : UIView {
	CGPoint startTouchPosition;
	CGPoint endTouchPosition;
	UILabel *label;
}

@property (nonatomic, retain) UILabel *label;

@end
