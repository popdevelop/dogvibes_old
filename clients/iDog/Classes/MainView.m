//
//  MainView.m
//  iDog
//
//  Created by Johan Nyström on 2010-05-11.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "MainView.h"
#import "DogUtils.h"
#import "FirstViewController.h"

@interface MainView()
// Private Methods
-(void)animateFirstTouchAtPoint:(CGPoint)touchPoint forView:(UIImageView *)theView;
-(void)animateView:(UIView *)theView toPosition:(CGPoint) thePosition;
-(void)dispatchFirstTouchAtPoint:(CGPoint)touchPoint forEvent:(UIEvent *)event;
-(void)dispatchTouchEvent:(UIView *)theView toPosition:(CGPoint)position;
-(void)dispatchTouchEndEvent:(UIView *)theView toPosition:(CGPoint)position;
@end

@implementation MainView

@synthesize label;

- (id)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        // Initialization code
		
    }
    return self;
}

- (void)drawRect:(CGRect)rect {
    // Drawing code
}

// Handles the start of a touch
-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	NSUInteger numTaps = [[touches anyObject] tapCount];
	if(numTaps >= 2) {
		if ((numTaps == 2)) {
			// A double tap positions the three pieces in a diagonal.
			// The user will want to double tap when two or more pieces are on top of each other
		}
	}
	
	// Enumerate through all the touch objects.
	NSUInteger touchCount = 0;
	for (UITouch *touch in touches) {
		// Send to the dispatch method, which will make sure the appropriate subview is acted upon
		[self dispatchFirstTouchAtPoint:[touch locationInView:self] forEvent:nil];
		touchCount++;  
	}
}

// Checks to see which view, or views, the point is in and then calls a method to perform the opening animation,
// which  makes the piece slightly larger, as if it is being picked up by the user.
-(void)dispatchFirstTouchAtPoint:(CGPoint)touchPoint forEvent:(UIEvent *)event
{
	startTouchPosition = touchPoint;
}

// Handles the continuation of a touch.
-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	NSUInteger touchCount = 0;
	// Enumerates through all touch objects
	for (UITouch *touch in touches) {
		// Send to the dispatch method, which will make sure the appropriate subview is acted upon
		[self dispatchTouchEvent:[touch view] toPosition:[touch locationInView:self]];
		touchCount++;
	}	
}

// Checks to see which view, or views, the point is in and then sets the center of each moved view to the new postion.
// If views are directly on top of each other, they move together.
-(void)dispatchTouchEvent:(UIView *)theView toPosition:(CGPoint)position
{
	// Check to see which view, or views,  the point is in and then move to that position.
}

// Handles the end of a touch event.
-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	// Enumerates through all touch object
	for (UITouch *touch in touches) {
		// Sends to the dispatch method, which will make sure the appropriate subview is acted upon
		[self dispatchTouchEndEvent:[touch view] toPosition:[touch locationInView:self]];
	}
}

// Checks to see which view, or views,  the point is in and then calls a method to perform the closing animation,
// which is to return the piece to its original size, as if it is being put down by the user.
-(void)dispatchTouchEndEvent:(UIView *)theView toPosition:(CGPoint)position
{   
	endTouchPosition = position;
	if (fabs(endTouchPosition.x - startTouchPosition.x) > 100 ) {
		DogUtils *dog = [[DogUtils alloc] init];
		NSString *jsonData = [NSString alloc];
		jsonData = [dog dogRequest:@"/amp/0/nextTrack"];
		[dog release];
        FirstViewController *myViewController = [FirstViewController sharedFirstViewController];
        [myViewController updateTrackInfo];
        
	}
	
}

-(void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	// Enumerates through all touch object
	for (UITouch *touch in touches) {
		// Sends to the dispatch method, which will make sure the appropriate subview is acted upon
		[self dispatchTouchEndEvent:[touch view] toPosition:[touch locationInView:self]];
	}
}

// Scales up a view slightly which makes the piece slightly larger, as if it is being picked up by the user.
-(void)animateFirstTouchAtPoint:(CGPoint)touchPoint forView:(UIImageView *)theView 
{
	// Pulse the view by scaling up, then move the view to under the finger.
	[UIView beginAnimations:nil context:nil];
	[UIView setAnimationDuration:1];
	theView.transform = CGAffineTransformMakeScale(1.2, 1.2);
	[UIView commitAnimations];
}

// Scales down the view and moves it to the new position. 
-(void)animateView:(UIView *)theView toPosition:(CGPoint)thePosition
{
	[UIView beginAnimations:nil context:NULL];
	[UIView setAnimationDuration:1];
	// Set the center to the final postion
	theView.center = thePosition;
	// Set the transform back to the identity, thus undoing the previous scaling effect.
	theView.transform = CGAffineTransformIdentity;
	[UIView commitAnimations];	
}

- (void)dealloc {
    [super dealloc];
}


@end
