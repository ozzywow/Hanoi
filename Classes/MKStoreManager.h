//
//  StoreManager.h
//  MKStoreKit
//
//  Created by Mugunth Kumar on 17-Nov-2010.
//  Copyright 2010 Steinlogic. All rights reserved.
//	File created using Singleton XCode Template by Mugunth Kumar (http://mugunthkumar.com
//  Permission granted to do anything, commercial/non-commercial with this file apart from removing the line/URL above
//  Read my blog post at http://mk.sg/1m on how to use this code

//  As a side note on using this code, you might consider giving some credit to me by
//	1) linking my website from your app's website 
//	2) or crediting me inside the app's credits page 
//	3) or a tweet mentioning @mugunthkumar
//	4) A paypal donation to mugunth.kumar@gmail.com
//
//  A note on redistribution
//	While I'm ok with modifications to this source code, 
//	if you are re-publishing after editing, please retain the above copyright notices

#ifndef MK_STORE_MANAGER_H_
#define MK_STORE_MANAGER_H_

#import <Foundation/Foundation.h>
#import <StoreKit/StoreKit.h>
#import "MKStoreObserver.h"

#define IPAD_VER
// CONFIGURATION STARTS -- Change this in your app
#ifdef IPAD_VER
#define BUY_AT_STORE_URL "https://itunes.apple.com/app/id504138737?mt=8"
#define kConsumableBaseFeatureId @"com.ozzywow.kw4ipadlite"
#define kProductIdStep2 @"com.ozzywow.kw4ipadlite.step2"
#define kProductIdStep3 @"com.ozzywow.kw4ipadlite.step3"
#define kProductIdStep4 @"com.ozzywow.kw4ipadlite.step4"
#define kProductIdStep5 @"com.ozzywow.kw4ipadlite.step5"
#define kProductIdTotal @"com.ozzywow.kw4ipadlite.total"
#else //IPAD_VER
#define BUY_AT_STORE_URL "http://itunes.apple.com/app/id509909625?mt=8"
#define kConsumableBaseFeatureId @"com.ozzywow.kw4iphonelite"
#define kProductIdStep2 @"com.ozzywow.kw4iphonelite.step2"
#define kProductIdStep3 @"com.ozzywow.kw4iphonelite.step3"
#define kProductIdStep4 @"com.ozzywow.kw4iphonelite.step4"
#define kProductIdStep5 @"com.ozzywow.kw4iphonelite.step5"
#define kProductIdTotal @"com.ozzywow.kw4iphonelite.total"
#endif //IPAD_VER
// consumable features should have only number as the last part of the product name
// MKStoreKit automatically keeps track of the count of your consumable product

#define SERVER_PRODUCT_MODEL 0
// CONFIGURATION ENDS -- Change this in your app

@protocol MKStoreKitDelegate <NSObject>
@optional
- (void)productFetchComplete;
- (void)productPurchased:(NSString *)productId;
- (void)transactionCanceled;
- (void)restorePreviousTransactions;
// as a matter of UX, don't show a "User Canceled transaction" alert view here
// use this only to "enable/disable your UI or hide your activity indicator view etc.,
@end

@interface MKStoreManager : NSObject<SKProductsRequestDelegate> {

	NSMutableArray *_purchasableObjects;
	MKStoreObserver *_storeObserver;
	
	BOOL isProductsAvailable;
}

@property (nonatomic, retain) NSMutableArray *purchasableObjects;
@property (nonatomic, retain) MKStoreObserver *storeObserver;

// These are the methods you will be using in your app
+ (MKStoreManager*)sharedManager;

// this is a static method, since it doesn't require the store manager to be initialized prior to calling
+ (BOOL) isFeaturePurchased:(NSString*) featureId;

// these three are not static methods, since you have to initialize the store with your product ids before calling this function
- (void) buyFeature:(NSString*) featureId;
- (NSMutableArray*) purchasableObjectsDescription;
- (void) restorePreviousTransactions;
- (void) cbRetored:(int)count;

- (BOOL) canConsumeProduct:(NSString*) productIdentifier quantity:(int) quantity;
- (BOOL) consumeProduct:(NSString*) productIdentifier quantity:(int) quantity;


//DELEGATES
+(id)delegate;	
+(void)setDelegate:(id)newDelegate;

@end




#endif //MK_STORE_MANAGER_H_
