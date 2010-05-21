//
//  SettingsViewController.m
//  iDog
//
//  Created by Johan Nyström on 2009-05-26.
//  Copyright 2009 NoName. All rights reserved.
//

#import "iDogAppDelegate.h"
#import "DogUtils.h"
#import "SettingsViewController.h"

@implementation SettingsViewController

@synthesize statusBtn, label;

static SettingsViewController *sharedViewController;

- (void)viewDidLoad {
	[super viewDidLoad];
	iDogAppDelegate *iDogApp = (iDogAppDelegate *)[[UIApplication sharedApplication] delegate];
	if (iDogApp.dogIP != nil) {
	  IPtextField.text = iDogApp.dogIP;
	}
}

- (IBAction)statusButtonPressed:(id)sender
{
	DogUtils *dog = [[DogUtils alloc] init];
	NSString *jsonData = [NSString alloc];
	jsonData = [dog dogRequest:@"/amp/0/getStatus"];
		
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Webservice Down" message:@"The webservice you are accessing is down. Please try again later."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];
		[alert release];
	} else {
		label.backgroundColor = [UIColor greenColor];
	}
	
	[dog release];
	[jsonData release];
}

- (IBAction)IPtextFieldChanged:(id)sender
{
	[IPtextField resignFirstResponder];
	iDogAppDelegate *iDogApp = (iDogAppDelegate *)[[UIApplication sharedApplication] delegate];
	textView.text = IPtextField.text;
	iDogApp.dogIP = [(NSString *)CFURLCreateStringByAddingPercentEscapes(
																		 nil,
																		 (CFStringRef)[IPtextField text],
																		 NULL,
																		 NULL,
																		 kCFStringEncodingUTF8) autorelease];
	[[NSUserDefaults standardUserDefaults] setObject:iDogApp.dogIP forKey:iDogApp.kDogVibesIP];
	[[NSUserDefaults standardUserDefaults] synchronize];
}

- (NSString *)getIPfromTextField {
	NSString *escapedIPValue =
	[(NSString *)CFURLCreateStringByAddingPercentEscapes(
														 nil,
														 (CFStringRef)[IPtextField text],
														 NULL,
														 NULL,
														 kCFStringEncodingUTF8) autorelease];
	if ([escapedIPValue length] > 0) {
		return escapedIPValue;
	} else { 
		return nil;
	}
}

-(void)setIPTextField:(NSString *)text
{
	if (text != nil)
		IPtextField.text = text;
}

- (BOOL)textFieldShouldReturn:(UITextField *)theTextField 
{
    if ( theTextField == IPtextField ) {
		[self IPtextFieldChanged:theTextField];
		[IPtextField resignFirstResponder];
	}
    return YES;
}

+(SettingsViewController *)sharedViewController {
	if (!sharedViewController)
		sharedViewController = [[SettingsViewController alloc] init];
	return sharedViewController;
}

+(id)alloc
{
	NSAssert(sharedViewController == nil, @"Attempted to allocate a second instance of a singleton.");
	sharedViewController = [super alloc];
	return sharedViewController;
}

+(id)copy {
	NSAssert(sharedViewController == nil, @"Attempted to copy the singleton");
	return sharedViewController;
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}

- (void)dealloc {
    [super dealloc];
}

@end
