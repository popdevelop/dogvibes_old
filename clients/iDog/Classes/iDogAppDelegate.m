//
//  iDogAppDelegate.m
//  iDog
//
//  Created by Johan Nyström on 2009-05-26.
//  Copyright NoName 2009. All rights reserved.
//

#import "iDogAppDelegate.h"


@implementation iDogAppDelegate

@synthesize window;
@synthesize tabBarController;
@synthesize navController;
@synthesize curTrack;


- (void)applicationDidFinishLaunching:(UIApplication *)application {
    // Add the tab bar controller's current view as a subview of the window
    [window addSubview:tabBarController.view];
}

// Optional UITabBarControllerDelegate method
- (void)tabBarController:(UITabBarController *)tabBarController didSelectViewController:(UIViewController *)viewController {
	[viewController viewDidLoad];
}

/*
// Optional UITabBarControllerDelegate method
- (void)tabBarController:(UITabBarController *)tabBarController didEndCustomizingViewControllers:(NSArray *)viewControllers changed:(BOOL)changed {
}
*/

- (NSString*) getCurTrack {
	return self.curTrack;
}

- (void) setCurTrack:(NSString *)uri {
	self.curTrack = uri;
}

- (void)dealloc {
    [tabBarController release];
    [window release];
    [super dealloc];
}

@end
