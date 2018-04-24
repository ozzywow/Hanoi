//
//  GameConfig.h
//  Hanoi
//
//  Created by Jinguk Hong on 11. 2. 26..
//  Copyright __MyCompanyName__ 2011. All rights reserved.
//

#ifndef __GAME_CONFIG_H
#define __GAME_CONFIG_H

//
// Supported Autorotations:
//		None,
//		UIViewController,
//		CCDirector
//
#define kGameAutorotationNone 0
#define kGameAutorotationCCDirector 1
#define kGameAutorotationUIViewController 2

//
// Define here the type of autorotation that you want for your game
//
#ifdef KR_VERSION
	#ifdef AD_VER
		#define GAME_AUTOROTATION kGameAutorotationUIViewController
	#else
		#define GAME_AUTOROTATION kGameAutorotationNone
	#endif
#else //KR_VERSION
	
	#define GAME_AUTOROTATION kGameAutorotationNone
#endif


#endif // __GAME_CONFIG_H