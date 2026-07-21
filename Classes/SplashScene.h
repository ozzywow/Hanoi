#pragma once

#include "cocos2d.h"
using namespace cocos2d;

// ── 시그니처(브랜드) 인트로 씬 ────────────────────────────────────────────────
// 시스템 스플래시(OS) 직후 잠깐 노출되는 브랜드 화면.
// 타이틀과 동일한 우주 배경(NewUI/title_bg.png)을 히어로로 사용 → MainScene으로
// 이어질 때 "배경은 그대로, UI만 등장"하는 매끄러운 연결을 만든다.
class SplashScene : public Scene
{
public:
	static SplashScene* createScene() { return SplashScene::create(); }
	CREATE_FUNC(SplashScene);

	virtual bool init() override;

private:
	bool m_leaving = false;   // 전환 중복 방지 (타이머 + 탭 스킵)
	void gotoMain();
};
