//
//  DogUtils.m
//  iDog
//
//  Created by Johan Nyström on 2009-11-16.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "DogUtils.h"
#import "JSON.h"
#import "SettingsViewController.h"

@implementation DogUtils

// Customize the number of rows in the table view.
- (NSString *)dogRequest:(NSString *)request{
	SettingsViewController *svc = [SettingsViewController sharedViewController];
	NSString *ip = [svc getIPfromTextField];
	if (ip != nil) {
		NSLog(@"dog: %@, %@", ip, request);
		NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@%@",ip, request, nil]];
		NSString *jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
		return jsonData;
	} else {
		return nil;
	}
}

- (UIImage *) dogGetAlbumArt:(NSString *)uri {
	SettingsViewController *svc = [SettingsViewController sharedViewController];
	NSString *ip = [svc getIPfromTextField];
	if (ip != nil) {
		return [UIImage imageWithData: 
				[NSData dataWithContentsOfURL: 
				 [NSURL URLWithString:
				  [NSString stringWithFormat:
				   @"http://%@/dogvibes/getAlbumArt?size=159&uri=%@",ip, uri, nil]]]];
	} else {
		return nil;
	}
}

@end