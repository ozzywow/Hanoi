#pragma once
#include <stdlib.h>
#include "cocos2d.h"
using namespace cocos2d;

#ifdef _WIN32
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif

#define LITE_VER // LITE version
#define MAX_PLAY_LEVEL	10
#define MAX_LIMIT_LEVEL_FOR_LITE 4

#define BUY_AT_STORE_URL "https://itunes.apple.com/app/id504138737?mt=8"
#define kConsumableBaseFeatureId "com.ozzywow.TowerOfHanoiOlympic"
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#define kProductIdTotal "com.ozzywow.nanoi.fullversion"
#else
#define kProductIdTotal "com.ozzywow.TowerOfHanoiOlympic.FullVersion"
#endif
  
enum STR_ANCHO_TYPE
{
	LEFT,
	RIGHT,
	CENTER,
};

#define TEXT_Z_DEF	1000


#define FRAME_HEIGHT				640.0f
#define FRAME_WIDTH					960.0f

// 리소스 해상도 사이즈
#define RESOURCE_HEIGHT				320.0f
#define RESOURCE_WIDTH				480.0f


enum TAG
{
	tagBG,	
	tagBGRankingBoard,	
	tagInfoText,
	tagSelectionMark,
	tagDiscus,
	tagTouchingLayer,
	tagCoin,
	tagPopup,
	tagCountDown,	
	tagCart,
};

enum PLAY_STATE
{
	NONE,
	COUNT_DOWN,
	PLAY,
	COMPLATE,
};


#ifdef _WIN32
static inline unsigned long timeGetTimeEx()
{
    return static_cast<unsigned long>(GetTickCount());
}
#else
static inline unsigned long timeGetTimeEx()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}
#endif




static int getMilliCount()
{
#ifdef _WIN32
	timeb tb;
	ftime(&tb);
	return (int)(tb.millitm + (tb.time & 0xfffff) * 1000);
#else
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (int)((tv.tv_sec & 0xfffff) * 1000 + tv.tv_usec / 1000);
#endif
}

struct RecordTime
{	
	int min;
	int sec;
	int ms;
};
static RecordTime getRecordTime(int time)
{
	RecordTime res;	
	res.min = time / (1000 * 60);
	res.sec = (time % (1000 * 60)) / 1000;
	res.ms = ((time % (1000 * 60)) % 1000) / 10;
	return res;
}
