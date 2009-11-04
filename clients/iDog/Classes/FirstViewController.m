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
	SettingsViewController *svc = [SettingsViewController sharedViewController];
	NSString *ip = [svc getIPfromTextField];	
	/* get status from server! */
	NSLog(@"IP: %@", ip);
	NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/getStatus",ip ? ip : @"83.249.229.59:2000", nil]];
	NSString *jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
	/* load album art */
	UIImage *img = [UIImage alloc];
	
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"No reply from server!" message:@"Either the webservice is down (verify with Statusbutton under setting) or else, there's nothing added in playlist."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
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
			img = [UIImage imageWithData: 
				   [NSData dataWithContentsOfURL: 
					[NSURL URLWithString:
					 [NSString stringWithFormat:
					  @"http://%@/dogvibes/getAlbumArt?size=159&uri=%@",ip ? ip : @"83.249.229.59:2000", [result objectForKey:@"uri"],nil]]]];
		} else if (playState){
			/* set play button available */
			[self setPlayButtonImage:[UIImage imageNamed:@"play.png"]];
			NSLog(@"STOPPED, SET PLAY button");
			img = [UIImage imageWithData: 
				   [NSData dataWithContentsOfURL: 
					[NSURL URLWithString:
					 [NSString stringWithFormat:
					  @"http://%@/dogvibes/getAlbumArt?size=159&uri=%@",ip ? ip : @"83.249.229.59:2000", [result objectForKey:@"uri"],nil]]]];
		}
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
	SettingsViewController *svc = [SettingsViewController sharedViewController];
	NSString *ip = [svc getIPfromTextField];
	NSLog(@"IP: %@", ip);
	NSURL *jsonURL;
	NSString *jsonData;
	/* update button */
	if (state != 1) {
		jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/play",ip ? ip : @"83.249.229.59:2000", nil]];
		jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
		state = 1;
		[self setPlayButtonImage:[UIImage imageNamed:@"pause.png"]];
		NSLog(@"switching to state %d ", state);
	} else {
		jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/pause",ip ? ip : @"83.249.229.59:2000", nil]];
		jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
		state = 0;
		[self setPlayButtonImage:[UIImage imageNamed:@"play.png"]];
		NSLog(@"switching to state %d ", state);
	}
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"No reply from server!" message:@"Either the webservice is down (verify with Statusbutton under setting) or else, there's nothing added in playlist."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];
		[alert release];
	}
	/* refresh album and track title */
	[self updateTrackInfo];
	
}

- (IBAction)prevButtonPressed:(id)sender
{
	SettingsViewController *svc = [SettingsViewController sharedViewController];
	NSString *ip = [svc getIPfromTextField];
	NSLog(@"IP: %@", ip);
	NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/previousTrack",ip ? ip : @"83.249.229.59:2000", nil]];
	NSString *jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];	
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Webservice Down" message:@"The webservice you are accessing is down. Please try again later."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];
		[alert release];
	}
	[self updateTrackInfo];
}

- (IBAction)stopButtonPressed:(id)sender
{	
	SettingsViewController *svc = [SettingsViewController sharedViewController];
	NSString *ip = [svc getIPfromTextField];
	NSLog(@"IP: %@", ip);
	NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/stop",ip ? ip : @"83.249.229.59:2000", nil]];
	NSString *jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
	
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Webservice Down" message:@"The webservice you are accessing is down. Please try again later."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];
		[alert release];
	}
}

- (IBAction)nextButtonPressed:(id)sender
{
	
	SettingsViewController *svc = [SettingsViewController sharedViewController];
	NSString *ip = [svc getIPfromTextField];
	NSLog(@"IP: %@", ip);
	NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/nextTrack",ip ? ip : @"83.249.229.59:2000", nil]];
	NSString *jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];	
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Webservice Down" message:@"The webservice you are accessing is down. Please try again later."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];
		[alert release];
	}
	
	[self updateTrackInfo];
}

- (void) updateTrackInfo {
	SettingsViewController *svc = [SettingsViewController sharedViewController];
	NSString *ip = [svc getIPfromTextField];	
	/* get status from server! */
	NSLog(@"IP: %@", ip);
	NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/getStatus",ip ? ip : @"83.249.229.59:2000", nil]];
	NSString *jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
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
