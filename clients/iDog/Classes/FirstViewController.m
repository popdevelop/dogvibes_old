//
//  FirstViewController.m
//  iDog
//
//  Created by Johan Nyström on 2009-05-26.
//  Copyright NoName 2009. All rights reserved.
//

#import "FirstViewController.h"
#import "SettingsViewController.h"
#import "iDogAppDelegate.h"
#import "DogUtils.h"
#import "JSON.h"

@implementation FirstViewController

static FirstViewController *sharedFirstViewController;
@synthesize playButton, nextButton, prevButton, jsonImage, seekSlider, volumeSlider, label, timeLabel;

- (void)viewDidLoad {
    [super viewDidLoad];
    [self check_system_prefs];
	
	seekSlider.minimumValue = 0;
	seekSlider.maximumValue = 1000;
	seekSlider.value = 0;
	
	timeLabel.text = [self timeFormatted:0];
	
	/* load album art */
	UIImage *img = [UIImage alloc];
	iDogAppDelegate *appDelegate = (iDogAppDelegate *)[[UIApplication sharedApplication] delegate];
	DogUtils *dog = [[DogUtils alloc] init];
	
	[appDelegate.dogTimer invalidate]; 
	appDelegate.dogTimer = [NSTimer scheduledTimerWithTimeInterval: 1.0
															target:self selector:@selector(timerFunc)
														  userInfo:nil repeats:YES];
	
	if (visitCount != 0) {
		NSString *jsonData = [NSString alloc];
		jsonData = [dog dogRequest:@"/amp/0/getStatus"];
		if (jsonData == nil) {
			UIAlertView *alert = [[UIAlertView alloc] 
								  initWithTitle:@"No reply from server!"   \
								  message:@"Either the webservice is down  \
								  (verify with Statusbutton under setting) \
								  or else there's nothing added in playlist."  
								  delegate:self cancelButtonTitle:@"OK" 
								  otherButtonTitles: nil];
			[alert show];
			[alert release];
		} else {
			NSDictionary *trackDict = [jsonData JSONValue];
			NSDictionary *result = [trackDict objectForKey:@"result"];
			NSString *playState = [NSString stringWithFormat:@"%@",[result objectForKey:@"state"], nil];
            NSString *curDuration = [NSString stringWithFormat:@"%@",[result objectForKey:@"duration"], nil];
            NSString *curElapsed = [NSString stringWithFormat:@"%@",[result objectForKey:@"elapsedmseconds"], nil];
            NSString *curVolume = [NSString stringWithFormat:@"%@",[result objectForKey:@"volume"], nil];
            seekSlider.maximumValue = [curDuration intValue] / 1000;
            seekSlider.value = [curElapsed intValue] / 1000;
            volumeSlider.value = [curVolume floatValue];
            NSLog(@"%f", [curVolume floatValue]);
            
			if ([playState compare:@"playing"] == 0) {
				state = STATE_PLAYING;
				[self setPlayButtonImage:[UIImage imageNamed:@"pause.png"]];
			} else if (playState){
				state = STATE_IDLE;
				[self setPlayButtonImage:[UIImage imageNamed:@"play.png"]];
			}
			
			NSLog(@"new track: %@   curTrack: %@ ", (NSString *)[result objectForKey:@"uri"], [appDelegate getCurTrack]);
			
			if ([(NSString *)[result objectForKey:@"uri"] compare:[appDelegate getCurTrack]] != 0) {
				/* only request image if we need.. */
				img = [dog dogGetAlbumArt:[NSString stringWithFormat:@"artist=%@&album=%@", [result objectForKey:@"artist"], 
										   [result objectForKey:@"album"], nil]];
				NSLog(@"Update album art..");
				jsonImage.image = [img retain];			
			}
			
			label.text = [NSString stringWithFormat:@"%@\n%@\n%@", [result objectForKey:@"artist"], 
						  [result objectForKey:@"title"], [result objectForKey:@"album"], nil];
			[appDelegate setCurTrack:(NSString *)[result objectForKey:@"uri"]];
		}
	}
	visitCount++;
	[img release];
	[dog release];
}

- (void)viewDidAppear:(BOOL)animated
{
}

- (void)setPlayButtonImage:(UIImage *)image
{
	[playButton setImage:image forState:0];
}

- (IBAction)playButtonPressed:(id)sender
{
	DogUtils *dog = [[DogUtils alloc] init];
	NSString *jsonData = [NSString alloc];
	
	/* update button */
	if (state != STATE_PLAYING) {
		state = STATE_PLAYING;
		[self setPlayButtonImage:[UIImage imageNamed:@"pause.png"]];
		jsonData = [dog dogRequest:@"/amp/0/play"];
	} else {
		state = STATE_IDLE;
		[self setPlayButtonImage:[UIImage imageNamed:@"play.png"]];
		jsonData = [dog dogRequest:@"/amp/0/pause"];
	}
	
	[dog release];
	/* refresh album and track title */
	[self updateTrackInfo];	
}

- (IBAction)prevButtonPressed:(id)sender
{
	DogUtils *dog = [[DogUtils alloc] init];
	NSString *jsonData = [NSString alloc];
	jsonData = [dog dogRequest:@"/amp/0/previousTrack"];
	[dog release];
	[self updateTrackInfo];
}

- (IBAction)nextButtonPressed:(id)sender
{
	DogUtils *dog = [[DogUtils alloc] init];
	NSString *jsonData = [NSString alloc];
	jsonData = [dog dogRequest:@"/amp/0/nextTrack"];
	[dog release];
	[self updateTrackInfo];
}

- (void) updateTrackInfo {
	DogUtils *dog = [[DogUtils alloc] init];
	NSString *jsonData = [NSString alloc];
	jsonData = [dog dogRequest:@"/amp/0/getStatus"];
	UIImage *img = [UIImage alloc];
	iDogAppDelegate *appDelegate = (iDogAppDelegate *)[[UIApplication sharedApplication] delegate];
		
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"No reply from server!" message:@"Either the webservice is down (verify with Statusbutton under setting) or else, there's nothing added in playlist."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];
		[alert release];
	} else {
		NSDictionary *trackDict = [jsonData JSONValue];
		NSDictionary *result = [trackDict objectForKey:@"result"];
		label.text = [NSString stringWithFormat:@"%@\n%@\n%@", [result objectForKey:@"artist"], 
					  [result objectForKey:@"title"], [result objectForKey:@"album"], nil];
		if ([(NSString *)[result objectForKey:@"uri"] compare:[appDelegate getCurTrack]] != 0) {
			/* only request image if we need.. */
			NSLog(@"Update albumart!");
			img = [dog dogGetAlbumArt:[NSString stringWithFormat:@"artist=%@&album=%@", [result objectForKey:@"artist"], 
								 [result objectForKey:@"album"], nil]];
			jsonImage.image = [img retain];			
		}
		
		NSString *playState = [NSString stringWithFormat:@"%@",[result objectForKey:@"state"], nil];
		
		if ([playState compare:@"playing"] == 0) {
			state = STATE_PLAYING;
			[self setPlayButtonImage:[UIImage imageNamed:@"pause.png"]];
			NSString *curDuration = [NSString stringWithFormat:@"%@",[result objectForKey:@"duration"], nil];
			NSString *curElapsed = [NSString stringWithFormat:@"%@",[result objectForKey:@"elapsedmseconds"], nil];
			seekSlider.maximumValue = [curDuration intValue] / 1000;
			seekSlider.value = [curElapsed intValue] / 1000;
		} else if (playState){
			state = STATE_IDLE;
			[self setPlayButtonImage:[UIImage imageNamed:@"play.png"]];
		}
		[appDelegate setCurTrack:(NSString *)[result objectForKey:@"uri"]];		
	}
	[dog release];
}

- (IBAction) volumeChanged:(id)sender
{
	UISlider *mySlider = (UISlider *) sender;
	DogUtils *dog = [[DogUtils alloc] init];
	NSString *jsonData = [NSString alloc];
	jsonData = [dog dogRequest:[NSString stringWithFormat:@"/amp/0/setVolume?level=%f",mySlider.value, nil]];
	[dog release];
}

- (IBAction) seekChanged:(id)sender
{
	UISlider *mySlider = (UISlider *) sender;
	DogUtils *dog = [[DogUtils alloc] init];
	NSString *jsonData = [NSString alloc];
	jsonData = [dog dogRequest:[NSString stringWithFormat:@"/amp/0/seek?mseconds=%d",(int)round(mySlider.value*1000), nil]];
	[dog release];
}

- (void) timerFunc {
	if (state == STATE_PLAYING) {
		seekSlider.value++;
		timeLabel.text = [self timeFormatted:seekSlider.value];
		if (seekSlider.value == seekSlider.maximumValue) {
			[self updateTrackInfo];	
		}
	}
}

- (NSString *)timeFormatted:(int)totalSeconds
{
    int seconds = totalSeconds % 60; 
    int minutes = (totalSeconds / 60) % 60; 
    return [NSString stringWithFormat:@"%02d:%02d",minutes, seconds]; 
}

- (void) check_system_prefs {
	iDogAppDelegate *iDogApp = (iDogAppDelegate *)[[UIApplication sharedApplication] delegate];
	iDogApp.kDogVibesIP = @"dogVibesIP";
	NSString *testValue = [[NSUserDefaults standardUserDefaults] stringForKey:iDogApp.kDogVibesIP];
	if (testValue == nil)
	{
		// no default values have been set, create them here based on what's in our Settings bundle info
		//
		NSString *pathStr = [[NSBundle mainBundle] bundlePath];
		NSString *settingsBundlePath = [pathStr stringByAppendingPathComponent:@"Settings.bundle"];
		NSString *finalPath = [settingsBundlePath stringByAppendingPathComponent:@"Root.plist"];
		
		NSDictionary *settingsDict = [NSDictionary dictionaryWithContentsOfFile:finalPath];
		NSArray *prefSpecifierArray = [settingsDict objectForKey:@"PreferenceSpecifiers"];
		
		NSString *dogVibesIPDefault;		
		NSDictionary *prefItem;
		
		for (prefItem in prefSpecifierArray)
		{
			NSString *keyValueStr = [prefItem objectForKey:@"Key"];
			id defaultValue = [prefItem objectForKey:@"DefaultValue"];
			
			if ([keyValueStr isEqualToString:iDogApp.kDogVibesIP])
			{
				dogVibesIPDefault = defaultValue;
			}
			
		}
		
		// since no default values have been set (i.e. no preferences file created), create it here		
		NSDictionary *appDefaults = [NSDictionary dictionaryWithObjectsAndKeys:
									 dogVibesIPDefault, iDogApp.kDogVibesIP,
									 nil];
		[[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];
		[[NSUserDefaults standardUserDefaults] synchronize];
	}
	iDogApp.dogIP = [[NSUserDefaults standardUserDefaults] stringForKey:iDogApp.kDogVibesIP];
}

+(FirstViewController *)sharedFirstViewController {
	if (!sharedFirstViewController)
		sharedFirstViewController = [[FirstViewController alloc] init];
	return sharedFirstViewController;
}

+(id)alloc
{
	NSAssert(sharedFirstViewController == nil, @"Attempted to allocate a second instance of a singleton.");
	sharedFirstViewController = [super alloc];
	return sharedFirstViewController;
}

+(id)copy {
	NSAssert(sharedFirstViewController == nil, @"Attempted to copy the singleton");
	return sharedFirstViewController;
}


- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}

- (void)dealloc {
    [super dealloc];
}

@end
