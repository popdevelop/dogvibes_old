//
//  dogvibesViewController.h
//  dogvibes
//
//  Created by Johan Nyström on 2009-05-14.
//

#import <UIKit/UIKit.h>

@interface dogvibesViewController : UIViewController <UITextFieldDelegate> {
	/* gui components */
	IBOutlet UITextField *textField;
	IBOutlet UITextField *IPtextField;
	IBOutlet UIButton *playButton;
	IBOutlet UIButton *nextButton;
	IBOutlet UIButton *prevButton;
	IBOutlet UIButton *stopButton;
	IBOutlet UIButton *searchButton;
	IBOutlet UILabel *label;
	IBOutlet UITextView *textView;
	IBOutlet UISwitch *spk0;
	IBOutlet UISwitch *spk1;
	IBOutlet UISwitch *spk2;
}

/* button handlers */
- (IBAction)playButtonPressed:(id)sender;
- (IBAction)stopButtonPressed:(id)sender;
- (IBAction)nextButtonPressed:(id)sender;
- (IBAction)prevButtonPressed:(id)sender;
- (IBAction)searchButtonPressed:(id)sender;

/* slider handlers */
- (IBAction)slider0Changed:(id)sender;
- (IBAction)slider1Changed:(id)sender;
- (IBAction)slider2Changed:(id)sender;

/* textfield handlers */
- (IBAction)IPtextFieldChanged:(id)sender;

- (NSString *)getIPfromTextField;
@end
