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
		self.itemID = 0;
	}
	return self;
}

-(void)setID:(NSInteger)val {
	self.itemID = (NSInteger *)val;
}

- (void)viewDidLoad {
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
	return (interfaceOrientation == UIInterfaceOrientationPortrait);
}

- (void)didReceiveMemoryWarning {
	[super didReceiveMemoryWarning];
}

- (void)dealloc {
	[jsonLabel dealloc];
	[jsonImage dealloc];
	[self.jsonItem dealloc];
	[super dealloc];
}

@end