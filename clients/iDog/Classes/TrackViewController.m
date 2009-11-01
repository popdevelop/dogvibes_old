//
//  TrackViewController.m
//  iDog
//
//  Created by Johan Nyström on 2009-05-27.
//  Copyright 2009 NoName. All rights reserved.
//

#import "TrackViewController.h"
#import "JSON.h"

@implementation TrackViewController

@synthesize jsonLabel, jsonImage, jsonItem, itemID;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
	if (self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil]) {
		// Initialization code
		
		self.itemID = 0;
	}
	return self;
}

-(void)setID:(NSInteger)val {
	self.itemID = (NSInteger *)val;
}


/*
 Implement loadView if you want to create a view hierarchy programmatically
 - (void)loadView {
 }
 */

- (void)viewDidLoad {
	// init the url

#if 0
	NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://iphone.zcentric.com/test-json-get.php?id=%@", self.itemID, nil]];
	
	NSString *jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
	
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Webservice Down" message:@"The webservice you are accessing is down. Please try again later."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];	
		[alert release];
	}
	else {
		self.jsonItem = [jsonData JSONValue]; 
		
		// setting up the title
		self.jsonLabel.text = [self.jsonItem objectForKey:@"title"];
		
		// setting up the image now
		self.jsonImage.image = [UIImage imageWithData: [NSData dataWithContentsOfURL: [NSURL URLWithString: [self.jsonItem objectForKey:@"img"]]]];
	}
#endif
	
}


- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
	// Return YES for supported orientations
	return (interfaceOrientation == UIInterfaceOrientationPortrait);
}


- (void)didReceiveMemoryWarning {
	[super didReceiveMemoryWarning]; // Releases the view if it doesn't have a superview
	// Release anything that's not essential, such as cached data
}


- (void)dealloc {
	[jsonLabel dealloc];
	[jsonImage dealloc];
	[self.jsonItem dealloc];
	[super dealloc];
}


@end
