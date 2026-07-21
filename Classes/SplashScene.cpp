#include "SplashScene.h"
#include "MainScene.h"
#include <algorithm>

// 연출 타이밍
static const float kBgFadeIn   = 0.6f;   // 배경 페이드 인
static const float kKenBurns   = 2.2f;   // 켄번즈 줌아웃(1.06→1.0)
static const float kHold       = 2.0f;   // 총 노출 시간(이후 자동 전환)
static const float kOutFade    = 0.4f;   // MainScene 크로스페이드

bool SplashScene::init()
{
	if (!Scene::init())
		return false;

	auto director = Director::getInstance();
	const Size  vs     = director->getVisibleSize();
	const Vec2  origin = director->getVisibleOrigin();
	const Vec2  center(origin.x + vs.width * 0.5f, origin.y + vs.height * 0.5f);

	// ── 히어로 배경: 타이틀과 동일한 우주 배경(브랜드 연속성) ──
	Sprite* bg = Sprite::create("NewUI/title_bg.png");
	if (bg)
	{
		bg->setAnchorPoint(Vec2(0.5f, 0.5f));
		bg->setPosition(center);

		// 화면을 꽉 채우도록 스케일(레터박스 방지)
		const Size bgSize = bg->getContentSize();
		float fit = std::max(vs.width / bgSize.width, vs.height / bgSize.height);
		bg->setScale(fit * 1.06f);            // 살짝 확대에서 시작
		bg->setOpacity(0);
		this->addChild(bg, 0);

		bg->runAction(FadeIn::create(kBgFadeIn));
		bg->runAction(EaseSineOut::create(ScaleTo::create(kKenBurns, fit)));  // 서서히 줌아웃
	}
	else
	{
		// 배경 로드 실패 시 최소한 브랜드 네이비(엔진 클리어 컬러와 동일)
		this->addChild(LayerColor::create(Color4B(10, 10, 38, 255)), 0);
	}

	// ── SPEEDRUN 서브타이틀 (타이틀 로고 하단) ──
	// title_bg의 "TOWER OF HANOI"는 상단부에 위치 → 그 아래에 자간 넓은 골드 캡션.
	auto tag = Label::createWithSystemFont("S P E E D R U N", "Arial", 20);
	tag->setColor(Color3B(255, 215, 0));                       // 골드(성취 톤)
	tag->enableOutline(Color4B(0, 0, 0, 200), 2);
	tag->setPosition(Vec2(center.x, origin.y + vs.height * 0.60f));
	tag->setOpacity(0);
	tag->setScale(0.85f);
	this->addChild(tag, 2);
	tag->runAction(Sequence::create(
		DelayTime::create(kBgFadeIn * 0.7f),
		Spawn::create(FadeIn::create(0.5f),
		              EaseBackOut::create(ScaleTo::create(0.5f, 1.0f)),
		              nullptr),
		nullptr));

	// ── 하단 로딩 바 (브랜드 시안) — 대기 시간을 시각적으로 페이싱 ──
	const float barW = 180.f, barH = 6.f;
	const Vec2  barPos(center.x, origin.y + vs.height * 0.14f);

	auto barBg = DrawNode::create();                          // 트랙(어두운 배경)
	barBg->drawSolidRect(Vec2(-barW * 0.5f, -barH * 0.5f),
	                     Vec2(barW * 0.5f, barH * 0.5f), Color4F(1, 1, 1, 0.12f));
	barBg->setPosition(barPos);
	this->addChild(barBg, 2);

	auto barFill = DrawNode::create();                        // 채움(왼→오 성장)
	barFill->drawSolidRect(Vec2(0, -barH * 0.5f),
	                      Vec2(barW, barH * 0.5f), Color4F(0.47f, 0.78f, 1.0f, 1.0f));
	barFill->setPosition(Vec2(barPos.x - barW * 0.5f, barPos.y));
	barFill->setScaleX(0.0f);
	this->addChild(barFill, 3);
	barFill->runAction(EaseSineInOut::create(ScaleTo::create(kHold - 0.15f, 1.0f, 1.0f)));

	// ── 자동 전환 + 탭 스킵 ──
	this->runAction(Sequence::create(
		DelayTime::create(kHold),
		CallFunc::create([this]() { gotoMain(); }),
		nullptr));

	auto touch = EventListenerTouchOneByOne::create();
	touch->onTouchBegan = [this](Touch*, Event*) { gotoMain(); return true; };
	_eventDispatcher->addEventListenerWithSceneGraphPriority(touch, this);

	return true;
}

void SplashScene::gotoMain()
{
	if (m_leaving) return;    // 타이머와 탭이 겹쳐도 1회만
	m_leaving = true;

	// 배경이 MainScene과 동일하므로 딥 컬러를 네이비로 → 검은 깜빡임 없이 매끄럽게.
	auto next = MainScene::createScene();
	Director::getInstance()->replaceScene(
		TransitionFade::create(kOutFade, next, Color3B(10, 10, 38)));
}
