//
//  PlaylistViewController.m
//  iDog
//
//  Created by Johan Nyström on 2009-05-26.
//  Copyright 2009 NoName. All rights reserved.
//

#import "PlaylistViewController.h"
#import "SettingsViewController.h"
#import "DogUtils.h"
#import "JSON.h"
#import "iDogAppDelegate.h"

@implementation PlaylistViewController

@synthesize playlistTableView, trackItems, trackURIs, trackDetails;

- (void)awakeFromNib
{
    trackItems = [[NSMutableArray alloc] initWithCapacity: 100];
    trackDetails = [[NSMutableArray alloc] initWithCapacity: 100];
	trackURIs = [[NSMutableArray alloc] initWithCapacity: 100];
}

- (void)viewDidLoad {
	[super viewDidLoad];
	
    [trackURIs removeAllObjects];
    [trackItems removeAllObjects];
    [trackDetails removeAllObjects];
	
	DogUtils *dog = [[DogUtils alloc] init];
	NSString *jsonData = [NSString alloc];
	jsonData = [dog dogRequest:[NSString stringWithFormat:@"/amp/0/getAllTracksInQueue", nil]];
	[dog release];
	
	if (jsonData != nil) {
		NSDictionary *trackDict = [jsonData JSONValue];
		NSDictionary *result = [trackDict objectForKey:@"result"];

		for (id key in result) {
			if ([key objectForKey:@"title"]) {
				[trackItems addObject:(NSString *)[key objectForKey:@"title"]];
                NSString *track = [NSString stringWithFormat:@"%@ - %@", [key objectForKey:@"artist"], [key objectForKey:@"album"], nil];
                [trackDetails addObject:track];
				[trackURIs addObject:(NSString *)[key objectForKey:@"uri"]];
			}
		}
		[playlistTableView reloadData];
	}
	[jsonData release];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return [trackItems count];
}

- (UITableViewCell *)tableView:(UITableView *)tv cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *kCellIdentifier = @"MyCell";
    UITableViewCell *cell = [playlistTableView dequeueReusableCellWithIdentifier:kCellIdentifier];
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:kCellIdentifier] autorelease];
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    }
    cell.textLabel.text = [trackItems objectAtIndex:indexPath.row];
    cell.detailTextLabel.text = [trackDetails objectAtIndex:indexPath.row];
    return cell;
}

- (void)tableView:(UITableView *)table didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	DogUtils *dog = [[DogUtils alloc] init];
	NSString *jsonData = [NSString alloc];
	jsonData = [dog dogRequest:[NSString stringWithFormat:@"/amp/0/playTrack?nbr=%d",indexPath.row, nil]];
	[dog release];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated {
}

- (void)viewDidDisappear:(BOOL)animated {
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
	return (interfaceOrientation == UIInterfaceOrientationPortrait);
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}

- (void)dealloc {
	[trackItems release];
	[trackURIs release];
	[playlistTableView release];
    [trackDetails release];
	[super dealloc];
}

@end
