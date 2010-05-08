//
//  iDogAppDelegate.m
//  iDog
//
//  Created by Johan Nyström on 2009-05-26.
//  Copyright NoName 2009. All rights reserved.
//

#import "iDogAppDelegate.h"
#import "FirstViewController.h"

@implementation iDogAppDelegate

@synthesize window;
@synthesize tabBarController;
@synthesize navController;
@synthesize curTrack;
@synthesize dogIP, kDogVibesIP, dogTimer;

- (void)applicationDidFinishLaunching:(UIApplication *)application {
	[window addSubview:tabBarController.view];
}

- (void)tabBarController:(UITabBarController *)tabBarController didSelectViewController:(UIViewController *)viewController {
	[viewController viewDidLoad];
}

- (NSString*) getCurTrack {
	NSLog(@"cur track is: %@ ", self.curTrack);
	return curTrack;
}

- (void) setCurTrack:(NSString *)uri {
	NSLog(@"cur track updated to: %@ ", uri);
	[curTrack autorelease];
	curTrack = [uri retain];
}

- (void)dealloc {
    [tabBarController release];
    [window release];
    [super dealloc];
}

@end
