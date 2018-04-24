//
//  HanoiAppDelegate.h
//  Hanoi
//
//  Created by Jinguk Hong on 11. 2. 26..
//  Copyright __MyCompanyName__ 2011. All rights reserved.
//

#import <UIKit/UIKit.h>
//#import "AdViewController.h"

@class RootViewController;

@interface HanoiAppDelegate : NSObject <UIApplicationDelegate> {
	UIWindow			*window;
	RootViewController	*viewController;
	//AdViewController	*adController;
}

@property (nonatomic, retain) UIWindow *window;

@end
