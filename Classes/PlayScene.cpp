#include "stdafx.h"
#include "PlayScene.h"
#include "PlaySceneTouchHandlerLayer.h"
#include "Discus.h"
#include "SoundFactory.h"
#include "MainScene.h"
#include "UserDataManager.h"
#include "LeaderboardManager.h"
#include "DrawUtils.h"
#ifdef LITE_VER
#include "MKStoreManager_cpp.h"
#endif

using namespace cocos2d;

static Point arrPosOfPole[3];
static const float BOTTOM_PANEL_Y      = 28.0f;
static const float BOTTOM_FONT_DEFAULT = 10.0f;
static const int   TAG_GUIDE_ANIM      = 201;
static const int   TAG_PODIUM_BASE     = 310;



void PlayScene::onExitTransitionDidStart()
{
	stopIdleAnimation();
	stopEqualizerAnimation();
	this->stopAllActions();
	stopRankTicker();
	if (m_aliveFlag) *m_aliveFlag = false;

#ifdef LITE_VER
	this->isProgress = false;
	this->isRestored = false;
	CMKStoreManager::Instance()->ToggleIndicator(false);
#endif //#ifdef LITE_VER
}



bool PlayScene::initWithDiscusNum(int numOfDiscus)
{
	if (!Scene::init())
	{
		return false;
	}

	m_aliveFlag = std::make_shared<bool>(true);

#ifdef LITE_VER
	this->isProgress = false;
	this->isRestored = false;
	CMKStoreManager::Instance()->SetDelegate(this);
#endif //LITE_VER
	
	m_countOfDiscus = numOfDiscus;

	arrPosOfPole[0] = Vec2(105, 140);
	arrPosOfPole[1] = Vec2(241, 140);
	arrPosOfPole[2] = Vec2(377, 140);

	std::vector<Discus*> poleQueue0;
	std::vector<Discus*> poleQueue1;
	std::vector<Discus*> poleQueue2;

		
	m_pole.push_back(poleQueue0);
	m_pole.push_back(poleQueue1);
	m_pole.push_back(poleQueue2);

	

	// 터치 이벤트를 담당할 Layer를 만든 후 GameScene에 넣습니다.
	m_touchHanderLayer = PlaySceneTouchHandlerLayer::createAppleTouchedHandleLayer(this);
	this->addChild(m_touchHanderLayer, tagTouchingLayer, tagTouchingLayer);

	
	Sprite* backGround = Sprite::create("NewUI/main_bg.png");
	if (backGround)
	{
		backGround->setAnchorPoint(Point::ZERO);				
		this->addChild(backGround, tagBG, tagBG);
	}
	

	m_selectionMark = Sprite::create("NewUI/block_selectEff.png");
	if (m_selectionMark)
	{
		BlendFunc bfMask = {backend::BlendFactor::SRC_ALPHA, backend::BlendFactor::ONE};
		m_selectionMark->setBlendFunc(bfMask);
		m_selectionMark->setOpacity(100);
		m_selectionMark->setAnchorPoint(Point(0.5, 0));
		this->addChild(m_selectionMark, tagSelectionMark, tagSelectionMark);
		m_selectionMark->setPosition(500, 500);
	}

	m_deselectionMark = Sprite::create("NewUI/block_selectEff.png") ;
	if (m_deselectionMark)
	{
		BlendFunc bfMask = {backend::BlendFactor::SRC_ALPHA, backend::BlendFactor::ONE};
		m_deselectionMark->setBlendFunc(bfMask);
		m_deselectionMark->setOpacity(100);
		m_deselectionMark->setAnchorPoint(Point(0.5, 0));
		this->addChild(m_deselectionMark, tagSelectionMark, tagSelectionMark);
		m_deselectionMark->setPosition(500, 500);
	}


	// LED 전광판 HUD
	const float LP_W = RESOURCE_WIDTH;
	const float LP_H = 20.0f;
	const float BRD  = 3.0f;
	const Vec2  LED_POS(LP_W * 0.5f, RESOURCE_HEIGHT - 13.0f);

	auto ledBg = DrawNode::create();
	setupLedBackground(ledBg, LP_W, LP_H, BRD);
	ledBg->setPosition(LED_POS);
	this->addChild(ledBg, 1, tagHudBg);

	// 카운트다운 신호등 (InfoBar 중앙, COUNT_DOWN 상태에서만 표시)
	{
		const float dotY       = RESOURCE_HEIGHT - 13.0f;
		const float dotCenterX = RESOURCE_WIDTH * 0.5f;
		const float dotSpacing = 28.0f;
		for (int i = 0; i < 3; ++i) {
			auto dot = DrawNode::create();
			dot->drawDot(Vec2::ZERO, 5.5f, Color4F(0.35f, 0.35f, 0.35f, 0.85f));
			dot->setPosition(Vec2(dotCenterX + (i - 1) * dotSpacing, dotY));
			dot->setVisible(false);
			this->addChild(dot, 4);
			m_trafficDot[i] = dot;
		}
	}

	// 타이머 (PLAY 상태에서만 표시)
	m_labelTime = Label::createWithSystemFont("00:00.00", "Arial", 13);
	m_labelTime->setColor(Color3B::WHITE);
	m_labelTime->setPosition(Vec2(362, RESOURCE_HEIGHT - 13));
	m_labelTime->setVisible(false);
	this->addChild(m_labelTime, 4, tagInfoText);

	// RPM 레이블 (PLAY 상태에서만 표시)
	m_labelRPM = Label::createWithSystemFont("RPM 0", "Arial", 11);
	m_labelRPM->setColor(Color3B(80, 220, 180));
	m_labelRPM->setPosition(Vec2(RESOURCE_WIDTH * 0.5f, RESOURCE_HEIGHT - 13));
	m_labelRPM->setVisible(false);
	this->addChild(m_labelRPM, 4);

	// 티커 레이블 (NONE: 랭킹 슬라이딩, COMPLATE: 결과 깜빡임)
	m_hudTickerLabel = Label::createWithSystemFont("", "Arial", 11);
	m_hudTickerLabel->setAnchorPoint(Vec2(0, 0.5f));
	m_hudTickerLabel->setColor(Color3B::WHITE);
	m_hudTickerLabel->setPosition(Vec2(RESOURCE_WIDTH + 5, RESOURCE_HEIGHT - 13));
	this->addChild(m_hudTickerLabel, 4);

	{
		auto dimDots = DrawNode::create();
		setupLedDotOverlay(dimDots, LP_W, LP_H);
		dimDots->setPosition(LED_POS);
		this->addChild(dimDots, 3);
	}

	// ─── 하단 LED 전광판 (3구역) ──────────────────────────────────
	{
		const float BOT_H   = 50.0f;
		const float BOT_BRD = 3.0f;
		const float BOT_Y   = 28.0f;
		const float DIV1    = (arrPosOfPole[0].x + arrPosOfPole[1].x) * 0.5f;
		const float DIV2    = (arrPosOfPole[1].x + arrPosOfPole[2].x) * 0.5f;

		const Vec2 BOT_CENTER(RESOURCE_WIDTH * 0.5f, BOT_Y);
		auto botBg = DrawNode::create();
		setupLedBackground(botBg, RESOURCE_WIDTH, BOT_H, BOT_BRD);
		botBg->drawLine(Vec2(DIV1 - BOT_CENTER.x, -BOT_H/2), Vec2(DIV1 - BOT_CENTER.x, BOT_H/2),
			Color4F(0.5f, 0.5f, 0.5f, 0.8f));
		botBg->drawLine(Vec2(DIV2 - BOT_CENTER.x, -BOT_H/2), Vec2(DIV2 - BOT_CENTER.x, BOT_H/2),
			Color4F(0.5f, 0.5f, 0.5f, 0.8f));
		botBg->setPosition(BOT_CENTER);
		this->addChild(botBg, 1, tagBottomHudBg);

		auto botDots = DrawNode::create();
		setupLedDotOverlay(botDots, RESOURCE_WIDTH, BOT_H);
		botDots->setPosition(BOT_CENTER);
		this->addChild(botDots, 3);

		m_bottomLabelA = Label::createWithSystemFont("", "Arial", 10);
		m_bottomLabelA->setAnchorPoint(Vec2(0.5f, 0.5f));
		m_bottomLabelA->setPosition(Vec2(arrPosOfPole[0].x, BOT_Y));
		m_bottomLabelA->setColor(Color3B(80, 220, 180));
		this->addChild(m_bottomLabelA, tagInfoText);

		m_bottomLabelB = Label::createWithSystemFont("", "Arial", 10);
		m_bottomLabelB->setAnchorPoint(Vec2(0.5f, 0.5f));
		m_bottomLabelB->setPosition(Vec2(arrPosOfPole[1].x, BOT_Y));
		m_bottomLabelB->setColor(Color3B(80, 220, 180));
		this->addChild(m_bottomLabelB, tagInfoText);

		m_bottomLabelC = Label::createWithSystemFont("", "Arial", 10);
		m_bottomLabelC->setAnchorPoint(Vec2(0.5f, 0.5f));
		m_bottomLabelC->setPosition(Vec2(arrPosOfPole[2].x, BOT_Y));
		m_bottomLabelC->setColor(Color3B(80, 220, 180));
		this->addChild(m_bottomLabelC, tagInfoText);
	}


	m_discusLayer = Layer::create();
	this->addChild(m_discusLayer);


	int offset = 10 / m_countOfDiscus;
	for (int i = 0; i<m_countOfDiscus; ++i)
	{
		Discus* pDiscus = Discus::createScene(i*offset);
		if (pDiscus)
		{
			m_discusLayer->addChild(pDiscus, tagDiscus, tagDiscus);
			m_arrDiscurs.push_back(pDiscus);
		}
	}

	
	bool soundOpt = UserDataManager::Instance()->GetSoundOpt();
	if (!m_isFirstPlay)
		this->DrawMenu(soundOpt);
	this->InitGame();
	this->ResetGame();
	this->DrawDiscus();
	this->EnterWaitingState();


	// 최최 3Level 플레이 시, 화살표/규칙 안내 이미지 표시
	if (m_isFirstPlay && m_countOfDiscus == 3)
	{
		Sprite* imgCount = NULL;
		imgCount = Sprite::create("NewUI/arrow.png");
		this->addChild(imgCount, tagCountDown, tagCountDown);
		imgCount->setAnchorPoint(Vec2(0.5, 0.5));
		imgCount->setPosition(Vec2(480 / 2, 220));
		
		Sprite* text = NULL;		
		text = Sprite::create("NewUI/text_en.png");
		text->setAnchorPoint(Vec2(0.5, 0.5));
		text->setPosition(Vec2(imgCount->getContentSize().width / 2, (imgCount->getContentSize().height / 2) - 80));
		imgCount->addChild(text);
	}

#ifdef LITE_VER
	else if(m_countOfDiscus > MAX_LIMIT_LEVEL_FOR_LITE)
	{
		if (false == UserDataManager::Instance()->GetCart())
		{
			// 자물쇠 아이콘 + 8방향 검은 외곽선
			auto lockNode = Node::create();
			lockNode->setContentSize(Size(90.f, 90.f));
			lockNode->setCascadeOpacityEnabled(true);
			static const float lox[8] = { 1,-1, 0, 0, 1, 1,-1,-1 };
			static const float loy[8] = { 0, 0, 1,-1, 1,-1, 1,-1 };
			auto lockShadow = DrawNode::create();
			// 3단계 그라데이션: 안→밖으로 alpha 감소 → 부드러운 글로우 테두리
			static const struct { float d; float a; } rings[] = {{1.f,0.45f},{2.f,0.25f},{3.f,0.10f}};
			for (auto& r : rings)
				for (int i = 0; i < 8; ++i)
					drawVecLock(lockShadow, 45.f + lox[i]*r.d, 45.f + loy[i]*r.d, 60.f, Color4F(0,0,0,r.a));
			lockNode->addChild(lockShadow, 0);
			auto lockMain = DrawNode::create();
			drawVecLock(lockMain, 45.f, 45.f, 60.f, Color4F(80/255.f, 220/255.f, 180/255.f, 1.f));
			lockNode->addChild(lockMain, 1);
			auto cartMenuItem = MenuItemLabel::create(lockNode, CC_CALLBACK_1(PlayScene::callbackLockBtn, this));
			Menu* pMenu = Menu::create(cartMenuItem, NULL);
			pMenu->setPosition(Vec2(105, 200));
			this->addChild(pMenu, tagCart, tagCart);

			auto action = Blink::create(10, 10);
			pMenu->runAction(action);
		}
	}
#endif //LITE_VER

	return true;
}


PlayScene::~PlayScene()
{
	SoundFactory::Instance()->stop(const_cast<char*>(m_bgmName.c_str()));
	m_pole.clear();
	m_arrDiscurs.clear();	
}



void	PlayScene::DrawMenu(bool SoundOpt)
{
	this->removeChildByTag(1234, false);

	// ─── LED/네온 좌측 컨트롤 도크 ───────────────────────────────────
	// 정보 패널은 "도트 화면", 이 도크는 "키캡+프레임 컨트롤"로 차별화해 터치 UI임을 명확히 한다.
	const Color3B MINT(80, 220, 180);     // 네온 민트 (HUD 공통색)
	const Color4F MINT4(0.31f, 0.86f, 0.70f, 0.9f);
	const float   DX = 24.0f;             // 도크 중심 X (게임 화면 가림 최소화)
	const float   PANEL_W = 40.0f;        // 패널 폭 (축소)
	const float   PANEL_CY = 177.0f;      // 패널 중심 Y
	const float   PANEL_H = 190.0f;       // 패널 높이
	const float   KW = 32.0f;             // 키캡 공통 폭 (좌우 정렬 통일)

	// 아이콘/스테퍼 Y 위치 (위→아래, 8px 균일 간격)
	const float Y_HOME    = 254.0f;
	const float Y_RESET   = 216.0f;
	const float Y_STEPPER = 158.0f;   // ▲/숫자/▼ 통합 키캡 중심
	const float Y_NEXT    = 178.0f;   // ▲
	const float Y_NUM     = 158.0f;   // 현재 디스크 수
	const float Y_PREV    = 138.0f;   // ▼
	const float Y_SPEAKER = 100.0f;

	// 컨테이너: 배경/키캡/도트/메뉴/숫자를 한 덩어리로 (tag 1234 일괄 교체)
	auto dock = Node::create();
	dock->setPosition(Vec2::ZERO);
	this->addChild(dock, 1234, 1234);

	// 배경 패널 + 네온 외곽 프레임 (테두리 없는 정보 패널과 구분)
	auto bg = DrawNode::create();
	setupLedBackground(bg, PANEL_W, PANEL_H, 4.0f);
	bg->drawRect(Vec2(-PANEL_W/2 + 1, -PANEL_H/2 - 3), Vec2(PANEL_W/2 - 1, PANEL_H/2 + 3), MINT4);
	bg->setPosition(Vec2(DX, PANEL_CY));
	dock->addChild(bg, 0);

	auto cells = DrawNode::create();
	cells->setPosition(Vec2(DX, PANEL_CY));
	drawKeycap(cells, 0, Y_HOME    - PANEL_CY, KW, 30.0f);
	drawKeycap(cells, 0, Y_RESET   - PANEL_CY, KW, 30.0f);
	drawKeycap(cells, 0, Y_STEPPER - PANEL_CY, KW, 70.0f);
	drawKeycap(cells, 0, Y_SPEAKER - PANEL_CY, KW, 30.0f);
	dock->addChild(cells, 1);

	// 아주 옅은 도트 (정보 패널보다 성기고 흐리게 — 톤만 살짝)
	auto dots = DrawNode::create();
	dots->setPosition(Vec2(DX, PANEL_CY));
	for (float dy = -PANEL_H/2 + 1.5f; dy < PANEL_H/2; dy += 3.0f)
		for (float dx = -PANEL_W/2 + 1.5f; dx < PANEL_W/2; dx += 3.0f)
			dots->drawDot(Vec2(dx, dy), 0.5f, Color4F(0.35f, 0.40f, 0.38f, 0.20f));
	dock->addChild(dots, 2);

	// 검은 외곽선 텍스트: 시스템 폰트는 enableOutline이 무시되므로 8방향 복제로 외곽선 합성
	// (아이콘 PNG의 검은 테두리와 무게감 통일). contentSize 지정 → MenuItemLabel 터치영역 확보.
	// thickness: 외곽선 픽셀 두께(링 수). 아이콘 PNG 테두리와 톤 맞추려면 2 권장.
	auto makeOutlined = [](const std::string& s, float fs, Color3B col, int thickness) -> Node* {
		auto base = Label::createWithSystemFont(s, "Arial", fs);
		Size sz = base->getContentSize();
		auto n = Node::create();
		n->setContentSize(sz);
		n->setCascadeOpacityEnabled(true);
		n->setCascadeColorEnabled(true);
		Vec2 c(sz.width * 0.5f, sz.height * 0.5f);
		static const float ox[8] = { 1, -1,  0,  0,  1,  1, -1, -1 };
		static const float oy[8] = { 0,  0,  1, -1,  1, -1,  1, -1 };
		for (int d = 1; d <= thickness; ++d)
			for (int i = 0; i < 8; ++i) {
				auto o = Label::createWithSystemFont(s, "Arial", fs);
				o->setColor(Color3B::BLACK);
				o->setAnchorPoint(Vec2(0.5f, 0.5f));
				o->setPosition(c + Vec2(ox[i] * d, oy[i] * d));
				n->addChild(o, 0);
			}
		auto t = Label::createWithSystemFont(s, "Arial", fs);
		t->setColor(col);
		t->setAnchorPoint(Vec2(0.5f, 0.5f));
		t->setPosition(c);
		n->addChild(t, 1);
		return n;
	};

	// 이미지 아이콘: 흰색 PNG를 민트로 틴트 (× 곱연산 → 네온 글로우 통일)
	auto tintMint = [&](MenuItemImage* item) {
		item->setCascadeColorEnabled(true);
		item->setColor(MINT);
	};

	MenuItemImage* homeMenuItem = MenuItemImage::create("NewUI/btn_home_n.png", "NewUI/btn_home_s.png", CC_CALLBACK_1(PlayScene::callbackOnPushed_homeMenuItem, this));
	homeMenuItem->setScale(0.72f);
	homeMenuItem->setPosition(Vec2(DX, Y_HOME));
	tintMint(homeMenuItem);

	auto resetNode = Node::create();
	resetNode->setContentSize(Size(KW, 30.f));
	resetNode->setCascadeOpacityEnabled(true);
	// 8방향 검은 그림자 (makeOutlined 와 동일 방식)
	static const float ox[8] = { 1,-1, 0, 0, 1, 1,-1,-1 };
	static const float oy[8] = { 0, 0, 1,-1, 1,-1, 1,-1 };
	auto shadowDn = DrawNode::create();
	for (int i = 0; i < 8; ++i)
		drawVecReset(shadowDn, KW/2 + ox[i]*2.0f, 15.f + oy[i]*2.0f, 20.f, Color4F(0,0,0,0.75f));
	resetNode->addChild(shadowDn, 0);
	auto mainDn = DrawNode::create();
	drawVecReset(mainDn, KW/2, 15.f, 20.f, Color4F(80/255.f, 220/255.f, 180/255.f, 1.f));
	resetNode->addChild(mainDn, 1);
	auto resetMenuItem = MenuItemLabel::create(resetNode, CC_CALLBACK_1(PlayScene::callbackOnPushed_resetMenuItem, this));
	resetMenuItem->setPosition(Vec2(DX, Y_RESET));

	// ▲▼ 디스크 스테퍼: 외곽선 텍스트(두께 2로 아이콘 톤 맞춤) + MenuItemLabel(내장 zoom), 한계치 dim
	bool canInc = (m_countOfDiscus < MAX_PLAY_LEVEL);
	bool canDec = (m_countOfDiscus > 3);

	Node* nextNode = makeOutlined("\xE2\x96\xB2", 20.0f, MINT, 2); // ▲
	nextNode->setOpacity(canInc ? 255 : 90);
	auto nextMenuItem = MenuItemLabel::create(nextNode, CC_CALLBACK_1(PlayScene::callbackOnPushed_nextMenuItem, this));
	nextMenuItem->setPosition(Vec2(DX, Y_NEXT));

	Node* prevNode = makeOutlined("\xE2\x96\xBC", 20.0f, MINT, 2); // ▼
	prevNode->setOpacity(canDec ? 255 : 90);
	auto prevMenuItem = MenuItemLabel::create(prevNode, CC_CALLBACK_1(PlayScene::callbackOnPushed_prevMenuItem, this));
	prevMenuItem->setPosition(Vec2(DX, Y_PREV));

	// BGM 텍스트: ON=풀 민트, OFF=딤드(opacity 70)
	Node* noteNode = makeOutlined("BGM", 11.0f, MINT, 2);
	noteNode->setOpacity(SoundOpt ? 255 : 70);
	auto speakerMenuItem = MenuItemLabel::create(noteNode, CC_CALLBACK_1(PlayScene::callbackOnPushed_speakerMenuItem, this));
	speakerMenuItem->setPosition(Vec2(DX, Y_SPEAKER));

	// 메뉴 항목은 모두 절대 위치 지정 (자동 정렬 미사용)
	Menu* dockMenu = Menu::create(homeMenuItem, resetMenuItem, nextMenuItem, prevMenuItem, (MenuItem*)speakerMenuItem, NULL);
	dockMenu->setPosition(Vec2::ZERO);
	dock->addChild(dockMenu, 3);

	// 디스크 수 = 정보 영역(비클릭): 버튼과 구분되게 외곽선 없이, 밝기 낮추고, 도트를 더 얹어 "표시 readout" 느낌
	// 숫자 뒤 도트 패치 — 도크 기본 도트보다 촘촘(gap 2)·진하게(alpha 0.45)
	auto numDots = DrawNode::create();
	numDots->setPosition(Vec2(DX, Y_NUM));
	for (float dy = -9.0f; dy <= 9.0f; dy += 2.0f)
		for (float dx = -KW/2 + 2.0f; dx <= KW/2 - 2.0f; dx += 2.0f)
			numDots->drawDot(Vec2(dx, dy), 0.5f, Color4F(0.35f, 0.45f, 0.42f, 0.45f));
	dock->addChild(numDots, 3);

	const Color3B NUM_DIM(58, 158, 130);   // 민트를 어둡게 (정보 톤)
	auto numLabel = Label::createWithSystemFont(StringUtils::format("%d", m_countOfDiscus), "Arial", 17);
	numLabel->setColor(NUM_DIM);
	numLabel->setPosition(Vec2(DX, Y_NUM));
	dock->addChild(numLabel, 4);
}

void	PlayScene::InitGame()
{
	this->removeChildByTag(tagPopup, false);
	m_isIng = NONE ;
	
	
	
	m_countDown=3;
	m_labelTime->setString("00:00.00");

}


// 게임 대기(NONE) 상태의 정보바/하단 패널 연출을 씬 진입 시와 동일하게 복원한다.
// 씬 최초 진입과 결과 팝업 닫은 직후(재플레이 대기) 양쪽에서 공유한다.
void	PlayScene::EnterWaitingState()
{
	// 결과 HUD 잔류(깜빡임 티커) 및 PLAY 전용 표시(타이머/레벨/RPM/신호등) 정리
	stopRankTicker();
	if (m_labelTime)  m_labelTime->setVisible(false);
	if (m_labelLevel) m_labelLevel->setVisible(false);
	for (int i = 0; i < 3; ++i) if (m_trafficDot[i]) m_trafficDot[i]->setVisible(false);
	if (m_labelRPM) { m_labelRPM->stopAllActions(); m_labelRPM->setOpacity(255); m_labelRPM->setVisible(false); }
	m_rpmStartTime = 0;
	m_rpmBlinking  = false;
	m_rpmSmoothed  = 0.0f;

	this->DrawInfoText();
	this->startRankTicker(m_countOfDiscus);
	if (m_countOfDiscus == 3)
		this->startGuideAnimation();
	else
		this->startIdleAnimation();
}


PLAY_STATE PlayScene::GetPlayState()
{
	return m_isIng;
}

void PlayScene::Start()
{
	m_isIng = PLAY;
	stopRankTicker();

	// 신호등: 1초 유지 후 페이드아웃
	for (int i = 0; i < 3; ++i) {
		if (!m_trafficDot[i]) continue;
		m_trafficDot[i]->stopAllActions();
		auto dot = m_trafficDot[i];
		dot->runAction(Sequence::create(
			DelayTime::create(1.0f),
			FadeOut::create(0.4f),
			CallFunc::create([dot]{ dot->setVisible(false); dot->setOpacity(255); }),
			nullptr));
	}
	// RPM: 1초 후 신호등 페이드와 동시에 페이드인
	if (m_labelRPM) {
		m_labelRPM->setString("RPM 0");
		m_labelRPM->setOpacity(0);
		m_labelRPM->setVisible(true);
		m_labelRPM->runAction(Sequence::create(
			DelayTime::create(1.0f),
			FadeIn::create(0.4f),
			nullptr));
	}
	// m_labelLevel, m_labelTime 는 카운트다운부터 이미 표시 중

	scheduleOnce([this](float){ startEqualizerAnimation(); }, 1.5f, "cheer_start");

	// MainScene BGM 선택값 읽어 트랙 결정
	// bgm_selection: 0=Random, 1=Space, 2=Universe, 3=Cosmos, 4=Nova, 5=Moon, 6=Earth
	// s_bgmTracks(MainScene) 순서와 동일: index 0=Space, 1=Universe, 2=Cosmos, 3=Nova, 4=Moon, 5=Earth
	static const char* BGM_LIST[] = {"bgm_space", "bgm_universe", "bgm_cosmos", "bgm_nova", "bgm_moon", "bgm_earth"};
	const int BGM_LIST_COUNT = (int)(sizeof(BGM_LIST) / sizeof(BGM_LIST[0]));
	{
		auto ud  = UserDefault::getInstance();
		int sel  = ud->getIntegerForKey("bgm_selection", 0);
		if (sel == 0) {
			// Random: 매 판마다 랜덤 트랙
			m_bgmName = BGM_LIST[rand() % BGM_LIST_COUNT];
		} else {
			// 특정 트랙 고정 재생
			m_bgmName = BGM_LIST[(sel - 1) % BGM_LIST_COUNT];
		}
	}
	bool bSoundOpt = UserDataManager::Instance()->GetSoundOpt();
	// switchBGM: 이전 BGM(MainScene 카세트 등) 무조건 교체
	SoundFactory::Instance()->switchBGM(m_bgmName.c_str());
	if (!bSoundOpt)
		CocosDenshion::SimpleAudioEngine::getInstance()->pauseBackgroundMusic();

	// RPM 초기화 — BGM은 RPM이 오르면서 페이드인
	m_rpmTouchCount = 0;
	m_rpmSmoothed   = 0.0f;
	m_bgmCurrentVol = 0.0f;
	if (bSoundOpt)
		CocosDenshion::SimpleAudioEngine::getInstance()->setBackgroundMusicVolume(0.0f);
	m_dateTime = getMilliCount();
	m_rpmStartTime = m_dateTime;
	m_lastActivityMs = m_dateTime;   // 첫판 SKIP 무활동 판정 기준
	m_skipShown = false;
	
	
	
	DelayTime* delay = DelayTime::create(0.1);

		
	std::function<void()> func = std::bind(&PlayScene::DrawTime, this);
	CallFunc* callFunc_drawTime = CallFunc::create(func);
	
	Sequence* actionToDrawTime = Sequence::create(callFunc_drawTime, delay, NULL);
		
	m_actionTimeRun = RepeatForever::create(actionToDrawTime);
	
	this->runAction(m_actionTimeRun);
}


void PlayScene::Finished()
{
	m_isIng = COMPLATE;
	m_popupShownTime = 0;
	stopEqualizerAnimation();
	unschedule("cheer_start");
	clearBottomPanels();
	this->stopAction(m_actionTimeRun);
	// 최종 RPM 저장 (리셋 전)
	if (m_rpmStartTime > 0) {
		float elapsedMs = std::max(2000.0f, (float)(getMilliCount() - m_rpmStartTime));
		float rpm = (float)m_rpmTouchCount / (elapsedMs / 60000.0f);
		m_finalRPM = (int)std::min(rpm, 999.0f);
	}
	m_rpmStartTime = 0;
	m_rpmBlinking  = false;
	m_rpmSmoothed  = 0.0f;
	if (m_labelRPM) { m_labelRPM->stopAllActions(); m_labelRPM->setOpacity(255); m_labelRPM->setVisible(false); }
	// BGM 볼륨을 mastVolume으로 복원 후 페이드아웃
	CocosDenshion::SimpleAudioEngine::getInstance()->setBackgroundMusicVolume(
		SoundFactory::Instance()->m_mastVolume);
	int elapsedTime = getMilliCount() - m_dateTime;
	SoundFactory::Instance()->fadeOutBGM(1.0f);

	m_mastTime = elapsedTime;
	RecordTime rt = getRecordTime(m_mastTime);

	// HUD: 타이머 숨기고 결과 텍스트 표시
	if (m_labelTime) m_labelTime->setVisible(false);
	int bestRecord = UserDataManager::Instance()->GetBestRecord(m_countOfDiscus);
	bool isNewRecord = (bestRecord == 0 || bestRecord > m_mastTime);
	showHudResult(isNewRecord, rt);
	if (isNewRecord)
		SoundFactory::Instance()->play("efs_new_record");
	else
		SoundFactory::Instance()->play("efs_clean");

	DelayTime* delay = DelayTime::create(2.0);
	std::function<void()> func = std::bind(&PlayScene::MessagePopup, this);
	CallFunc* callFucn = CallFunc::create(func);
	this->runAction(Sequence::create(delay, callFucn, NULL));
}


void PlayScene::MessagePopup()
{
	m_popupShownTime = getMilliCount();
	SoundFactory::Instance()->play("efs_click");

	if (m_mastTime < 100) m_mastTime = 100;
	RecordTime recordTime = getRecordTime(m_mastTime);

	bool isNewRecord = false;
	int bestRecordTime = UserDataManager::Instance()->GetBestRecord(m_countOfDiscus);
	if (bestRecordTime == 0 || (bestRecordTime > 0 && bestRecordTime > m_mastTime))
	{
		UserDataManager::Instance()->SetBestRecord(m_countOfDiscus, m_mastTime);
		if (m_countOfDiscus <= MAX_PLAY_LEVEL)
			UserDataManager::Instance()->SetLevel(m_countOfDiscus);
		UserDataManager::Instance()->SaveUserData();
		if (!m_isFirstPlay) {
			LeaderboardManager::Instance()->submitScore(m_countOfDiscus, m_mastTime);
			UserDataManager::Instance()->m_justGotNewRecord = true;
		}
		isNewRecord = true;
	}

	// 딤 오버레이
	auto overlay = LayerColor::create(Color4B(0, 0, 0, 0), RESOURCE_WIDTH, RESOURCE_HEIGHT);
	overlay->setPosition(Vec2::ZERO);
	this->addChild(overlay, tagPopup, tagPopup);
	overlay->runAction(FadeTo::create(0.25f, 160));

	// 팝업 박스
	const float PW = 280, PH = 185;
	auto popupBox = LayerColor::create(Color4B(10, 15, 50, 230), PW, PH);
	popupBox->setPosition(Vec2((RESOURCE_WIDTH - PW) / 2, (RESOURCE_HEIGHT - PH) / 2));
	popupBox->setScale(0.7f);
	overlay->addChild(popupBox);
	popupBox->runAction(Sequence::create(
		ScaleTo::create(0.15f, 1.05f),
		ScaleTo::create(0.08f, 1.0f),
		nullptr
	));

	// 타이틀
	std::string titleStr = isNewRecord ? "NEW RECORD!" : "LEVEL CLEAR";
	Color3B titleColor = isNewRecord ? Color3B(255, 215, 0) : Color3B(100, 220, 255);
	auto titleLabel = Label::createWithSystemFont(titleStr, "Arial", 22);
	titleLabel->setColor(titleColor);
	titleLabel->setPosition(Vec2(PW / 2, PH - 28));
	popupBox->addChild(titleLabel);

	// 구분선
	auto divider = DrawNode::create();
	divider->drawLine(Vec2(20, PH - 52), Vec2(PW - 20, PH - 52), Color4F(0.5f, 0.5f, 0.5f, 0.8f));
	popupBox->addChild(divider);

	// 레벨 정보
	auto levelLabel = Label::createWithSystemFont(
		StringUtils::format("LEVEL %d", m_countOfDiscus), "Arial", 11);
	levelLabel->setColor(Color3B(160, 160, 160));
	levelLabel->setPosition(Vec2(PW / 2, PH - 78));
	popupBox->addChild(levelLabel);

	// 기록 시간
	std::string timeStr = StringUtils::format("%02d:%02d.%02d", recordTime.min, recordTime.sec, recordTime.ms);
	auto timeLabel = Label::createWithSystemFont(timeStr, "Arial", 26);
	timeLabel->setColor(Color3B::WHITE);
	timeLabel->setPosition(Vec2(PW / 2, PH - 110));
	popupBox->addChild(timeLabel);

	// 최종 RPM — 수치에 따라 색상 구분
	{
		Color3B rpmCol;
		if      (m_finalRPM >= 100) rpmCol = Color3B( 80, 220,  80);
		else if (m_finalRPM >   50) rpmCol = Color3B(255, 215,   0);
		else                        rpmCol = Color3B(255,  80,  80);

		auto rpmResultLabel = Label::createWithSystemFont(
			StringUtils::format("AVG RPM  %d", m_finalRPM), "Arial", 11);
		rpmResultLabel->setColor(rpmCol);
		rpmResultLabel->setPosition(Vec2(PW / 2, PH - 143));
		popupBox->addChild(rpmResultLabel);
	}

	// 힌트 (깜빡임)
	std::string hintStr = m_isFirstPlay ? "TAP TO CONTINUE" : "TAP TO PLAY AGAIN";
	auto hintLabel = Label::createWithSystemFont(hintStr, "Arial", 11);
	hintLabel->setColor(Color3B(200, 200, 100));
	hintLabel->setPosition(Vec2(PW / 2, 20));
	popupBox->addChild(hintLabel);
	hintLabel->runAction(RepeatForever::create(
		Sequence::create(FadeOut::create(0.7f), FadeIn::create(0.7f), nullptr)
	));

	if (m_isFirstPlay) {
		UserDataManager::Instance()->SetPendingSubmit(m_countOfDiscus, m_mastTime);
		auto listener = EventListenerTouchOneByOne::create();
		listener->setSwallowTouches(true);
		listener->onTouchBegan = [](Touch*, Event*) {
			Director::getInstance()->replaceScene(
				TransitionFade::create(0.3f, MainScene::createScene()));
			return true;
		};
		_eventDispatcher->addEventListenerWithSceneGraphPriority(listener, overlay);
	}
}



void PlayScene::DrawTime()
{
	int elapsedTime = getMilliCount() - m_dateTime;
	RecordTime recordTime = getRecordTime(elapsedTime);
	int minutes = recordTime.min;
	int seconds = recordTime.sec;
	int ms = recordTime.ms;

	std::string strTime = StringUtils::format("%02d:%02d.%02d", minutes, seconds, ms) ;
	m_labelTime->setString(strTime);
	if (elapsedTime < 0)
	{
		MainScene* mainScene = MainScene::createScene();
		Director::getInstance()->replaceScene(TransitionFade::create(0.2, mainScene));
	}

	// 첫판(강제 3레벨) 탈출 SKIP 노출 판정 — 막힌/이탈직전 유저에게만.
	// 30초 경과(백스톱) OR 15초 무활동(멍때림) OR 이동 25수↑(헤맴) 중 하나라도.
	if (m_isFirstPlay && m_countOfDiscus == 3 && m_isIng == PLAY && !m_skipShown) {
		int now = getMilliCount();
		bool elapsed30 = (now - m_rpmStartTime) >= 30000;
		bool idle15    = (now - m_lastActivityMs) >= 15000;
		bool flailing  = (m_moveCount >= 25);
		if (elapsed30 || idle15 || flailing)
			showFirstPlaySkipButton();
	}

	// RPM 실시간 계산 및 BGM 볼륨 조정
	if (m_rpmStartTime > 0 && m_labelRPM)
	{
		float elapsedMs = std::max(2000.0f, (float)(getMilliCount() - m_rpmStartTime));
		float rawRpm = (float)m_rpmTouchCount / (elapsedMs / 60000.0f);
		rawRpm = std::min(rawRpm, 200.0f);
		// 비대칭 EMA: 상승 α=0.05(천천히), 하락 α=0.10(2배 빠름)
		float alpha = (rawRpm > m_rpmSmoothed) ? 0.05f : 0.10f;
		m_rpmSmoothed = m_rpmSmoothed * (1.f - alpha) + rawRpm * alpha;
		float rpm = m_rpmSmoothed;

		m_labelRPM->setString(StringUtils::format("RPM %d", (int)rpm));

		// RPM 구간별 컬러 그라데이션: 빨강(≤50) → 금색(75) → 녹색(≥100)
		{
			auto lerp3B = [](Color3B a, Color3B b, float t) -> Color3B {
				t = std::max(0.0f, std::min(1.0f, t));
				return Color3B(
					(uint8_t)(a.r + (b.r - a.r) * t),
					(uint8_t)(a.g + (b.g - a.g) * t),
					(uint8_t)(a.b + (b.b - a.b) * t));
			};
			const Color3B RED  (255,  60,  60);
			const Color3B GOLD (255, 215,   0);
			const Color3B GREEN( 80, 220,  80);
			Color3B rpmColor;
			if      (rpm <= 50.0f)  rpmColor = RED;
			else if (rpm >= 100.0f) rpmColor = GREEN;
			else if (rpm <= 75.0f)  rpmColor = lerp3B(RED,  GOLD,  (rpm - 50.0f) / 25.0f);
			else                    rpmColor = lerp3B(GOLD, GREEN,  (rpm - 75.0f) / 25.0f);
			m_labelRPM->setColor(rpmColor);

			// 50 이하: 깜빡임 시작 / 초과: 깜빡임 중지 (상태 변경 시에만)
			if (rpm <= 50.0f && !m_rpmBlinking) {
				m_rpmBlinking = true;
				m_labelRPM->stopAllActions();
				m_labelRPM->runAction(RepeatForever::create(
					Sequence::create(FadeOut::create(0.3f), FadeIn::create(0.3f), nullptr)));
			} else if (rpm > 50.0f && m_rpmBlinking) {
				m_rpmBlinking = false;
				m_labelRPM->stopAllActions();
				m_labelRPM->setOpacity(255);
			}
		}

		// 목표 볼륨: RPM<=50→0, RPM>=100→1, 중간→선형보간
		float targetVol = 0.0f;
		if (rpm >= 100.0f)      targetVol = 1.0f;
		else if (rpm > 50.0f)  targetVol = (rpm - 50.0f) / 50.0f;

		// 페이드 (0.02/tick ≈ 0.2/초)
		const float FADE_STEP = 0.02f;
		if (m_bgmCurrentVol < targetVol)
			m_bgmCurrentVol = std::min(targetVol, m_bgmCurrentVol + FADE_STEP);
		else if (m_bgmCurrentVol > targetVol)
			m_bgmCurrentVol = std::max(targetVol, m_bgmCurrentVol - FADE_STEP);

		if (UserDataManager::Instance()->GetSoundOpt())
		{
			float finalVol = m_bgmCurrentVol * SoundFactory::Instance()->m_mastVolume;
			CocosDenshion::SimpleAudioEngine::getInstance()->setBackgroundMusicVolume(finalVol);
		}
	}
}



void	PlayScene::ResetGame()
{

	m_moveCount = 0 ;
	m_touchHanderLayer->Reset();
	
	
	if (m_selectionMark)   m_selectionMark->setPosition(Vec2(500,500));
	if (m_deselectionMark) m_deselectionMark->setPosition(Vec2(500,500));



	for (auto& itr : m_pole)
	{
		itr.clear();		
	}
	

	for (auto& itr : m_arrDiscurs)
	{
		Discus* pDiscus = itr;
		pDiscus->SetCurrPoleID(0);
		auto& firstPoleQueue = m_pole[0];
		firstPoleQueue.push_back(pDiscus);
	}
	
}

void  PlayScene::DrawDiscus()
{
	
	int i = 0;
	for (auto& itr : m_pole)
	{
		auto& pole = itr;
		
		int j = 0;
		for (auto& it : pole)
		{
			Discus* pDiscus = it;
			Point posOfDiscus = Vec2(arrPosOfPole[i].x, arrPosOfPole[i].y + (15 * j));
			pDiscus->setPosition(posOfDiscus);
			++j;
		}
		++i;
	}

	if (true == this->CheckSuccess())
	{
		this->Finished();
	}	
}


void PlayScene::DrawInfoText ()
{
	
	std::string strLevelInfo = StringUtils::format("LV.%d / %d", m_countOfDiscus, MAX_PLAY_LEVEL);
	if (NULL == m_labelLevel)
	{
		m_labelLevel = Label::createWithSystemFont(strLevelInfo, "Arial", 13);
		m_labelLevel->setColor(Color3B::WHITE);
		m_labelLevel->setPosition(Vec2(120, RESOURCE_HEIGHT - 13));
		this->addChild(m_labelLevel, 4, tagInfoText);
		m_labelLevel->setVisible(false);
	}
	else
	{
		m_labelLevel->setString(strLevelInfo);
	}

}


void PlayScene::setTrafficDot(int i, Color4F color)
{
	if (i < 0 || i >= 3 || !m_trafficDot[i]) return;
	m_trafficDot[i]->clear();
	m_trafficDot[i]->drawDot(Vec2::ZERO, 5.5f, color);
}


void PlayScene::CountDown()
{
	this->removeChildByTag(tagCountDown, true);  // arrow/rules 이미지 제거
	if( m_isIng == NONE )
	{
		stopIdleAnimation();
		stopRankTicker();
		// 레벨·타이머 표시, 신호등 회색으로 초기화
		if (m_labelLevel) m_labelLevel->setVisible(true);
		if (m_labelTime)  m_labelTime->setVisible(true);
		for (int i = 0; i < 3; ++i) {
			if (m_trafficDot[i]) {
				setTrafficDot(i, Color4F(0.35f, 0.35f, 0.35f, 0.85f));
				m_trafficDot[i]->setVisible(true);
			}
		}
		m_isIng = COUNT_DOWN;

		DelayTime* readyDelay = DelayTime::create(2.0f);
		DelayTime* countDelay = DelayTime::create(1.0f);

		std::function<void()> func_CountDown = std::bind(&PlayScene::CountDown, this);
		CallFunc* callFunc_countDown = CallFunc::create(func_CountDown);
		Sequence* actionToContDown = Sequence::create(callFunc_countDown, countDelay, NULL);

		Repeat* countDown = Repeat::create(actionToContDown, m_countDown + 1);
		std::function<void()> func_Start = std::bind(&PlayScene::Start, this);
		CallFunc* callFunc_start = CallFunc::create(func_Start);

		// GO! 순간 Start 호출
		DelayTime* delayStart = DelayTime::create((float)m_countDown);

		this->runAction(Sequence::create(readyDelay, countDown, NULL));
		this->runAction(Sequence::create(readyDelay, delayStart, callFunc_start, NULL));
		SoundFactory::Instance()->play("efs_start_countdown");
	}
	else
	{
		if( m_countDown == 0 )
		{
			// GO! → 신호등 3개 파란색
			SoundFactory::Instance()->play("efs_go");
			for (int i = 0; i < 3; ++i)
				setTrafficDot(i, Color4F(0.0f, 0.7f, 1.0f, 1.0f));
			// 하단 패널 GO!
			for (int i = 0; i < 3; ++i) stopBottomPanel(i);
			setBottomPanel(0, "GO!",  Color3B( 80, 220, 180), 20.0f);
			setBottomPanel(1, "GO!",  Color3B( 80, 220, 180), 20.0f);
			setBottomPanel(2, "GO!",  Color3B( 80, 220, 180), 20.0f);
			blinkBottomPanel(0, 0.15f);
			blinkBottomPanel(1, 0.15f);
			blinkBottomPanel(2, 0.15f);
		}
		else
		{
			// 3→좌, 2→중, 1→우 노랑 점등
			SoundFactory::Instance()->play("efs_count_sec");
			setTrafficDot(3 - m_countDown, Color4F(1.0f, 0.84f, 0.0f, 1.0f));
			struct { int pole; Color3B color; } const info[] = {
				{ 0, Color3B(  0,   0,   0) },   // [0] unused
				{ 2, Color3B(255, 215,   0) },   // [1] "1" → Panel C, gold
				{ 1, Color3B(255, 165,   0) },   // [2] "2" → Panel B, orange
				{ 0, Color3B(255,  70,  70) },   // [3] "3" → Panel A, red
			};
			int pole = info[m_countDown].pole;
			std::string num = StringUtils::format("%d", m_countDown);
			setBottomPanel(pole, num, info[m_countDown].color, 22.0f);
			pulseBottomPanel(pole);
			--m_countDown;
		}
	}
}



Discus*	PlayScene::GetTopDiscus(int poleID)
{
	if (poleID < 0 || poleID >= m_pole.size())
	{
		return NULL;
	}
	auto& poleQueue = m_pole[poleID];
	if (poleQueue.empty())
	{
		return NULL;
	}
	return poleQueue.back();	
}
	
bool PlayScene::IsAbleToMoveDiscus(Discus* pDiscus, int poleID)
{
	Discus* pTopDiscusAtDestPole = GetTopDiscus(poleID);
	if (NULL == pTopDiscusAtDestPole )
	{
		return true;
	}

	if( pDiscus->GetDiscusID() < pTopDiscusAtDestPole->GetDiscusID() )
	{
		return false ;
	}	
	
	return true ;
}

bool PlayScene::AttachDiscusToPole(Discus* pDiscus, int poleID)
{
	if( false == IsAbleToMoveDiscus(pDiscus, poleID) )
	{
		return false; 
	}
	
	int currPoleID = pDiscus->GetCurrPoleID() ;
	if( currPoleID == poleID )
	{
		return false;
	}
	
	auto& polePrev = m_pole[currPoleID];
	if (!polePrev.empty())
	{
		polePrev.pop_back();
	}
	
	pDiscus->SetCurrPoleID(poleID);
	auto& poleNext = m_pole[poleID];
	poleNext.push_back(pDiscus);
	
	++m_moveCount ;
	
	this->DrawDiscus();
	this->DrawInfoText();
	
	return true ;
}


void PlayScene::SelectPole(int poleID, bool bIsAble)
{
	if (m_isIng == PLAY && poleID > -1) {
		++m_rpmTouchCount;
		m_lastActivityMs = getMilliCount();   // 활동 갱신 → 무활동 타이머 리셋
	}

	if( poleID > -1 && poleID < 3)
	{
		if( bIsAble )
		{
			if (m_selectionMark)   m_selectionMark->setPosition(Vec2(arrPosOfPole[poleID].x, 110));
			if (m_deselectionMark) m_deselectionMark->setPosition(Vec2(500,500));
		}
		else
		{
			if (m_deselectionMark) m_deselectionMark->setPosition(Vec2(arrPosOfPole[poleID].x, 110));
			if (m_selectionMark)   m_selectionMark->setPosition(Vec2(500,500));
		}
	}
	else
	{
		if (m_selectionMark)   m_selectionMark->setPosition(Vec2(500,500));
		if (m_deselectionMark) m_deselectionMark->setPosition(Vec2(500,500));
	}
}


bool PlayScene::CheckSuccess()
{
	int i = 0;
	for (auto& itr : m_pole)
	{
		auto& pole = itr;
		if (i < 2)
		{			
			if (pole.size() > 0)
			{
				return false;
			}
			++i;
		}		
	}

	auto& thirdPole = m_pole.back();
	int underDiscusID = -1 ;
	for (auto& itr : thirdPole)
	{
		Discus* pDiscus = itr;
		int currDiscusID = pDiscus->GetDiscusID() ;
		if ( currDiscusID < underDiscusID ) 
		{
			return false ;
		}
		
		underDiscusID = currDiscusID ;
	}	
	return true ;
	
}

void PlayScene::showFirstPlaySkipButton()
{
	if (m_skipShown) return;
	m_skipShown = true;

	// 우상단 구석에 잔잔한 텍스트 버튼(dim) — 강요 없이 "나갈 수 있다"만 알림.
	// (첫판엔 도크 메뉴가 없어 우상단이 비어 있음: 좌 LEVEL·중앙 RPM·우 TIME 옆)
	auto skipLabel = Label::createWithSystemFont("SKIP  >", "Arial", 12);
	skipLabel->setColor(Color3B(150, 160, 175));
	skipLabel->setOpacity(0);
	skipLabel->runAction(FadeIn::create(0.6f));

	auto skipItem = MenuItemLabel::create(skipLabel, [this](Ref*) {
		if (m_isTransitioning) return;
		m_isTransitioning = true;
		SoundFactory::Instance()->play("efs_click");
		// 첫판을 경험(포기 포함)했음을 영구 저장 → MainScene 재진입 시 강제 첫판 루프 방지.
		UserDefault::getInstance()->setBoolForKey("first_play_seen", true);
		UserDefault::getInstance()->flush();
#ifdef LITE_VER
		CMKStoreManager::Instance()->SetDelegate(NULL);
#endif
		Director::getInstance()->replaceScene(
			TransitionFade::create(0.3f, MainScene::createScene()));
	});

	auto menu = Menu::create(skipItem, nullptr);
	menu->setPosition(Vec2(RESOURCE_WIDTH - 36, RESOURCE_HEIGHT - 14));
	this->addChild(menu, 60);
}

void PlayScene::callbackOnPushed_homeMenuItem(Ref* pSender)
{
	if (m_isTransitioning) return;
	m_isTransitioning = true;

#ifdef LITE_VER
	CMKStoreManager::Instance()->SetDelegate(NULL);
#endif //LITE_VER

	SoundFactory::Instance()->play("efs_click");

	MainScene* mainScene = MainScene::createScene();
	auto director = Director::getInstance();
	director->replaceScene(TransitionFade::create(0.2, mainScene));

}



void PlayScene::callbackOnPushed_resetMenuItem(Ref* pSender)
{
	if (PLAY!= m_isIng ) 
	{
		return;
	}

	this->ResetGame();
	this->DrawDiscus();
	this->DrawInfoText();
	SoundFactory::Instance()->play("efs_click");
}

void PlayScene::callbackOnPushed_prevMenuItem(Ref* sender)
{
	if (m_isIng == COUNT_DOWN || m_isIng == PLAY)
	{
		return ;
	}
	if (m_isTransitioning) return;

	if( m_countOfDiscus > 3)
	{
		m_isTransitioning = true;
		SoundFactory::Instance()->play("efs_click");
		m_countOfDiscus = m_countOfDiscus-1;
		PlayScene* playScene = PlayScene::createScene(m_countOfDiscus) ;
		auto director = Director::getInstance();
		director->replaceScene(TransitionFade::create(0.2, playScene));
	}
	else
	{
		SoundFactory::Instance()->play("efs_cancel_select", 0.4);
	}
}

void PlayScene::callbackOnPushed_nextMenuItem(Ref* sender)
{
	if (m_isIng == COUNT_DOWN || m_isIng == PLAY)
	{
		return ;
	}
	if (m_isTransitioning) return;

	if( m_countOfDiscus < MAX_PLAY_LEVEL )
	{
		m_isTransitioning = true;
		SoundFactory::Instance()->play("efs_click");
		m_countOfDiscus = m_countOfDiscus+1;

		PlayScene* playScene = PlayScene::createScene(m_countOfDiscus);
		auto director = Director::getInstance();
		director->replaceScene(TransitionFade::create(0.2, playScene));
	}
	else
	{
		SoundFactory::Instance()->play("efs_cancel_select", 0.4);
	}
}


void PlayScene::callbackOnPushed_speakerMenuItem(Ref* sender)
{
	SoundFactory::Instance()->play("efs_click");
	if( false == UserDataManager::Instance()->GetSoundOpt())
	{		
		UserDataManager::Instance()->SetSoundOpt(true);
		this->DrawMenu(true);
		if (m_isIng == PLAY)
		{
			SoundFactory::Instance()->play(const_cast<char*>(m_bgmName.c_str()), true, true);
			CocosDenshion::SimpleAudioEngine::getInstance()->setBackgroundMusicVolume(
				m_bgmCurrentVol * SoundFactory::Instance()->m_mastVolume);
			startEqualizerAnimation();
		}
	}
	else
	{
		UserDataManager::Instance()->SetSoundOpt(false);
		this->DrawMenu(false);
		if (m_isIng == PLAY)
		{
			SoundFactory::Instance()->play(const_cast<char*>(m_bgmName.c_str()), false, true);
			stopEqualizerAnimation();
		}
	}
	
}



void PlayScene::callbackLockBtn(Ref* sender)
{
#ifdef LITE_VER
	if (true == isProgress) { return;  }
	isProgress = true;

	CMKStoreManager::Instance()->ToggleIndicator(true);
	CMKStoreManager::Instance()->buyFeature(kProductIdTotal);
#endif //LITE_VER

	SoundFactory::Instance()->play("efs_click");
}




#ifdef LITE_VER
void PlayScene::productFetchComplete()
{
	cocos2d::log("productFetchComplete");
	CMKStoreManager::Instance()->ToggleIndicator(false);
	isProgress = false;	
}
void PlayScene::productPurchased(std::string productId)
{
	cocos2d::log("productPurchased /%s", productId.c_str());
	CMKStoreManager::Instance()->ToggleIndicator(false);
	isProgress = false;

	if (productId == kProductIdTotal)
	{
		UserDataManager::Instance()->SetCart(true);
		UserDataManager::Instance()->SaveUserData();

		std::string strMsg = "You can play all levels.";
		Label* pPrizeMsg = Label::createWithSystemFont(strMsg, "Arial", 20);
		pPrizeMsg->setPosition(Vec2(RESOURCE_WIDTH / 2, 20));
		this->addChild(pPrizeMsg);

		auto action1 = ScaleTo::create(0.1, 1.0);
		auto action2 = Blink::create(4, 4);
		auto actionSeq = Sequence::create(action1, action2, NULL);
		pPrizeMsg->runAction(actionSeq);

		this->removeChildByTag(tagCart);		
	}
}
void PlayScene::transactionCanceled()
{
	cocos2d::log("transactionCanceled");
	CMKStoreManager::Instance()->ToggleIndicator(false);
	isProgress = false;	
}

void PlayScene::restorePreviousTransactions(int count)
{
	if (true == isRestored) { return; }

	isRestored = true;
	isProgress = false;
	if (count == 0) { return; }

	cocos2d::log("restorePreviousTransactions");

	UserDataManager::Instance()->SetCart(true);
	UserDataManager::Instance()->SaveUserData();


	CMKStoreManager::Instance()->ToggleIndicator(false);
	SoundFactory::Instance()->play("efs_click");

	Sprite* pMSGBG = Sprite::create("NewUI/text_empty.png");
	std::string strMsg = "Restored all levels you bought.";
	Label* pPrizeMsg = Label::createWithSystemFont(strMsg, "Arial", 30);
	pPrizeMsg->setPosition(Vec2(180, 130));
	pMSGBG->addChild(pPrizeMsg);
	pMSGBG->setAnchorPoint(Vec2(0.5, 0.5));
	pMSGBG->setPosition(Vec2(480 / 2, 320 / 2));
	pMSGBG->setScale(0.5);


	auto action1 = ScaleTo::create(0.1, 1.0);
	auto action2 = Blink::create(2, 2);
	auto action3 = FadeOut::create(1);
	auto actionSeq = Sequence::create(action1, action2, action3, NULL);
	pMSGBG->runAction(actionSeq);
	this->addChild(pMSGBG, tagPopup, tagPopup);

	this->removeChildByTag(tagCart);

}
#endif //LITE_VER

// ─── HUD 티커 / 결과 표시 ───────────────────────────────────────

void PlayScene::startRankTicker(int level)
{
    auto alive = m_aliveFlag;

    // 인삿말 + 플레이어명 prefix — 비동기 콜백 전에 동기적으로 구성
    std::string greeting   = getLocalGreeting();
    std::string playerName = UserDataManager::Instance()->GetUserName();
    if (playerName.empty()) playerName = "PLAYER";
    std::string prefix = greeting + "!  " + playerName + ".          ";

    LeaderboardManager::Instance()->fetchLeaderboard(level, 10,
        [this, alive, prefix, level](const std::vector<LeaderboardEntry>& entries) {
            if (!alive || !*alive) return;
            if (m_isIng != NONE) return;

            // 상단 티커
            std::string rankText;
            if (entries.empty()) {
                rankText = "---  BE THE FIRST TO RANK!  ---";
            } else {
                for (const auto& e : entries) {
                    if (!rankText.empty()) rankText += "   |   ";
                    std::string cc = e.countryCode.empty() ? "--" : e.countryCode;
                    for (char& c : cc) c = (char)toupper((unsigned char)c);
                    rankText += StringUtils::format("%d.%s%s", e.rank, countryToFlag(cc).c_str(), e.displayName.c_str());
                }
            }

            if (m_hudTickerLabel) {
                m_hudTickerLabel->setString(prefix + rankText);
                m_hudTickerLabel->setVisible(true);
                tickerScrollStep();
            }

            // 하단 전광판 — Level 4 이상: 올림픽 시상대
            if (level >= 4) {
                stopIdleAnimation();
                showPodiumRanking(entries);
            }
        });
}

void PlayScene::tickerScrollStep()
{
    if (!m_hudTickerLabel || !m_hudTickerLabel->getParent()) return;
    if (m_isIng != NONE) return;

    const float SPEED = 80.0f;
    float labelW   = m_hudTickerLabel->getContentSize().width;
    float startX   = RESOURCE_WIDTH + 5.0f;
    float endX     = -(labelW + 5.0f);
    float duration = (startX - endX) / SPEED;

    m_hudTickerLabel->setAnchorPoint(Vec2(0, 0.5f));
    m_hudTickerLabel->setPositionX(startX);
    m_hudTickerLabel->stopAllActions();
    m_hudTickerLabel->runAction(Sequence::create(
        MoveTo::create(duration, Vec2(endX, RESOURCE_HEIGHT - 13.0f)),
        CallFunc::create(CC_CALLBACK_0(PlayScene::tickerScrollStep, this)),
        nullptr
    ));
}

void PlayScene::stopRankTicker()
{
    if (m_hudTickerLabel) {
        m_hudTickerLabel->stopAllActions();
        m_hudTickerLabel->setVisible(false);
    }
}

void PlayScene::setBottomPanel(int pole, const std::string& text, Color3B color, float fontSize)
{
    Label* const labels[] = { m_bottomLabelA, m_bottomLabelB, m_bottomLabelC };
    if (pole < 0 || pole > 2 || !labels[pole]) return;
    Label* lbl = labels[pole];
    lbl->stopAllActions();
    lbl->setVisible(true);
    lbl->setOpacity(255);
    lbl->setScale(1.0f);
    lbl->setPosition(Vec2(arrPosOfPole[pole].x, BOTTOM_PANEL_Y));
    lbl->setSystemFontSize(fontSize);
    lbl->setString(text);
    lbl->setColor(color);
}

void PlayScene::blinkBottomPanel(int pole, float interval)
{
    Label* const labels[] = { m_bottomLabelA, m_bottomLabelB, m_bottomLabelC };
    if (pole < 0 || pole > 2 || !labels[pole]) return;
    Label* lbl = labels[pole];
    lbl->stopAllActions();
    lbl->setVisible(true);
    lbl->runAction(RepeatForever::create(
        Sequence::create(
            FadeOut::create(interval),
            FadeIn::create(interval),
            nullptr)));
}

void PlayScene::stopBottomPanel(int pole)
{
    Label* const labels[] = { m_bottomLabelA, m_bottomLabelB, m_bottomLabelC };
    if (pole < 0 || pole > 2 || !labels[pole]) return;
    Label* lbl = labels[pole];
    lbl->stopAllActions();
    lbl->setVisible(true);
    lbl->setOpacity(255);
    lbl->setScale(1.0f);
    lbl->setSystemFontSize(BOTTOM_FONT_DEFAULT);
    lbl->setPosition(Vec2(arrPosOfPole[pole].x, BOTTOM_PANEL_Y));
}

void PlayScene::clearBottomPanels()
{
    for (int i = 0; i < 3; ++i)
    {
        stopBottomPanel(i);
        setBottomPanel(i, "");
    }
    // 시상대 동적 레이블 제거
    for (int t = TAG_PODIUM_BASE; t < TAG_PODIUM_BASE + 6; ++t)
        this->removeChildByTag(t, true);
}

// 한 글자씩 타이핑되듯 등장 (UTF-8 멀티바이트/이모지 지원)
void PlayScene::typingBottomPanel(int pole, const std::string& text, float charInterval, cocos2d::Color3B color, float fontSize)
{
    Label* const labels[] = { m_bottomLabelA, m_bottomLabelB, m_bottomLabelC };
    if (pole < 0 || pole > 2 || !labels[pole]) return;
    Label* lbl = labels[pole];
    stopBottomPanel(pole);
    lbl->setSystemFontSize(fontSize);
    lbl->setColor(color);
    lbl->setString("");

    std::vector<size_t> ends;
    for (size_t i = 0; i < text.size(); ) {
        unsigned char c = (unsigned char)text[i];
        size_t n = (c < 0x80) ? 1 : (c < 0xE0) ? 2 : (c < 0xF0) ? 3 : 4;
        i += n;
        ends.push_back(i);
    }

    Vector<FiniteTimeAction*> acts;
    for (size_t idx = 0; idx < ends.size(); ++idx) {
        std::string partial = text.substr(0, ends[idx]);
        acts.pushBack(DelayTime::create(charInterval));
        acts.pushBack(CallFunc::create([lbl, partial]{ lbl->setString(partial); }));
    }
    lbl->runAction(Sequence::create(acts));
}

// 좌/우에서 미끄러져 들어오기
void PlayScene::slideInBottomPanel(int pole, const std::string& text, bool fromLeft, cocos2d::Color3B color, float fontSize)
{
    Label* const labels[] = { m_bottomLabelA, m_bottomLabelB, m_bottomLabelC };
    if (pole < 0 || pole > 2 || !labels[pole]) return;
    Label* lbl = labels[pole];
    stopBottomPanel(pole);
    lbl->setSystemFontSize(fontSize);
    lbl->setColor(color);
    lbl->setString(text);

    Vec2 rest(arrPosOfPole[pole].x, BOTTOM_PANEL_Y);
    lbl->setPosition(rest + Vec2(fromLeft ? -130.0f : 130.0f, 0));
    lbl->runAction(EaseOut::create(MoveTo::create(0.35f, rest), 2.5f));
}

// 크기가 쿵쿵 맥박치듯 강조
void PlayScene::pulseBottomPanel(int pole)
{
    Label* const labels[] = { m_bottomLabelA, m_bottomLabelB, m_bottomLabelC };
    if (pole < 0 || pole > 2 || !labels[pole]) return;
    Label* lbl = labels[pole];
    lbl->stopAllActions();
    lbl->setScale(1.0f);
    lbl->runAction(RepeatForever::create(
        Sequence::create(
            ScaleTo::create(0.2f, 1.35f),
            ScaleTo::create(0.2f, 1.0f),
            DelayTime::create(0.7f),
            nullptr)));
}

// 색상이 무지개처럼 순환
void PlayScene::colorCycleBottomPanel(int pole)
{
    Label* const labels[] = { m_bottomLabelA, m_bottomLabelB, m_bottomLabelC };
    if (pole < 0 || pole > 2 || !labels[pole]) return;
    Label* lbl = labels[pole];
    lbl->stopAllActions();
    lbl->runAction(RepeatForever::create(
        Sequence::create(
            TintTo::create(0.35f, 255,  80,  80),
            TintTo::create(0.35f, 255, 200,  50),
            TintTo::create(0.35f,  80, 220, 180),
            TintTo::create(0.35f, 100, 160, 255),
            TintTo::create(0.35f, 200,  80, 255),
            nullptr)));
}

// 좌우로 짧게 흔들기 (경고/실수 연출)
void PlayScene::shakeBottomPanel(int pole)
{
    Label* const labels[] = { m_bottomLabelA, m_bottomLabelB, m_bottomLabelC };
    if (pole < 0 || pole > 2 || !labels[pole]) return;
    Label* lbl = labels[pole];
    lbl->stopAllActions();
    lbl->runAction(RepeatForever::create(
        Sequence::create(
            MoveBy::create(0.04f, Vec2( 4, 0)),
            MoveBy::create(0.04f, Vec2(-8, 0)),
            MoveBy::create(0.04f, Vec2( 8, 0)),
            MoveBy::create(0.04f, Vec2(-4, 0)),
            DelayTime::create(1.5f),
            nullptr)));
}

// 세 패널에 서로 다른 효과를 동시에 실행하는 데모
void PlayScene::demoBottomPanels()
{
    auto alive = m_aliveFlag;

    // 패널A: 타이핑 등장 → 완료 후 pulse
    typingBottomPanel(0, "🎯 HANOI", 0.09f, Color3B(80, 220, 180));
    float typingDur = 8 * 0.09f + 0.1f;   // 글자수 * interval
    this->runAction(Sequence::create(
        DelayTime::create(typingDur),
        CallFunc::create([this, alive]{ if (*alive) pulseBottomPanel(0); }),
        nullptr));

    // 패널B: 왼쪽에서 슬라이드 인 → 완료 후 blink
    slideInBottomPanel(1, "⚡ READY?", true, Color3B(255, 210, 50));
    this->runAction(Sequence::create(
        DelayTime::create(0.5f),
        CallFunc::create([this, alive]{ if (*alive) blinkBottomPanel(1, 0.45f); }),
        nullptr));

    // 패널C: 색상 사이클 (shake와 colorCycle은 stopAllActions 충돌로 동시 불가 — 별도 사용)
    setBottomPanel(2, "🏆 PLAY!", Color3B(255, 80, 80));
    colorCycleBottomPanel(2);
}

// ── 대기 화면 랜덤 연출 ──────────────────────────────────────────
void PlayScene::_showNextIdleScene()
{
    static int last = -1;
    const int N = 7;
    int pick;
    do { pick = rand() % N; } while (pick == last);
    last = pick;

    std::string lv = StringUtils::format("LV.%d", m_countOfDiscus);

    switch (pick)
    {
    case 0:
        // 소형(10) — 긴 메시지, 타이핑/슬라이드/깜빡임
        typingBottomPanel (0, "🎯 " + lv,        0.08f, Color3B( 80, 220, 180), 10.0f);
        slideInBottomPanel(1, "⚡ CHALLENGE",     true,  Color3B(255, 210,  50), 10.0f);
        setBottomPanel    (2, "🏆 READY?",               Color3B(255, 120,  80), 10.0f);
        blinkBottomPanel  (2, 0.5f);
        break;
    case 1:
        // 중형(16) — 이모지+단어 조합
        slideInBottomPanel(0, "🔥 EXPERT",        false, Color3B(255,  80,  80), 16.0f);
        setBottomPanel    (1, "⭐ " + lv,                Color3B(180, 180, 255), 16.0f);
        colorCycleBottomPanel(1);
        typingBottomPanel (2, "🎮 TAP!",          0.1f,  Color3B( 80, 220, 180), 16.0f);
        break;
    case 2:
        // 대형(24) — 짧은 단어/이모지, pulse·슬라이드
        setBottomPanel    (0, "🌟 OLYMPIC",              Color3B(255, 215,   0), 24.0f);
        pulseBottomPanel  (0);
        typingBottomPanel (1, "GO!",              0.12f, Color3B( 80, 220, 180), 24.0f);
        slideInBottomPanel(2, "💪",               true,  Color3B(255, 120,  80), 24.0f);
        break;
    case 3:
        // 중형(15) — TOWER · OF · HANOI 분할
        setBottomPanel    (0, "🎮 TOWER",               Color3B(100, 200, 255), 15.0f);
        colorCycleBottomPanel(0);
        slideInBottomPanel(1, "⚡ OF",            false, Color3B(255, 210,  50), 15.0f);
        slideInBottomPanel(2, "🏰 HANOI",         true,  Color3B(255, 120,  80), 15.0f);
        break;
    case 4:
        // 혼합 — 소형 설명 + 대형 레벨번호
        typingBottomPanel (0, "⏱ BEST TIME?",   0.07f, Color3B(255, 210,  50), 10.0f);
        setBottomPanel    (1, "⭐ CHALLENGE",            Color3B( 80, 220, 180), 10.0f);
        blinkBottomPanel  (1, 0.4f);
        setBottomPanel    (2, lv,                        Color3B(255, 215,   0), 28.0f);
        pulseBottomPanel  (2);
        break;
    case 5:
        // 초대형(30) — 이모지 하나씩, 패널을 꽉 채움
        setBottomPanel    (0, "⚡",                      Color3B(255, 210,  50), 30.0f);
        shakeBottomPanel  (0);
        slideInBottomPanel(1, "START",            true,  Color3B( 80, 220, 180), 28.0f);
        setBottomPanel    (2, "🏆",                      Color3B(255, 215,   0), 30.0f);
        colorCycleBottomPanel(2);
        break;
    case 6:
        // 초대형(32) — 레벨번호 + 이모지 스폿라이트
        setBottomPanel    (0, "🎯",                      Color3B( 80, 220, 180), 32.0f);
        pulseBottomPanel  (0);
        setBottomPanel    (1, lv,                        Color3B(255, 255, 255), 26.0f);
        colorCycleBottomPanel(1);
        slideInBottomPanel(2, "🔥",               false, Color3B(255,  80,  80), 32.0f);
        break;
    }
}

void PlayScene::startIdleAnimation()
{
    clearBottomPanels();
    _showNextIdleScene();
    schedule([this](float){ _showNextIdleScene(); }, 3.5f, "idle_anim");
}

void PlayScene::stopIdleAnimation()
{
    unschedule("idle_anim");
    unschedule("podium_to_idle");
    unschedule("marquee_cycle");
    unschedule("marquee_next");
    unschedule("cheer_anim");
    unschedule("cheer_start");
    unschedule("eq_update");
    this->stopActionByTag(TAG_GUIDE_ANIM);
    clearBottomPanels();
}

// ── 플레이 중 응원 연출 ──────────────────────────────────────────────
void PlayScene::startCheerAnimation()
{
    clearBottomPanels();
    _showNextCheerScene();
    schedule([this](float){ _showNextCheerScene(); }, 3.5f, "cheer_anim");
}

void PlayScene::_showNextCheerScene()
{
    static const char* msgs[] = {
        "GO! GO! 🔥",
        "NICE! ⭐",
        "YOU GOT IT!",
        "KEEP GOING!",
        "AMAZING! 🔥",
        "BRILLIANT!",
        "DON'T STOP!",
        "FOCUS! 🎯",
        "NICE MOVE!",
        "UNSTOPPABLE!",
        "SO CLOSE! 🏆",
        "LEGEND! ⭐",
        "GO FOR IT!",
        "CRUSHING IT!",
        "GENIUS! 🔥",
    };
    static const Color3B cols[] = {
        Color3B(255, 100,  60),   // 오렌지레드
        Color3B(255, 215,   0),   // 골드
        Color3B( 80, 220, 180),   // 청록
        Color3B(255,  80, 180),   // 핫핑크
        Color3B(100, 200, 255),   // 스카이블루
    };
    static const int NMSG = 15, NCOL = 5;

    for (int i = 0; i < 3; ++i) {
        int m = rand() % NMSG;
        int c = rand() % NCOL;
        float fs = 9.5f + (float)(rand() % 3);  // 9.5~11.5px
        switch (rand() % 4) {
            case 0: typingBottomPanel(i, msgs[m], 0.07f, cols[c], fs);                      break;
            case 1: setBottomPanel(i, msgs[m], cols[c], fs); blinkBottomPanel(i, 0.4f);     break;
            case 2: slideInBottomPanel(i, msgs[m], i % 2 == 0, cols[c], fs);                break;
            case 3: setBottomPanel(i, msgs[m], cols[c], fs + 1); pulseBottomPanel(i);       break;
        }
    }
}

// ── Level 4+ 올림픽 시상대: 하단 전광판 A=2위(은) B=1위(금) C=3위(동) ─
// 올림픽 시상대: A=2위(은) B=1위(금) C=3위(동)
// 깃발: 1위만 높게, 2·3위 동일 높이, 위에서 슬라이드다운
// 이름: 3패널 동일 Y 고정 표시, 순위번호 없음
void PlayScene::showPodiumRanking(const std::vector<LeaderboardEntry>& entries)
{
    clearBottomPanels();
    if (entries.empty()) {
        // 랭커 없음 → 마르퀴↔패널 순차 연출 (병렬 없음)
        m_noRankPhase = 0;
        _noRankNextScene(m_countOfDiscus);
        return;
    }

    struct Slot {
        int     pole;
        int     entryIdx;
        Color3B color;
        float   flagY;   // 깃발 최종 Y
    };

    // 깃발 Y: 1위만 높게, 2·3위 동일
    const float FLAG_Y_GOLD   = BOTTOM_PANEL_Y + 10.0f;
    const float FLAG_Y_SILVER = BOTTOM_PANEL_Y - 1.0f;
    const float FLAG_Y_BRONZE = BOTTOM_PANEL_Y - 1.0f;  // 2위와 동일
    const float NAME_Y        = BOTTOM_PANEL_Y - 14.0f; // 전 패널 동일

    const Slot slots[] = {
        { 0, 1, Color3B(192, 192, 192), FLAG_Y_SILVER },  // 패널A = 2위(은)
        { 1, 0, Color3B(255, 215,   0), FLAG_Y_GOLD   },  // 패널B = 1위(금)
        { 2, 2, Color3B(205, 127,  50), FLAG_Y_BRONZE },  // 패널C = 3위(동)
    };

    Label* const mainLabels[] = { m_bottomLabelA, m_bottomLabelB, m_bottomLabelC };

    for (const auto& s : slots) {
        if (s.entryIdx >= (int)entries.size()) {
            // 해당 순위 랭커 없음 → 패널에 랜덤 예제 연출
            const char* texts[]  = { "🏆 PLAY!", "⭐ RANK IT", "🔥 BE #1!", "🎮 JOIN!" };
            const Color3B cols[] = {
                Color3B( 80, 220, 180), Color3B(255, 210,  50),
                Color3B(255, 100,  80), Color3B(100, 200, 255)
            };
            int r = rand() % 4;
            switch (r) {
                case 0: typingBottomPanel(s.pole, texts[r], 0.1f, cols[r], 11.0f);                          break;
                case 1: setBottomPanel(s.pole, texts[r], cols[r], 11.0f); blinkBottomPanel(s.pole, 0.5f);   break;
                case 2: slideInBottomPanel(s.pole, texts[r], s.pole % 2 == 0, cols[r], 11.0f);              break;
                case 3: setBottomPanel(s.pole, texts[r], cols[r], 13.0f); pulseBottomPanel(s.pole);         break;
            }
            continue;
        }
        const auto& e = entries[s.entryIdx];

        std::string cc   = e.countryCode;
        for (char& c : cc) c = (char)toupper((unsigned char)c);
        std::string flag = cc.empty() ? "  " : countryToFlag(cc);
        std::string name = e.displayName.empty() ? "???" : e.displayName;

        float cx = arrPosOfPole[s.pole].x;

        // ── 깃발: 위에서 슬라이드다운 + FadeIn ──────────────────
        const float flagStartY = s.flagY - 50.0f;  // 50px 아래에서 상승 시작

        Label* flagLbl = Label::createWithSystemFont(flag, "Arial", 22.0f);
        flagLbl->setColor(Color3B::WHITE);  // 이모지 원본색 유지
        flagLbl->setAnchorPoint(Vec2(0.5f, 0.5f));
        flagLbl->setHorizontalAlignment(TextHAlignment::CENTER);
        flagLbl->setOpacity(0);
        flagLbl->setPosition(Vec2(cx, flagStartY));
        flagLbl->setTag(TAG_PODIUM_BASE + s.pole * 2);
        this->addChild(flagLbl, 2);  // LED 도트(z=3) 아래에 렌더링

        // A→B→C 순으로 0.4s 시차
        float delay = s.pole * 0.4f;
        flagLbl->runAction(Sequence::create(
            DelayTime::create(delay),
            Spawn::create(
                EaseOut::create(MoveTo::create(2.8f, Vec2(cx, s.flagY)), 3.0f),
                FadeIn::create(1.4f),
                nullptr),
            nullptr));

        // ── 이름: 전 패널 동일 Y, 애니 없음 ─────────────────────
        Label* nameLbl = mainLabels[s.pole];
        nameLbl->stopAllActions();
        nameLbl->setVisible(true);
        nameLbl->setOpacity(255);
        nameLbl->setScale(1.0f);
        nameLbl->setSystemFontSize(11.0f);
        nameLbl->setAnchorPoint(Vec2(0.5f, 0.5f));
        nameLbl->setHorizontalAlignment(TextHAlignment::CENTER);
        nameLbl->setPosition(Vec2(cx, NAME_Y));
        nameLbl->setColor(s.color);
        nameLbl->setString(name);
    }

    // 5~8초 시상대 감상 후 idle 애니로 전환
    auto alive = m_aliveFlag;
    float podiumSec = 5.0f + static_cast<float>(rand() % 4);
    this->scheduleOnce([this, alive](float) {
        if (!alive || !*alive) return;
        if (m_isIng != NONE) return;
        startIdleAnimation();
    }, podiumSec, "podium_to_idle");
}

// ── 랭커 없는 레벨: 마르퀴 ↔ 패널 개별 연출 순차 전환 ──────────────
void PlayScene::_noRankNextScene(int lv)
{
    if (m_isIng != NONE) return;

    this->removeChildByTag(TAG_PODIUM_BASE, true);
    for (int i = 0; i < 3; ++i) stopBottomPanel(i);

    auto alive = m_aliveFlag;

    if (m_noRankPhase % 2 == 0) {
        // ── 짝수: 대형 마르퀴 슬라이딩 ────────────────────────────
        struct M { const char* fmt; Color3B color; };
        const M msgs[3] = {
            { "🔥  LEVEL %d : NO RECORD  ·  BE THE 1ST!  🏆  ·  ", Color3B(255, 215,   0) },
            { "⭐  YOUR NAME ON TOP  ·  DARE TO WIN?  🔥  THRONE AWAITS  ·  ", Color3B( 80, 220, 180) },
            { "🏆  NO CHAMPION YET  ·  WILL IT BE YOU?  ·  MAKE HISTORY!  🔥  ·  ", Color3B(255, 120,  60) },
        };
        int idx = (m_noRankPhase / 2) % 3;
        std::string txt = (idx == 0)
            ? StringUtils::format(msgs[0].fmt, lv)
            : std::string(msgs[idx].fmt);

        const float startX = 660.0f, endX = -380.0f, slideSec = 9.0f;

        Label* lbl = Label::createWithSystemFont(txt, "Arial", 22.0f);
        lbl->setColor(msgs[idx].color);
        lbl->setAnchorPoint(Vec2(0.5f, 0.5f));
        lbl->setOpacity(0);
        lbl->setPosition(Vec2(startX, BOTTOM_PANEL_Y));
        lbl->setTag(TAG_PODIUM_BASE);
        this->addChild(lbl, 2);  // LED 도트(z=3) 아래

        lbl->runAction(Sequence::create(
            FadeIn::create(0.5f),
            MoveTo::create(slideSec, Vec2(endX, BOTTOM_PANEL_Y)),
            FadeOut::create(0.3f),
            nullptr));

        ++m_noRankPhase;
        this->scheduleOnce([this, lv, alive](float) {
            if (!alive || !*alive) return;
            _noRankNextScene(lv);
        }, slideSec + 0.5f, "marquee_next");

    } else {
        // ── 홀수: 각 패널 개별 랜덤 연출 ──────────────────────────
        const char* texts[] = { "🏆 PLAY!", "⭐ RANK IT", "🔥 BE #1!", "🎮 JOIN!" };
        const Color3B cols[] = {
            Color3B( 80, 220, 180), Color3B(255, 210,  50),
            Color3B(255, 100,  80), Color3B(100, 200, 255)
        };
        for (int i = 0; i < 3; ++i) {
            int r = rand() % 4;
            switch (r) {
                case 0: typingBottomPanel(i, texts[r], 0.1f, cols[r], 11.0f);                       break;
                case 1: setBottomPanel(i, texts[r], cols[r], 11.0f); blinkBottomPanel(i, 0.5f);     break;
                case 2: slideInBottomPanel(i, texts[r], i % 2 == 0, cols[r], 11.0f);                break;
                case 3: setBottomPanel(i, texts[r], cols[r], 13.0f); pulseBottomPanel(i);           break;
            }
        }
        ++m_noRankPhase;
        this->scheduleOnce([this, lv, alive](float) {
            if (!alive || !*alive) return;
            _noRankNextScene(lv);
        }, 4.5f, "marquee_next");
    }
}

// ── Level 3 전용 가이드: A→B→C 순서로 페이징 ───────────────────────
// 정보1(청록) → 정보2(황금) → 정보3(주황) 순환
// 같은 페이지 안에서는 동일 색상, 페이지 전환 시 색상 변경
void PlayScene::startGuideAnimation()
{
    clearBottomPanels();
    auto alive = m_aliveFlag;

    const float T_IN   = 1.2f;   // 패널 하나씩 등장하는 간격
    const float T_HOLD = 3.5f;   // 세 패널 모두 보이는 유지 시간
    const float T_GAP  = 0.6f;   // 페이지 사이 암전 간격

    // 페이지별 색상 — 페이징 느낌이 확실히 나도록 뚜렷하게 구분
    const Color3B c1( 80, 220, 180);   // 정보1: 청록  (디스크 선택)
    const Color3B c2(255, 210,  50);   // 정보2: 황금  (디스크 이동)
    const Color3B c3(255, 100,  80);   // 정보3: 붉은주황 (디스크 배치)

    auto seq = Sequence::create(

        // ══ 정보1: 전광판을 터치해서 디스크를 선택하세요 ══
        CallFunc::create([this, alive, c1]{
            if (!*alive) return;
            clearBottomPanels();
            setBottomPanel(0, "👆 TOUCH HERE", c1, 12.0f);
            blinkBottomPanel(0, 0.7f);              // A 깜빡임 — "이곳을 눌러보세요"
        }),
        DelayTime::create(T_IN),
        CallFunc::create([this, alive, c1]{
            if (!*alive) return;
            setBottomPanel(1, "TO SELECT",     c1, 12.0f);
        }),
        DelayTime::create(T_IN),
        CallFunc::create([this, alive, c1]{
            if (!*alive) return;
            setBottomPanel(2, "A DISC",        c1, 12.0f);
        }),
        DelayTime::create(T_HOLD),

        // ══ 정보2: 선택한 디스크를 옆 폴로 이동하세요 ══
        CallFunc::create([this, alive, c2]{
            if (!*alive) return;
            clearBottomPanels();
            setBottomPanel(0, "DISC SELECTED!", c2, 11.0f);
        }),
        DelayTime::create(T_IN),
        CallFunc::create([this, alive, c2]{
            if (!*alive) return;
            setBottomPanel(1, "MOVE ➡",        c2, 13.0f);
        }),
        DelayTime::create(T_IN),
        CallFunc::create([this, alive, c2]{
            if (!*alive) return;
            setBottomPanel(2, "THIS POLE",     c2, 11.0f);
        }),
        DelayTime::create(T_HOLD),

        // ══ 정보3: 전광판을 터치해서 디스크를 내려놓으세요 ══
        CallFunc::create([this, alive, c3]{
            if (!*alive) return;
            clearBottomPanels();
            setBottomPanel(0, "PLACE DISC",    c3, 12.0f);
        }),
        DelayTime::create(T_IN),
        CallFunc::create([this, alive, c3]{
            if (!*alive) return;
            setBottomPanel(1, "TAP BELOW",     c3, 12.0f);
        }),
        DelayTime::create(T_IN),
        CallFunc::create([this, alive, c3]{
            if (!*alive) return;
            setBottomPanel(2, "👇 HERE!",      c3, 12.0f);
            blinkBottomPanel(2, 0.7f);              // C 깜빡임 — "이곳을 눌러보세요"
        }),
        DelayTime::create(T_HOLD),

        // 페이지 암전 후 다음 루프
        CallFunc::create([this, alive]{ if (*alive) clearBottomPanels(); }),
        DelayTime::create(T_GAP),
        nullptr
    );

    auto repeatAction = RepeatForever::create(seq);
    repeatAction->setTag(TAG_GUIDE_ANIM);
    this->runAction(repeatAction);
}

// ── 이퀄라이저 ──────────────────────────────────────────────────────

void PlayScene::startEqualizerAnimation()
{
    clearBottomPanels();
    memset(m_eqH,    0, sizeof(m_eqH));
    memset(m_eqPeak, 0, sizeof(m_eqPeak));

    if (!m_eqNode) {
        m_eqNode = DrawNode::create();
        m_eqNode->setPosition(Vec2::ZERO);
        this->addChild(m_eqNode, 4);  // LED dot overlay(z=3) 위
    }
    schedule([this](float dt){ _updateEqualizer(dt); }, 0.05f, "eq_update");
}

void PlayScene::stopEqualizerAnimation()
{
    unschedule("eq_update");
    if (m_eqNode) {
        m_eqNode->removeFromParent();
        m_eqNode = nullptr;
    }
    memset(m_eqH,    0, sizeof(m_eqH));
    memset(m_eqPeak, 0, sizeof(m_eqPeak));
}

void PlayScene::_updateEqualizer(float dt)
{
    if (!m_eqNode || !m_eqNode->getParent()) return;
    m_eqNode->clear();

    static const int   BARS     = 6;
    static const float BAR_BOT  = 5.0f;   // 패널 하단 여백
    static const float BAR_MAXH = 42.0f;  // 최대 막대 높이
    static const float DECAY    = 95.0f;  // 막대 낙하 속도 px/s
    static const float PK_DECAY = 28.0f;  // peak 낙하 속도 px/s

    const float DIV1 = (arrPosOfPole[0].x + arrPosOfPole[1].x) * 0.5f;
    const float DIV2 = (arrPosOfPole[1].x + arrPosOfPole[2].x) * 0.5f;
    const float sectX[3] = { 2.0f, DIV1 + 2.0f, DIV2 + 2.0f };
    const float sectW[3] = { DIV1 - 4.0f, DIV2 - DIV1 - 4.0f, RESOURCE_WIDTH - DIV2 - 4.0f };

    float maxH = BAR_MAXH * m_bgmCurrentVol;

    for (int s = 0; s < 3; ++s) {
        float barW = sectW[s] / (BARS * 2 - 1);  // bar 폭 = gap 폭
        float x0   = sectX[s];

        for (int i = 0; i < BARS; ++i) {
            // 낙하
            m_eqH[s][i] = std::max(0.0f, m_eqH[s][i] - DECAY * dt);

            // 볼륨 비례 랜덤 kick (50% 확률)
            if (rand() % 2 == 0) {
                float kick = ((float)(rand() % 1000) / 1000.0f) * maxH;
                if (kick > m_eqH[s][i]) m_eqH[s][i] = kick;
            }

            // peak: 막대가 올라가면 갱신, 아니면 천천히 낙하
            if (m_eqH[s][i] >= m_eqPeak[s][i])
                m_eqPeak[s][i] = m_eqH[s][i];
            else
                m_eqPeak[s][i] = std::max(0.0f, m_eqPeak[s][i] - PK_DECAY * dt);

            float h  = m_eqH[s][i];
            float pk = m_eqPeak[s][i];
            float x  = x0 + i * barW * 2.0f;
            float w  = barW * 0.82f;

            // 막대 색: 18개 전체를 하나의 레드→퍼플 연속 그라데이션
            if (h > 0.5f) {
                static const Color4F STOPS[6] = {
                    Color4F(1.00f, 0.22f, 0.22f, 1.0f),  // 레드
                    Color4F(1.00f, 0.55f, 0.05f, 1.0f),  // 오렌지
                    Color4F(1.00f, 0.92f, 0.05f, 1.0f),  // 옐로우
                    Color4F(0.15f, 1.00f, 0.40f, 1.0f),  // 그린
                    Color4F(0.10f, 0.82f, 1.00f, 1.0f),  // 시안
                    Color4F(0.78f, 0.18f, 1.00f, 1.0f),  // 퍼플
                };
                // 전체 18막대 기준 위치(0→1)로 인접 stop 보간
                float gt  = (float)(s * BARS + i) / (3 * BARS - 1);
                float pos = gt * 5.0f;
                int   ci  = std::min((int)pos, 4);
                float ct  = pos - (float)ci;
                Color4F n(
                    STOPS[ci].r + (STOPS[ci+1].r - STOPS[ci].r) * ct,
                    STOPS[ci].g + (STOPS[ci+1].g - STOPS[ci].g) * ct,
                    STOPS[ci].b + (STOPS[ci+1].b - STOPS[ci].b) * ct,
                    1.0f);
                float t   = h / BAR_MAXH;
                float dim = 0.22f + t * 0.78f;
                float a   = 0.45f + t * 0.55f;
                m_eqNode->drawSolidRect(Vec2(x, BAR_BOT), Vec2(x + w, BAR_BOT + h),
                    Color4F(n.r * dim, n.g * dim, n.b * dim, a));
            }

            // peak 흰 선 (색 대비 극대화)
            if (pk > 2.0f) {
                float py = BAR_BOT + pk;
                m_eqNode->drawSolidRect(
                    Vec2(x, py - 1.0f), Vec2(x + w, py + 1.0f),
                    Color4F(1.0f, 1.0f, 1.0f, 0.95f));
            }
        }
    }
}

// ────────────────────────────────────────────────────────────────────

void PlayScene::showHudResult(bool isNewRecord, const RecordTime& rt)
{
    if (!m_hudTickerLabel) return;

    m_hudTickerLabel->stopAllActions();
    m_hudTickerLabel->setOpacity(255);
    m_hudTickerLabel->setVisible(true);

    const char* praise = isNewRecord ? "Perfect!" : "Great!";
    Color3B color      = isNewRecord ? Color3B(255, 215, 80) : Color3B(80, 220, 180);
    const char* result = isNewRecord ? "NEW RECORD!" : "LEVEL CLEAR";
    std::string text = StringUtils::format("%s  %02d:%02d.%02d  %s",
        praise, rt.min, rt.sec, rt.ms, result);

    m_hudTickerLabel->setString(text);
    m_hudTickerLabel->setColor(color);
    m_hudTickerLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
    m_hudTickerLabel->setPosition(Vec2(RESOURCE_WIDTH * 0.65f, RESOURCE_HEIGHT - 13.0f));

    // 1초 간격 깜빡임
    m_hudTickerLabel->runAction(RepeatForever::create(Sequence::create(
        DelayTime::create(1.0f), Hide::create(),
        DelayTime::create(1.0f), Show::create(),
        nullptr
    )));
}
