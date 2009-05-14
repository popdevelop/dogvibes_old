//
//  dogvibesViewController.m
//  dogvibes
//
//  Created by Johan Nyström on 2009-05-14.
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
	
	NSString *dogVibesIP = [NSString stringWithFormat:@"http://%@/amp/0/play", [self getIPfromTextField], nil];
	
	/* adding some tracks at startup */
	NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/queue?uri=spotify:track:75UqWU4Y0YdCB9MrnKZZnC",[self getIPfromTextField], nil]];
	NSString *jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
	jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/queue?uri=spotify:track:515garseWy4j6U5PvGNVIx",[self getIPfromTextField], nil]];
	jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
	jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/queue?uri=spotify:track:4piMVI3W0mzMbtzizmZqG4",[self getIPfromTextField], nil]];
	jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
	jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/queue?uri=spotify:track:6gLweSXN9mFtLo3VyzFLEP",[self getIPfromTextField], nil]];
	jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
	jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/queue?uri=spotify:track:75UqWU4Y0YdCB9MrnKZZnC",[self getIPfromTextField], nil]];
	jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
	
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Webservice Down" message:@"The webservice you are accessing is down. Please try again later."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];
		[alert release];
	}
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
	
	NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/play",[self getIPfromTextField], nil]];
	NSString *jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
	
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Webservice Down" message:@"The webservice you are accessing is down. Please try again later."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];
		[alert release];
	}
}

- (IBAction)prevButtonPressed:(id)sender
{
	NSString *greeting = [[NSString alloc] initWithFormat:@"Previous track"];
	textView.text = greeting;
	textView.textAlignment = UITextAlignmentCenter;
    [greeting release];
	
	NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/previousTrack",[self getIPfromTextField], nil]];
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
	
	NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/stop",[self getIPfromTextField], nil]];
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
	
	NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/nextTrack",[self getIPfromTextField], nil]];
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
	
	NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/dogvibes/search?query=%@", [self getIPfromTextField],escapedValue, nil]];
	NSString *jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
	
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Webservice Down" message:@"The webservice you are accessing is down. Please try again later."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];
		[alert release];
	}
	
	textView.text = jsonData;
	
}

- (IBAction)slider0Changed:(id)sender
{
	NSLog([self getIPfromTextField]);
	NSString *jsonData;
	NSURL *jsonURL;
	if ( spk0.on )
		jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/connectSpeaker?nbr=0",[self getIPfromTextField], nil]];
	else
		jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/disconnectSpeaker?nbr=0", [self getIPfromTextField], nil]];
	
	jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Webservice Down" message:@"The webservice you are accessing is down. Please try again later."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];
		[alert release];
	} else {
		//textView.text = jsonData;
	}
}

- (IBAction)slider1Changed:(id)sender
{
	NSString *jsonData;
	NSURL *jsonURL;
	if ( spk1.on )
		jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/connectSpeaker?nbr=2",[self getIPfromTextField], nil]];
	else
		jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/disconnectSpeaker?nbr=2",[self getIPfromTextField], nil]];
	
	jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Webservice Down" message:@"The webservice you are accessing is down. Please try again later."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];
		[alert release];
	} else {
		//textView.text = jsonData;
	}
	
}

- (IBAction)slider2Changed:(id)sender
{
	NSString *jsonData;
	NSURL *jsonURL;
	if ( spk2.on )
		jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/connectSpeaker?nbr=2",[self getIPfromTextField], nil]];
	else
		jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/disconnectSpeaker?nbr=2",[self getIPfromTextField], nil]];
	
	jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Webservice Down" message:@"The webservice you are accessing is down. Please try again later."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];
		[alert release];
	} else {
		//textView.text = jsonData;
	}	
}

- (IBAction)IPtextFieldChanged:(id)sender
{
	[IPtextField resignFirstResponder];
	printf("Changed textfield! ");
	textView.text = IPtextField.text;
}

- (BOOL)textFieldShouldReturn:(UITextField *)theTextField 
{
    if (theTextField == textField ) {
		[self searchButtonPressed:theTextField];
        [textField resignFirstResponder];
    } else if ( theTextField == IPtextField ) {
		[self IPtextFieldChanged:theTextField];
		[IPtextField resignFirstResponder];
	}
    return YES;
}

- (NSString *)getIPfromTextField {
	NSString *escapedIPValue =
	[(NSString *)CFURLCreateStringByAddingPercentEscapes(
														 nil,
														 (CFStringRef)[IPtextField text],
														 NULL,
														 NULL,
														 kCFStringEncodingUTF8)
	 autorelease];
	return escapedIPValue;
}

@end
