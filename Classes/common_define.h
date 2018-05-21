#pragma once
#include <stdlib.h>
#include "sys/timeb.h"
#include "cocos2d.h"
using namespace cocos2d;

#define LITE_VER // LITE version
#define MAX_PLAY_LEVEL	10
#define MAX_LIMIT_LEVEL_FOR_LITE 4

#define BUY_AT_STORE_URL "https://itunes.apple.com/app/id504138737?mt=8"
#define kConsumableBaseFeatureId "com.ozzywow.TowerOfHanoiOlympic"
#define kProductIdTotal "com.ozzywow.TowerOfHanoiOlympic.FullVersion"

enum STR_ANCHO_TYPE
{
	LEFT,
	RIGHT,
	CENTER,
};

#define TEXT_Z_DEF	1000


#define FRAME_HEIGHT				640.0f
#define FRAME_WIDTH					960.0f

// 원본 리소스 기준 사이즈
#define RESOURCE_HEIGHT				320.0f
#define RESOURCE_WIDTH				480.0f


enum TAG
{
	tagBG,	
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

//static int MAX_WORD_NUM = 8;
static bool arrRandFlag[8];

static void InitRandNum()
{
	memset(arrRandFlag, 0, sizeof(arrRandFlag));
}

static int GetRandNum(int max = 8)
{
	for (int i = 0; i< 100; ++i) {
		int ran = rand() % max;
		if (arrRandFlag[ran] == false)
		{
			arrRandFlag[ran] = true;
			return ran;
		}
	}
	return -1;
};



static void PrintStyle(Node* parent, std::string& str, int fontSize, Point pos)
{
	auto label0 = Label::createWithSystemFont(str, "Arial", fontSize);
	label0->setPosition(pos.x - 1, pos.y);
	label0->setColor(Color3B::BLACK);
	parent->addChild(label0, tagInfoText);

	auto label1 = Label::createWithSystemFont(str, "Arial", fontSize);
	label1->setPosition(pos.x + 1, pos.y);
	label1->setColor(Color3B::BLACK);
	parent->addChild(label1, tagInfoText);

	auto label2 = Label::createWithSystemFont(str, "Arial", fontSize);
	label2->setPosition(pos.x, pos.y - 1);
	label2->setColor(Color3B::BLACK);
	parent->addChild(label2, tagInfoText);

	auto label3 = Label::createWithSystemFont(str, "Arial", fontSize);
	label3->setPosition(pos.x, pos.y + 1);
	label3->setColor(Color3B::BLACK);
	parent->addChild(label3, tagInfoText);

	auto label = Label::createWithSystemFont(str, "Arial", fontSize);
	label->setPosition(pos.x, pos.y + 1);
	label->setColor(Color3B::WHITE);
	parent->addChild(label, tagInfoText);
};


static std::string replace_all(
	const std::string &message,
	const std::string &pattern,
	const std::string &replace
) {

	std::string result = message;
	std::string::size_type pos = 0;
	std::string::size_type offset = 0;

	while ((pos = result.find(pattern, offset)) != std::string::npos)
	{
		result.replace(result.begin() + pos, result.begin() + pos + pattern.size(), replace);
		offset = pos + replace.size();
	}

	return result;
};



static inline unsigned long timeGetTimeEx()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
};




static int getMilliCount()
{
	timeb tb;
	ftime(&tb);
	int nCount;
	nCount = tb.millitm + (tb.time & 0xfffff) * 1000;
	return nCount;

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