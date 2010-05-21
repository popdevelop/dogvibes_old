//
//  DogUtils.m
//  iDog
//
//  Created by Johan Nyström on 2009-11-16.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "DogUtils.h"
#import "JSON.h"
#import "iDogAppDelegate.h"
#import "SettingsViewController.h"

@implementation DogUtils

/* TODO!: asynchronous http requests */
- (NSString *)dogRequest:(NSString *)request {
	iDogAppDelegate *iDogApp = (iDogAppDelegate *)[[UIApplication sharedApplication] delegate];
	NSString *ip = iDogApp.dogIP;
	
	int stringLength = [request length];
	NSRange range = NSMakeRange(0, stringLength);
	NSString *newStr = [request stringByReplacingOccurrencesOfString:@" " withString:@"%20" options:NSCaseInsensitiveSearch range:range];	
	
	if (ip != nil) {
		NSLog(@"dog: %@, %@", ip, newStr);
		//synchronous...
		NSURL *jsonURL = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@%@%@",ip, @"/nystrom",newStr, nil]];
		NSString *jsonData = [[NSString alloc] initWithContentsOfURL:jsonURL];
		//asynchronous: TODO..
		if (jsonData == nil) {NSLog(@"NULL!"); return nil;}
		return jsonData;
	} else {
		return nil;
	}
}

- (UIImage *) dogGetAlbumArt:(NSString *)uri {
	iDogAppDelegate *iDogApp = (iDogAppDelegate *)[[UIApplication sharedApplication] delegate];
	NSString *ip = iDogApp.dogIP;
	int stringLength = [uri length];
	NSRange range = NSMakeRange(0, stringLength);
	NSString *newStr = [uri stringByReplacingOccurrencesOfString:@" " withString:@"%20" options:NSCaseInsensitiveSearch range:range];
	if (ip != nil) {
		return [UIImage imageWithData: 
				[NSData dataWithContentsOfURL: 
				 [NSURL URLWithString:
				  [NSString stringWithFormat:
				   @"http://%@%@/dogvibes/getAlbumArt?%@&size=159",ip, @"/nystrom", newStr, nil]]]];
	} else {
		return nil;
	}
}

#if 0 /* TODO! */
- (NSString *) getUrl:(NSString *)req {
	responseData = [[NSMutableData data] retain];
	NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"http://www.dogvibes.com"]];
	[[NSURLConnection alloc] initWithRequest:request delegate:self];
	
	response;
}

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response {
	[responseData setLength:0];
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data {
	[responseData appendData:data];
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error {
	printf("connection failed..%@", [NSString stringWithFormat:@"Connection failed: %@", [error description]]);
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection {
	[connection release];
	
	NSString *responseString = [[NSString alloc] initWithData:responseData encoding:NSUTF8StringEncoding];
	NSLog(@"%@", responseString);
	[responseData release];
	[responseString release];
}
#endif

@end