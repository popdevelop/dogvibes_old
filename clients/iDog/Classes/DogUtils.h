//
//  DogUtils.h
//  iDog
//
//  Created by Johan Nyström on 2009-11-16.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>


@interface DogUtils : NSObject {
	NSMutableArray *responseData;
}

- (NSString*) dogRequest:(NSString *)request;
- (UIImage *) dogGetAlbumArt:(NSString *)uri;
@end

