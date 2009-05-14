//
//  dogvibesViewController.h
//  dogvibes
//
//  Created by Johan Nyström on 2009-05-14.
//  Copyright Axis Communications 2009. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface dogvibesViewController : UIViewController {
	IBOutlet UITextField *textField;
	IBOutlet UIButton *playButton;
	IBOutlet UIButton *nextButton;
	IBOutlet UIButton *prevButton;
	IBOutlet UIButton *stopButton;
	IBOutlet UIButton *searchButton;
	IBOutlet UILabel *label;
	IBOutlet UITextView *textView;
}

- (IBAction)playButtonPressed:(id)sender;
- (IBAction)stopButtonPressed:(id)sender;
- (IBAction)nextButtonPressed:(id)sender;
- (IBAction)prevButtonPressed:(id)sender;
- (IBAction)searchButtonPressed:(id)sender;

@end
