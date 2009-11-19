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

/*
// The designated initializer. Override to perform setup that is required before the view is loaded.
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    if (self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil]) {
        // Custom initialization
    }
    return self;
}
*/

/*
// Implement loadView to create a view hierarchy programmatically, without using a nib.
- (void)loadView {
}
*/

// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
	/* todo, album art should be loaded for each track that is playing, default image else */	
	DogUtils *dog = [[DogUtils alloc] init];
	NSString *jsonData = [NSString alloc];
	jsonData = [dog dogRequest:@"/amp/0/getStatus"];
	/* load album art */
	UIImage *img = [UIImage alloc];
	
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
		NSLog(@"play state: %@ ", playState);
		if ([playState compare:@"playing"] == 0) {
			/* set correct button image */
			[self setPlayButtonImage:[UIImage imageNamed:@"pause.png"]];
			NSLog(@"PLAYING, SET PAUSE button");
		} else if (playState){
			/* set play button available */
			[self setPlayButtonImage:[UIImage imageNamed:@"play.png"]];
			NSLog(@"STOPPED, SET PLAY button");
		}
		img = [dog dogGetAlbumArt:[result objectForKey:@"uri"]];
		[self updateTrackInfo];
	}
	
	iDogAppDelegate *appDelegate = (iDogAppDelegate *)[[UIApplication sharedApplication] delegate];
	[appDelegate getCurTrack];
	
	if (img != nil) {
		/* diplay default album art */
		jsonImage.image = img;
	}	else {
		jsonImage.image = [UIImage imageNamed:@"dogvibes_logo.png"];
	}
}

/*
// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}
*/

- (void)setPlayButtonImage:(UIImage *)image
{
	//[playButton.layer removeAllAnimations];
	[playButton setImage:image forState:0];
}


- (IBAction)playButtonPressed:(id)sender
{
	DogUtils *dog = [[DogUtils alloc] init];
	NSString *jsonData = [NSString alloc];
	/* update button */
	if (state != 1) {
		jsonData = [dog dogRequest:@"/amp/0/play"];
		state = 1;
		[self setPlayButtonImage:[UIImage imageNamed:@"pause.png"]];
		NSLog(@"switching to state %d ", state);
	} else {
		jsonData = [dog dogRequest:@"/amp/0/pause"];		
		state = 0;
		[self setPlayButtonImage:[UIImage imageNamed:@"play.png"]];
		NSLog(@"switching to state %d ", state);
	}
	/* refresh album and track title */
	[self updateTrackInfo];	
}

- (IBAction)prevButtonPressed:(id)sender
{
	DogUtils *dog = [[DogUtils alloc] init];
	NSString *jsonData = [NSString alloc];
	jsonData = [dog dogRequest:@"/amp/0/previousTrack"];
	[self updateTrackInfo];
}

- (IBAction)stopButtonPressed:(id)sender
{
	DogUtils *dog = [[DogUtils alloc] init];
	NSString *jsonData = [NSString alloc];
	jsonData = [dog dogRequest:@"/amp/0/stop"];
}

- (IBAction)nextButtonPressed:(id)sender
{
	DogUtils *dog = [[DogUtils alloc] init];
	NSString *jsonData = [NSString alloc];
	jsonData = [dog dogRequest:@"/amp/0/nextTrack"];
	[self updateTrackInfo];
}

- (void) updateTrackInfo {
	DogUtils *dog = [[DogUtils alloc] init];
	NSString *jsonData = [NSString alloc];
	jsonData = [dog dogRequest:@"/amp/0/getStatus"];
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"No reply from server!" message:@"Either the webservice is down (verify with Statusbutton under setting) or else, there's nothing added in playlist."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];
		[alert release];
	} else {
		NSDictionary *trackDict = [jsonData JSONValue];
		NSDictionary *result = [trackDict objectForKey:@"result"];
		label.text = [NSString stringWithFormat:@"%@ - %@", [result objectForKey:@"title"], [result objectForKey:@"album"], nil];
	}
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning]; // Releases the view if it doesn't have a superview
    // Release anything that's not essential, such as cached data
}

- (void)dealloc {
    [super dealloc];
}

@end
