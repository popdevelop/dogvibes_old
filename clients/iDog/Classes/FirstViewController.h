//
//  FirstViewController.h
//  iDog
//
//  Created by Johan Nyström on 2009-05-26.
//  Copyright NoName 2009. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface FirstViewController : UIViewController {
	IBOutlet UIButton *playButton;
	IBOutlet UIButton *nextButton;
	IBOutlet UIButton *prevButton;
	IBOutlet UISlider *volumeSlider;
	IBOutlet UISlider *seekSlider;
	IBOutlet UIImageView *jsonImage;
	IBOutlet UILabel *label;
	IBOutlet UILabel *timeLabel;
	int state;
	int visitCount;
}

enum {
	STATE_IDLE = 0,
	STATE_PLAYING = 1,
};

@property (nonatomic, retain) IBOutlet UIButton *playButton;
@property (nonatomic, retain) IBOutlet UIButton *prevButton;
@property (nonatomic, retain) IBOutlet UIButton *nextButton;
@property (nonatomic, retain) IBOutlet UISlider *volumeSlider;
@property (nonatomic, retain) IBOutlet UISlider *seekSlider;
@property (nonatomic, retain) IBOutlet UILabel *label;
@property (nonatomic, retain) IBOutlet UILabel *timeLabel;
@property (nonatomic, retain) IBOutlet UIImageView *jsonImage;

- (IBAction)playButtonPressed:(id)sender;
- (IBAction)nextButtonPressed:(id)sender;
- (IBAction)prevButtonPressed:(id)sender;
- (IBAction)volumeChanged:(id)sender;
- (IBAction)seekChanged:(id)sender;

- (void)setPlayButtonImage:(UIImage *)image;
- (void)updateTrackInfo;
- (void)check_system_prefs;
- (void)timerFunc;
- (NSString *)timeFormatted:(int)totalSeconds;

/* export the settingsviewcontroller */
+(FirstViewController *) sharedFirstViewController;
@end
