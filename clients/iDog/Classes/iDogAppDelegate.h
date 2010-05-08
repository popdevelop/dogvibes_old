//
//  iDogAppDelegate.h
//  iDog
//
//  Created by Johan Nyström on 2009-05-26.
//  Copyright NoName 2009. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface iDogAppDelegate : NSObject <UIApplicationDelegate, UITabBarControllerDelegate> {
    UIWindow *window;
    UITabBarController *tabBarController;
	UINavigationController *navController;
	NSString *curTrack;
	NSString *dogIP;
	NSString *kDogVibesIP;
	NSTimer *dogTimer;
	int elapsedSec;
	int duration;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet UITabBarController *tabBarController;
@property (nonatomic, retain) IBOutlet UINavigationController *navController;
@property (nonatomic, retain) NSString *curTrack;
@property (nonatomic, retain) NSString *dogIP;
@property (nonatomic, retain) NSString *kDogVibesIP;
@property (nonatomic, retain) NSTimer *dogTimer;

- (NSString*) getCurTrack;
- (void) setCurTrack:(NSString *)uri;

@end
