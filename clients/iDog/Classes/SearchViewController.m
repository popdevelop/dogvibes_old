//
//  SearchViewController.m
//  iDog
//
//  Created by Johan Nyström on 2009-05-26.
//  Copyright 2009 NoName. All rights reserved.
//

#import "SearchViewController.h"
#import "SettingsViewController.h"
#import "TrackViewController.h"
#import "iDogAppDelegate.h"
#import "ImageCell.h"

#import "JSON/JSON.h"

@implementation SearchViewController

@synthesize jsonArray, jsonItem, mySearchBar, myTableView;
@synthesize listContent, filteredListContent, savedContent, trackItems, trackURIs;


/*
 - (id)initWithStyle:(UITableViewStyle)style {
 // Override initWithStyle: if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
 if (self = [super initWithStyle:style]) {
 }
 return self;
 }
 */

- (void)viewDidLoad {
	
	
	/* show some items from a search on the prinzen! */
	
	
	
    [super viewDidLoad];
	
    // Uncomment the following line to display an Edit button in the navigation bar for this view controller.
    // self.navigationItem.rightBarButtonItem = self.editButtonItem;
}

/*
 - (void)viewWillAppear:(BOOL)animated {
 [super viewWillAppear:animated];
 }
 */
/*
 - (void)viewDidAppear:(BOOL)animated {
 [super viewDidAppear:animated];
 }
 */
/*
 - (void)viewWillDisappear:(BOOL)animated {
 [super viewWillDisappear:animated];
 }
 */
/*
 - (void)viewDidDisappear:(BOOL)animated {
 [super viewDidDisappear:animated];
 }
 */

/*
 // Override to allow orientations other than the default portrait orientation.
 - (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
 // Return YES for supported orientations
 return (interfaceOrientation == UIInterfaceOrientationPortrait);
 }
 */

- (void)awakeFromNib
{	
	// create the master list
	listContent = [[NSArray alloc] initWithObjects:	@"Please search for a song!", nil];
	
	// create our filtered list that will be the data source of our table, start its content from the master "listContent"
	
	//filteredListContent = [[NSMutableArray alloc] initWithCapacity: [listContent count]];
	trackItems = [[NSMutableArray alloc] initWithCapacity: 1000];
	trackURIs = [[NSMutableArray alloc] initWithCapacity: 1000];
	[trackItems addObjectsFromArray: listContent];
	
	// this stored the current list in case the user cancels the filtering
	//savedContent = [[NSMutableArray alloc] initWithCapacity: [listContent count]]; 
	
	// don't get in the way of user typing
	mySearchBar.autocorrectionType = UITextAutocorrectionTypeNo;
	mySearchBar.autocapitalizationType = UITextAutocapitalizationTypeNone;
	mySearchBar.showsCancelButton = NO;
}


#pragma mark UIViewController

- (void)viewWillAppear:(BOOL)animated
{
	NSIndexPath *tableSelection = [myTableView indexPathForSelectedRow];
	[myTableView deselectRowAtIndexPath:tableSelection animated:NO];
}


- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning]; // Releases the view if it doesn't have a superview
    // Release anything that's not essential, such as cached data
}

#pragma mark Table view methods

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}


// Customize the number of rows in the table view.
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return [trackItems count];
}


- (UITableViewCell *)tableView:(UITableView *)tv cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	NSLog(@"called cellForIndexPath");
    static NSString *kCellIdentifier = @"MyCell";
    UITableViewCell *cell = [myTableView dequeueReusableCellWithIdentifier:kCellIdentifier];
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithFrame:CGRectZero reuseIdentifier:kCellIdentifier] autorelease];
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    }
    cell.text = [trackItems objectAtIndex:indexPath.row];
    return cell;
}


- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {

	// load the clicked cell
	//ImageCell *cell = (ImageCell *)[tableView cellForRowAtIndexPath:indexPath];
	
	// init the controller
	//TrackViewController *controller = [[TrackViewController alloc] initWithNibName:@"TrackView" bundle:nil];
	
	// set the ID and call JSON in the controller
	//[controller setID:[cell getID]];
	// show the view
	
	NSLog(@"adding %@ to playqueue", [trackURIs objectAtIndex:indexPath.row]);
	
	NSLog(@"Play this song title: %@, %@!", [trackItems objectAtIndex:indexPath.row], [trackURIs objectAtIndex:indexPath.row]);
	SettingsViewController *svc = [SettingsViewController sharedViewController];
	NSString *ip = [svc getIPfromTextField];
	NSLog(@"IP: %@", ip);
	NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/amp/0/queue?uri=%@",ip ? ip : @"83.249.229.59:2000", [trackURIs objectAtIndex:indexPath.row], nil]];
	NSString *jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
	
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"No reply from server!" message:@"Either the webservice is down (verify with Statusbutton under setting) or else, there's nothing added in playlist."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];
		[alert release];
	}
	
	//[self.navController pushViewController:controller animated:YES];	
}

/*
 // Override to support conditional editing of the table view.
 - (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
 // Return NO if you do not want the specified item to be editable.
 return YES;
 }
 */


/*
 // Override to support editing the table view.
 - (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
 
 if (editingStyle == UITableViewCellEditingStyleDelete) {
 // Delete the row from the data source
 [tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:YES];
 }   
 else if (editingStyle == UITableViewCellEditingStyleInsert) {
 // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
 }   
 }
 */


/*
 // Override to support rearranging the table view.
 - (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath {
 }
 */


/*
 // Override to support conditional rearranging of the table view.
 - (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
 // Return NO if you do not want the item to be re-orderable.
 return YES;
 }
 */


#pragma mark UISearchBarDelegate

- (void)searchBarTextDidBeginEditing:(UISearchBar *)searchBar
{
	// only show the status bar's cancel button while in edit mode
	mySearchBar.showsCancelButton = YES;
	
	// flush and save the current list content in case the user cancels the search later
	[savedContent removeAllObjects];
	[savedContent addObjectsFromArray: trackItems];
}

- (void)searchBarTextDidEndEditing:(UISearchBar *)searchBar
{
	mySearchBar.showsCancelButton = NO;
}

- (void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText
{
	[trackItems removeAllObjects];	// clear the filtered array first
	
	// search the table content for cell titles that match "searchText"
	// if found add to the mutable array and force the table to reload
	//
	NSString *cellTitle;
	NSString *json_track;
	
	/* let's put the search contents in the table */
	SettingsViewController *svc = [SettingsViewController sharedViewController];
	NSString *ip = [svc getIPfromTextField];
	NSLog(@"IP: %@", ip);
	NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@/dogvibes/search?query=%@",ip ? ip : @"83.249.229.59:2000", searchText,nil]];
	NSString *jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];	
	if (jsonData == nil) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Webservice Down" message:@"The webservice you are accessing is down. Please try again later."  delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil];
		[alert show];
		[alert release];
	} else {
		jsonArray = [jsonData JSONValue];
		NSDictionary *tsdic = [jsonData JSONValue];
		/* Dictionary!!! */
		for (id key in [tsdic objectForKey:@"result"]) {
			[trackItems addObject:(NSString *)[key objectForKey:@"title"]];
			[trackURIs addObject:(NSString *)[key objectForKey:@"uri"]];
		}		
	}

	[myTableView reloadData];
}

// called when cancel button pressed
- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar
{
	NSLog(@"Cancelled search!");
	
	// if a valid search was entered but the user wanted to cancel, bring back the saved list content
	if (searchBar.text.length > 0)
	{
		[trackItems removeAllObjects];
		//[trackItems addObjectsFromArray: savedContent];
	}
	
	[myTableView reloadData];
	
	[searchBar resignFirstResponder];
	searchBar.text = @"";
}

// called when Search (in our case "Done") button pressed
- (void)searchBarSearchButtonClicked:(UISearchBar *)searchBar
{
	NSLog(@"searchButtonClicked!");
	[searchBar resignFirstResponder];
}


- (void)dealloc {
	[myTableView release];
	[mySearchBar release];
	[listContent release];
	[filteredListContent release];
	[savedContent release];
	[trackItems release];
	[trackURIs release];
	[super dealloc];
}

@end
