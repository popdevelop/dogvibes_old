//
//  dogvibesAppDelegate.h
//  dogvibes
//
//  Created by Johan Nyström on 2009-05-14.
//  Copyright Axis Communications 2009. All rights reserved.
//

#import <UIKit/UIKit.h>

@class dogvibesViewController;

@interface dogvibesAppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
    dogvibesViewController *viewController;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet dogvibesViewController *viewController;

@end

