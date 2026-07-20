#pragma once
#include <stdlib.h>
#include "cocos2d.h"
#include "DrawUtils.h"
using namespace cocos2d;

#ifdef _WIN32
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif

#define LITE_VER // LITE version
#define MAX_PLAY_LEVEL	10
#define MAX_LIMIT_LEVEL_FOR_LITE 4

// 랭킹 Top10 수상소감 기능 — 이 줄을 주석 처리하면 빌드에서 완전히 제외됨
#define ENABLE_AWARD_COMMENT

#define MASTER_VOLUME 1.0f			// 0.0~1.0

#define BUY_AT_STORE_URL "https://apps.apple.com/app/id430261581"

// 설치 링크 공유용 스마트 URL — 클릭 기기(iOS/Android) 자동 판별 후 해당 스토어로 리다이렉트.
// 소스: landing/index.html (GitHub Pages 호스팅)
#define SHARE_URL "https://ozzywow.github.io/Hanoi/"
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
	tagHudBg,
	tagBottomHudBg,
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




// 정의: common_define.cpp (헤더에 두면 TU마다 사본 생성 → cpp로 이동)
int getMilliCount();

struct RecordTime
{
	int min;
	int sec;
	int ms;
};

// 정의: common_define.cpp
std::string countryToFlag(const std::string& cc);

// 디바이스 언어에 맞는 자국어 인삿말 반환 (greetings.plist 참조). 정의: common_define.cpp
std::string getLocalGreeting();

RecordTime getRecordTime(int time);

