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
static void setupLedBackground(DrawNode* node, float width, float height, float border = 3.0f)
{
    node->drawSolidRect(
        Vec2(-width/2, -height/2 - border), Vec2(width/2, height/2 + border),
        Color4F(0.03f, 0.03f, 0.03f, 1.0f));
    node->drawSolidRect(
        Vec2(-width/2, -height/2), Vec2(width/2, height/2),
        Color4F(0.03f, 0.09f, 0.09f, 1.0f));
}

static void setupLedDotOverlay(DrawNode* node, float width, float height,
                                float gap = 2.0f, float radius = 0.5f)
{
    const Color4F dotColor(0.30f, 0.30f, 0.30f, 1.0f);
    for (float dy = -height/2 + gap/2; dy < height/2; dy += gap)
        for (float dx = -width/2  + gap/2; dx < width/2;  dx += gap)
            node->drawDot(Vec2(dx, dy), radius, dotColor);
}

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

// 쇼핑카트 벡터 아이콘: 바구니(좌) + 역-L 손잡이(우) + 바퀴 2개
// MainScene (Buy 버튼) 및 PlayScene (잠금 안내) 공용
static void drawVecCartIcon(DrawNode* node, float cx, float cy, float sz, const Color4F& col)
{
	float lw = sz * 0.10f;
	if (lw < 2.0f) lw = 2.0f;
	Color4F bg(0.08f, 0.19f, 0.19f, 1.0f);

	float bx0 = cx - sz*0.42f, bx1 = cx + sz*0.18f;
	float by0  = cy - sz*0.20f, by1 = cy + sz*0.10f;
	node->drawSolidRect(Vec2(bx0,    by0), Vec2(bx0+lw, by1),    col);
	node->drawSolidRect(Vec2(bx1-lw, by0), Vec2(bx1,    by1),    col);
	node->drawSolidRect(Vec2(bx0,    by0), Vec2(bx1,  by0+lw),   col);
	node->drawSolidRect(Vec2(bx0,  by1-lw), Vec2(bx1,    by1),   col);

	float hvTop = cy + sz*0.40f;
	node->drawSolidRect(Vec2(bx1-lw,  by1-lw), Vec2(bx1,          hvTop), col);
	node->drawSolidRect(Vec2(bx1-lw, hvTop-lw), Vec2(cx+sz*0.50f,  hvTop), col);

	float wr = lw * 1.3f, wy = by0 - wr*0.8f;
	node->drawSolidCircle(Vec2(bx0+sz*0.12f, wy), wr,      0, 14, col);
	node->drawSolidCircle(Vec2(bx1-sz*0.08f, wy), wr,      0, 14, col);
	node->drawSolidCircle(Vec2(bx0+sz*0.12f, wy), wr*0.4f, 0, 10, bg);
	node->drawSolidCircle(Vec2(bx1-sz*0.08f, wy), wr*0.4f, 0, 10, bg);
}

static Node* makeVecCartNode(float w, float h, float sz)
{
	auto node = Node::create();
	node->setContentSize(Size(w, h));
	node->setCascadeOpacityEnabled(true);
	auto dn = DrawNode::create();
	drawVecCartIcon(dn, w/2.f, h/2.f, sz, Color4F(80/255.f, 220/255.f, 180/255.f, 1.f));
	node->addChild(dn);
	return node;
}

// 🔒 자물쇠: 두께있는 반원 고리 + 직사각형 몸통 + 열쇠구멍 (MainScene/PlayScene 공용)
static void drawVecLock(DrawNode* node, float cx, float cy, float sz, const Color4F& col)
{
	Color4F bg(0.08f, 0.19f, 0.19f, 1.0f);

	float archR  = sz * 0.26f;
	float bw     = archR * 2.2f;
	float bh     = sz * 0.42f;
	float by0    = cy - sz * 0.34f;
	float by1    = by0 + bh;
	node->drawSolidRect(Vec2(cx-bw/2, by0), Vec2(cx+bw/2, by1), col);

	float archIR = archR - sz * 0.11f;
	const int N  = 12;
	for (int i = 0; i < N; ++i) {
		float a0 = (float)M_PI * i / N;
		float a1 = (float)M_PI * (i + 1) / N;
		Vec2 pts[4] = {
			Vec2(cx + cosf(a0)*archIR, by1 + sinf(a0)*archIR),
			Vec2(cx + cosf(a0)*archR,  by1 + sinf(a0)*archR),
			Vec2(cx + cosf(a1)*archR,  by1 + sinf(a1)*archR),
			Vec2(cx + cosf(a1)*archIR, by1 + sinf(a1)*archIR),
		};
		node->drawSolidPoly(pts, 4, col);
	}

	float kr  = sz * 0.10f;
	float kcy = by0 + bh * 0.62f;
	node->drawSolidCircle(Vec2(cx, kcy), kr, 0, 12, bg);
	node->drawSolidRect(Vec2(cx-kr*0.55f, by0+bh*0.20f), Vec2(cx+kr*0.55f, kcy), bg);
}
