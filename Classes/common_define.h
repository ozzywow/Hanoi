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

static std::string countryToFlag(const std::string& cc)
{
    if (cc.size() != 2) return cc;
    uint32_t a = 0x1F1E6u + ((unsigned char)toupper((unsigned char)cc[0]) - 'A');
    uint32_t b = 0x1F1E6u + ((unsigned char)toupper((unsigned char)cc[1]) - 'A');
    auto u8 = [](uint32_t cp) -> std::string {
        return { (char)(0xF0 | (cp >> 18)),
                 (char)(0x80 | ((cp >> 12) & 0x3F)),
                 (char)(0x80 | ((cp >> 6)  & 0x3F)),
                 (char)(0x80 | (cp         & 0x3F)) };
    };
    return u8(a) + u8(b);
}

// 디바이스 언어에 맞는 자국어 인삿말 반환 (greetings.plist 참조)
static std::string getLocalGreeting()
{
    auto langType = Application::getInstance()->getCurrentLanguage();
    std::string nativeLang;
    switch (langType) {
        case LanguageType::KOREAN:     nativeLang = "KOREAN";     break;
        case LanguageType::JAPANESE:   nativeLang = "JAPANESE";   break;
        case LanguageType::CHINESE:    nativeLang = "CHINESE";    break;
        case LanguageType::FRENCH:     nativeLang = "FRENCH";     break;
        case LanguageType::SPANISH:    nativeLang = "SPANISH";    break;
        case LanguageType::GERMAN:     nativeLang = "GERMAN";     break;
        case LanguageType::ITALIAN:    nativeLang = "ITALIAN";    break;
        case LanguageType::PORTUGUESE: nativeLang = "PORTUGUESE"; break;
        case LanguageType::RUSSIAN:    nativeLang = "RUSSIAN";    break;
        case LanguageType::DUTCH:      nativeLang = "DUTCH";      break;
        case LanguageType::TURKISH:    nativeLang = "TURKISH";    break;
        case LanguageType::NORWEGIAN:  nativeLang = "NORWEGIAN";  break;
        case LanguageType::POLISH:     nativeLang = "POLISH";     break;
        case LanguageType::HUNGARIAN:  nativeLang = "HUNGARIAN";  break;
        case LanguageType::ARABIC:     nativeLang = "ARABIC";     break;
        default:                        nativeLang = "ENGLISH";    break;
    }
    auto data = FileUtils::getInstance()->getValueVectorFromFile("greetings.plist");
    for (auto& v : data) {
        auto& d = v.asValueMap();
        if (d["lang"].asString() == nativeLang)
            return d["text"].asString();
    }
    return "Hello";
}

static RecordTime getRecordTime(int time)
{
	RecordTime res;
	res.min = time / (1000 * 60);
	res.sec = (time % (1000 * 60)) / 1000;
	res.ms = ((time % (1000 * 60)) % 1000) / 10;
	return res;
}

