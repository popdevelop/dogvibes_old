//
//  dogvibesViewController.m
//  dogvibes
//
//  Created by Johan Nyström on 2009-05-14.
//  Copyright Axis Communications 2009. All rights reserved.
//

#import "dogvibesViewController.h"

@implementation dogvibesViewController

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
	NSString *greeting = [[NSString alloc] initWithFormat:@"Welcome to dogvibes"];
	textView.text = greeting;
	textView.textAlignment = UITextAlignmentCenter;
    [greeting release];
	
	NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://192.168.1.13:2000/amp/0/queue?uri=spotify:track:75UqWU4Y0YdCB9MrnKZZnC", nil]];
	NSString *jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Webservice Down" message:@"The webservice you are accessing is down. Please try again later."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];
		[alert release];
	}
	jsonData = nil;
	jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://192.168.1.13:2000/amp/0/connectSpeaker?nbr=0", nil]];
	jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Webservice Down" message:@"The webservice you are accessing is down. Please try again later."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];
		[alert release];
	}
	textView.text = jsonData;
}


/*
// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}
*/


- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning]; // Releases the view if it doesn't have a superview
    // Release anything that's not essential, such as cached data
}


- (void)dealloc {
    [super dealloc];
}

- (IBAction)playButtonPressed:(id)sender
{
	NSString *greeting = [[NSString alloc] initWithFormat:@"Playing"];
	textView.text = greeting;
	textView.textAlignment = UITextAlignmentCenter;
    [greeting release];
	
	NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://192.168.1.13:2000/amp/0/play", nil]];
	NSString *jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
	
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Webservice Down" message:@"The webservice you are accessing is down. Please try again later."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];
		[alert release];
	}
	
	textView.text = jsonData;
		
}

- (IBAction)prevButtonPressed:(id)sender
{
	NSString *greeting = [[NSString alloc] initWithFormat:@"Previous track"];
	textView.text = greeting;
	textView.textAlignment = UITextAlignmentCenter;
    [greeting release];
	
	NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://192.168.1.13:2000/amp/0/previousTrack", nil]];
	NSString *jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
	
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Webservice Down" message:@"The webservice you are accessing is down. Please try again later."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];
		[alert release];
	}
	
	textView.text = jsonData;
	
}

- (IBAction)stopButtonPressed:(id)sender
{
	NSString *greeting = [[NSString alloc] initWithFormat:@"Stop"];
	textView.text = greeting;
	textView.textAlignment = UITextAlignmentCenter;
    [greeting release];
	
	NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://192.168.1.13:2000/amp/0/stop", nil]];
	NSString *jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
	
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Webservice Down" message:@"The webservice you are accessing is down. Please try again later."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];
		[alert release];
	}
	
	textView.text = jsonData;
	
}

- (IBAction)nextButtonPressed:(id)sender
{
	NSString *greeting = [[NSString alloc] initWithFormat:@"Next track"];
	textView.text = greeting;
	textView.textAlignment = UITextAlignmentCenter;
    [greeting release];
	
	NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://192.168.1.13:2000/amp/0/nextTrack", nil]];
	NSString *jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
	
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Webservice Down" message:@"The webservice you are accessing is down. Please try again later."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];
		[alert release];
	}
	
	textView.text = jsonData;
	
}

- (IBAction)searchButtonPressed:(id)sender
{
	[textField resignFirstResponder];
	
	NSString *escapedValue =
	[(NSString *)CFURLCreateStringByAddingPercentEscapes(
														 nil,
														 (CFStringRef)[textField text],
														 NULL,
														 NULL,
														 kCFStringEncodingUTF8)
	 autorelease];
	
	NSString *greeting = [[NSString alloc] initWithFormat:@"Searching for %@", escapedValue];
	textView.text = greeting;
	textView.textAlignment = UITextAlignmentCenter;
    [greeting release];
	
	NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://192.168.1.13:2000/dogvibes/search?query=%@", escapedValue, nil]];
	NSString *jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
	
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Webservice Down" message:@"The webservice you are accessing is down. Please try again later."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];
		[alert release];
	}
	
	textView.text = jsonData;
	
}

- (BOOL)textFieldShouldReturn:(UITextField *)sender
{
	[self searchButtonPressed:sender];
	return NO;
}

@end
