//
//  dogvibesAppDelegate.m
//  dogvibes
//
//  Created by Johan Nyström on 2009-05-14.
//  Copyright Axis Communications 2009. All rights reserved.
//

#import "dogvibesAppDelegate.h"
#import "dogvibesViewController.h"

@implementation dogvibesAppDelegate

@synthesize window;
@synthesize viewController;


- (void)applicationDidFinishLaunching:(UIApplication *)application {    
    
    // Override point for customization after app launch    
    [window addSubview:viewController.view];
    [window makeKeyAndVisible];
}


- (void)dealloc {
    [viewController release];
    [window release];
    [super dealloc];
}


@end
