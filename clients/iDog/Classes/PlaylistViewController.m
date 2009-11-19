//
//  PlaylistViewController.m
//  iDog
//
//  Created by Johan Nyström on 2009-05-26.
//  Copyright 2009 NoName. All rights reserved.
//

#import "PlaylistViewController.h"
#import "SettingsViewController.h"
#import "JSON.h"

@implementation PlaylistViewController

@synthesize playlistTableView, trackItems, trackURIs;

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
	trackItems = [[NSMutableArray alloc] initWithCapacity: 1000];
	trackURIs = [[NSMutableArray alloc] initWithCapacity: 1000];
	SettingsViewController *svc = [SettingsViewController sharedViewController];
	NSString *ip = [svc getIPfromTextField];
	//NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/getAllTracksInQueue",ip ? ip : @"83.249.229.59:2000",nil]];
	NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/dogvibes/getAllTracksInPlaylist?playlist_id=1",ip ? ip : @"83.249.229.59:2000",nil]];
	
	NSString *jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];	
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Webservice Down" message:@"The webservice you are accessing is down. Please try again later."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];
		[alert release];
	} else {
		NSDictionary *trackDict = [jsonData JSONValue];
		NSDictionary *result = [trackDict objectForKey:@"result"];
		for (id key in result) {
			//NSLog(@"Songs in playlist: %@", [key objectForKey:@"title"]);
			[trackItems addObject:(NSString *)[key objectForKey:@"title"]];
			[trackURIs addObject:(NSString *)[key objectForKey:@"uri"]];
			NSLog(@"%d",[trackItems count]);
		}
	}
}


/*
// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}
*/

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	NSLog(@"called RowsInSection %d", [trackItems count]);
    return [trackItems count];
}

- (UITableViewCell *)tableView:(UITableView *)tv cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	NSLog(@"called cellForIndexPath");
    static NSString *kCellIdentifier = @"MyCell";
    UITableViewCell *cell = [playlistTableView dequeueReusableCellWithIdentifier:kCellIdentifier];
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithFrame:CGRectZero reuseIdentifier:kCellIdentifier] autorelease];
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    }
    cell.text = [trackItems objectAtIndex:indexPath.row];
    return cell;
}

- (void)tableView:(UITableView *)table didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	NSLog(@"Play this song title: %@, %@!", [trackItems objectAtIndex:indexPath.row], [trackURIs objectAtIndex:indexPath.row]);
	SettingsViewController *svc = [SettingsViewController sharedViewController];
	NSString *ip = [svc getIPfromTextField];
	NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/playTrack?nbr=%d",ip ? ip : @"83.249.229.59:2000", indexPath.row, nil]];
	NSString *jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"No reply from server!" message:@"Either the webservice is down (verify with Statusbutton under setting) or else, there's nothing added in playlist."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];
		[alert release];
	}
}


- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated {
}

- (void)viewDidDisappear:(BOOL)animated {
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
    [super dealloc];
	[trackItems dealloc];
	[playlistTableView dealloc];
}

@end
