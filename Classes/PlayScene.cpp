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

// 리플레이 직렬화 헬퍼 (정의는 파일 하단 리플레이 섹션) — MessagePopup 등에서 먼저 참조
static std::string encodeReplayBlob(const std::vector<ReplayEvent>& ev, int discus, int finalMs);
static std::string replayKey(int level);

// 120초 무입력 자동 포기 임계값 (DrawTime·인코딩 clamp 양쪽에서 사용 → 상단 정의)
static const uint32_t REPLAY_MAX_DELTA_MS = 120000;

// 리플레이 크기 안전장치 (C-6): 이 Base64 길이 초과 리플레이는 저장/업로드 안 함(서버 writeReplay와 동일값).
static const int REPLAY_BLOB_MAX_CHARS = 10000;
// A-1 잡음 이벤트 디바운스 임계(ms) — 이보다 빠른 FAIL/DESELECT는 녹화 스킵(봇 스팸 억제).
static const int REPLAY_JUNK_DEBOUNCE_MS = 40;

// 리플레이 배속 단계 (기본 x1 = 인덱스 1). 마지막 선택은 UserDefault "replay_speed"에 글로벌 저장.
static const float kReplaySpeeds[]  = { 0.5f, 1.0f, 1.5f, 2.0f, 4.0f };
static const int   kReplaySpeedCount = 5;

// 레이스 마커 스프링(가속·감속). K=강성(클수록 강한 가속), D=감쇠.
// 감쇠비 ζ = D/(2√K): ζ<1이면 목표를 살짝 지나쳤다 돌아옴(역동적). 여기선 ζ≈0.71 → 부드럽되 탄력.
// 더 튕기게 하려면 D를 줄이고, 더 빠릿하게 하려면 K를 키운다. RACE_SPRING_VMAX는 폭주 방지 상한(px/s).
static const float RACE_SPRING_K    = 300.0f;
static const float RACE_SPRING_D    = 24.5f;
static const float RACE_SPRING_VMAX = 900.0f;

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

	// 리플레이 녹화 초기화 — m_dateTime 기준으로 이벤트 t_ms 기록 (docs §6.1)
	m_replay.clear();
	m_isReplaying        = false;
	m_replaySelectedPole = -1;
	m_replayOverflow     = false;
	m_lastReplayEventMs  = 0;
	m_idleAbandoned      = false;

	// 3차: 고스트 테이블 + HUD — 카운트다운 신호등이 사라진 뒤 등장(겹침 방지)
	if (m_isRace) this->scheduleOnce([this](float){ this->_setupRace(); }, 1.3f, "race_setup");
	
	
	
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
	m_lastIsNewRecord = isNewRecord;   // 재생 종료 후 결과 텍스트 복원용
	if (m_isRace) {
		// 레이스: 상단 "Perfect!" 티커 생략(결과는 팝업 YOU WIN/LOSE), 트랙 HUD 정리
		this->unschedule("race_setup");
		this->unschedule("race_anim");
		if (m_raceBar)       m_raceBar->setVisible(false);
		if (m_raceYouIcon)   m_raceYouIcon->setVisible(false);
		if (m_raceGapLabel)  m_raceGapLabel->setVisible(false);
		// 승리 시 고스트 K.O. 연출(battle_reward): 회전하며 추락+페이드 → 리셋(리플레이 재사용 대비).
		// 패배 시 즉시 숨김.
		if (m_raceGhostIcon) {
			if (m_mastTime < m_ghostScoreMs) {
				m_raceGhostIcon->stopAllActions();
				m_raceGhostIcon->runAction(Sequence::create(
					Spawn::create(
						RotateBy::create(0.6f, 540.0f),
						MoveBy::create(0.6f, Vec2(0, -40)),
						FadeOut::create(0.6f),
						nullptr),
					CallFunc::create([this]() {
						if (m_raceGhostIcon) {
							m_raceGhostIcon->setVisible(false);
							m_raceGhostIcon->setOpacity(255);
							m_raceGhostIcon->setRotation(0);
						}
					}),
					nullptr));
			} else {
				m_raceGhostIcon->setVisible(false);
			}
		}
	} else {
		showHudResult(isNewRecord, rt);
	}
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

		// 최고기록 리플레이 로컬 영속화 (docs §5). 비정상(캡 초과)·과대 리플레이는 저장 안 함 → 기록은 유효.
		bool replaySaved = false;
		if (!m_replay.empty() && !m_replayOverflow) {
			std::string blob = encodeReplayBlob(m_replay, m_countOfDiscus, m_mastTime);
			if ((int)blob.size() <= REPLAY_BLOB_MAX_CHARS) {   // C-6 클라측 상한
				UserDefault::getInstance()->setStringForKey(replayKey(m_countOfDiscus).c_str(), blob);
				replaySaved = true;
			}
		}
		if (!replaySaved) {
			// 저장 불가 → 신기록과 불일치하는 옛 저장본 제거 (틀린 리플레이 업로드 방지)
			UserDefault::getInstance()->deleteValueForKey(replayKey(m_countOfDiscus).c_str());
		}
		UserDefault::getInstance()->flush();
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

	// 팝업 박스 (리플레이 버튼 공간 확보 위해 세로 확장)
	const float PW = 280, PH = 218;
	auto popupBox = LayerColor::create(Color4B(10, 15, 50, 230), PW, PH);
	popupBox->setPosition(Vec2((RESOURCE_WIDTH - PW) / 2, (RESOURCE_HEIGHT - PH) / 2));
	popupBox->setScale(0.7f);
	popupBox->setTag(4242);   // battle_reward: RANK STOLEN 배너 비동기 부착용 핸들
	overlay->addChild(popupBox);
	popupBox->runAction(Sequence::create(
		ScaleTo::create(0.15f, 1.05f),
		ScaleTo::create(0.08f, 1.0f),
		nullptr
	));

	// 타이틀 (레이스면 승패로 교체)
	bool raceWon = false;
	std::string titleStr;  Color3B titleColor;
	if (m_isRace) {
		raceWon    = (m_mastTime < m_ghostScoreMs);
		titleStr   = raceWon ? "YOU WIN!" : "YOU LOSE";
		titleColor = raceWon ? Color3B(255, 215, 0) : Color3B(255, 90, 90);
	} else {
		titleStr   = isNewRecord ? "NEW RECORD!" : "LEVEL CLEAR";
		titleColor = isNewRecord ? Color3B(255, 215, 0) : Color3B(100, 220, 255);
	}
	auto titleLabel = Label::createWithSystemFont(titleStr, "Arial", 22);
	titleLabel->setColor(titleColor);
	titleLabel->setPosition(Vec2(PW / 2, PH - 28));
	popupBox->addChild(titleLabel);

	// 레이스 vs 라인 (누구를 상대로, 몇 초 차)
	if (m_isRace) {
		int diff = m_mastTime - m_ghostScoreMs;  int ad = diff < 0 ? -diff : diff;
		std::string vs = raceWon
			? StringUtils::format("\xE2\x9A\x94 Beat %s by %d.%01ds", m_ghostName.c_str(), ad / 1000, (ad % 1000) / 100)
			: StringUtils::format("\xE2\x9A\x94 %s won by %d.%01ds", m_ghostName.c_str(), ad / 1000, (ad % 1000) / 100);
		auto vsLbl = Label::createWithSystemFont(vs, "Arial", 11);
		vsLbl->setColor(raceWon ? Color3B(80, 220, 120) : Color3B(255, 150, 150));
		vsLbl->setPosition(Vec2(PW / 2, PH - 64));   // 구분선(PH-52) 아래로 → 겹침 방지
		popupBox->addChild(vsLbl);
	}

	// ── 격파 기록 훅 (battle_reward): 레이스 승리 + 신기록 + 유효 고스트 → recordBattle.
	//    제출(submitScore) 전파 대기 2초 후 호출. 씬을 떠나도 기록되도록 스케줄 타겟은 싱글턴.
	//    서버가 상향 추월(격파)로 판정하면: ① 소감 프리필용 브래그 저장 ② 팝업에 RANK STOLEN 배너.
	if (m_isRace)
		cocos2d::log("[battle] hook check: raceWon=%d isNewRecord=%d ghostId='%s' myId='%s' ghostRank=%d",
			(int)raceWon, (int)isNewRecord, m_ghostPlayFabId.c_str(),
			LeaderboardManager::Instance()->getPlayFabId().c_str(), m_ghostRank);
	if (m_isRace && raceWon && isNewRecord && !m_ghostPlayFabId.empty()
	    && m_ghostPlayFabId != LeaderboardManager::Instance()->getPlayFabId())
	{
		PlayScene* self = this;
		auto alive        = m_aliveFlag;
		int  lvl          = m_countOfDiscus;
		int  aValWas      = bestRecordTime;   // A의 직전 기록(ms, 신기록 갱신 전 값) — 상향 판정용
		std::string loser = m_ghostPlayFabId;
		std::string gname = m_ghostName;
		cocos2d::log("[battle] hook armed: loser=%s aValWas=%d level=%d", loser.c_str(), aValWas, lvl);
		Director::getInstance()->getScheduler()->schedule(
			[self, alive, lvl, aValWas, loser, gname, PW, PH](float) {
				LeaderboardManager::Instance()->recordBattle(lvl, loser, aValWas,
					[self, alive, lvl, gname, PW, PH](bool ok, const std::string& reason) {
						cocos2d::log("[battle] recordBattle result: ok=%d reason=%s", (int)ok, reason.c_str());
						if (!ok) return;   // not_upward/not_passed 등 → 격파 미성립, 조용히 종료
						Director::getInstance()->getScheduler()->performFunctionInCocosThread(
							[self, alive, lvl, gname, PW, PH]() {
								// 소감 프리필용 브래그 저장(씬 생존 무관 — MainScene에서 1회 소비)
								UserDefault::getInstance()->setStringForKey(
									StringUtils::format("battle_brag_L%02d", lvl).c_str(), gname);
								UserDefault::getInstance()->flush();
								// RANK STOLEN 배너 (팝업이 아직 떠 있을 때만)
								if (!alive || !*alive) return;
								auto ov = self->getChildByTag(tagPopup);
								if (!ov) return;
								auto pb = ov->getChildByTag(4242);
								if (!pb) return;
								auto steal = Label::createWithSystemFont("\xF0\x9F\x97\xA1 RANK STOLEN!", "Arial", 13);  // 🗡
								steal->setColor(Color3B(255, 215, 0));
								steal->setPosition(Vec2(PW / 2, PH - 92));
								steal->setOpacity(0);
								pb->addChild(steal, 10);
								steal->runAction(Sequence::create(
									DelayTime::create(0.1f),
									Spawn::create(FadeIn::create(0.25f), ScaleTo::create(0.25f, 1.15f), nullptr),
									ScaleTo::create(0.1f, 1.0f),
									nullptr));
								SoundFactory::Instance()->play("efs_new_record");
							});
					});
			},
			(void*)LeaderboardManager::Instance(),   // 타겟 = 싱글턴(씬 소멸과 무관하게 존속)
			0.0f, 0, 2.0f, false, "battle_record_delay");
	}

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

	// 리플레이 보기 버튼 (1차) — 첫판 플로우는 터치 스왈로우가 있어 제외
	if (!m_isFirstPlay && !m_replay.empty())
	{
		auto replayLabel = Label::createWithSystemFont("▶  REPLAY", "Arial", 13);
		replayLabel->setColor(Color3B(120, 200, 255));
		auto replayItem = MenuItemLabel::create(replayLabel, [this](Ref*) {
			if (m_isReplaying) return;
			SoundFactory::Instance()->play("efs_click");
			this->startReplay();
		});
		auto replayMenu = Menu::create(replayItem, nullptr);
		replayMenu->setPosition(Vec2((m_isRace && !raceWon) ? PW * 0.30f : PW / 2, 48));  // 패배 시 RETRY와 나란히
		popupBox->addChild(replayMenu);
	}

	// 레이스 패배 시: RETRY 버튼 (같은 고스트에게 다시 도전)
	if (m_isRace && !raceWon)
	{
		auto retryLabel = Label::createWithSystemFont("\xE2\x86\xBB  RETRY", "Arial", 13);   // ↻
		retryLabel->setColor(Color3B(255, 180, 90));
		int lvl = m_countOfDiscus, gr = m_ghostRank, gs = m_ghostScoreMs;
		std::string gb = m_ghostBlob, gn = m_ghostName, gpid = m_ghostPlayFabId;
		bool rev = m_isRevenge;
		auto retryItem = MenuItemLabel::create(retryLabel, [lvl, gb, gn, gr, gs, gpid, rev](Ref*) {
			SoundFactory::Instance()->play("efs_click");
			Director::getInstance()->replaceScene(
				TransitionFade::create(0.3f, PlayScene::createRaceScene(lvl, gb, gn, gr, gs, gpid, rev)));
		});
		retryItem->setPosition(Vec2(PW * 0.70f, 48));
		auto retryMenu = Menu::create(retryItem, nullptr);
		retryMenu->setPosition(Vec2::ZERO);
		popupBox->addChild(retryMenu);
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



void PlayScene::abandonByIdle()
{
	if (m_idleAbandoned) return;
	m_idleAbandoned = true;

	m_isIng = COMPLATE;                 // 입력/판정 정지
	this->stopAction(m_actionTimeRun);
	stopEqualizerAnimation();
	unschedule("cheer_start");
	clearBottomPanels();
	if (m_labelRPM) { m_labelRPM->stopAllActions(); m_labelRPM->setVisible(false); }

	CocosDenshion::SimpleAudioEngine::getInstance()->setBackgroundMusicVolume(
		SoundFactory::Instance()->m_mastVolume);
	SoundFactory::Instance()->fadeOutBGM(1.0f);
	SoundFactory::Instance()->play("efs_cancel_select");

	showIdleAbandonPopup();
}

void PlayScene::showIdleAbandonPopup()
{
	auto overlay = LayerColor::create(Color4B(0, 0, 0, 0), RESOURCE_WIDTH, RESOURCE_HEIGHT);
	overlay->setPosition(Vec2::ZERO);
	this->addChild(overlay, tagPopup, tagPopup);
	overlay->runAction(FadeTo::create(0.25f, 180));

	const float PW = 280, PH = 150;
	auto box = LayerColor::create(Color4B(40, 14, 18, 235), PW, PH);
	box->setPosition(Vec2((RESOURCE_WIDTH - PW) / 2, (RESOURCE_HEIGHT - PH) / 2));
	box->setScale(0.7f);
	overlay->addChild(box);
	box->runAction(Sequence::create(
		ScaleTo::create(0.15f, 1.05f), ScaleTo::create(0.08f, 1.0f), nullptr));

	auto title = Label::createWithSystemFont("TIME OUT", "Arial", 22);
	title->setColor(Color3B(255, 120, 100));
	title->setPosition(Vec2(PW / 2, PH - 34));
	box->addChild(title);

	auto msg = Label::createWithSystemFont("No input for 2 minutes.\nGame abandoned.", "Arial", 12);
	msg->setColor(Color3B(215, 215, 215));
	msg->setHorizontalAlignment(TextHAlignment::CENTER);
	msg->setDimensions(PW - 30, 0);
	msg->setAnchorPoint(Vec2(0.5f, 0.5f));
	msg->setPosition(Vec2(PW / 2, PH / 2 - 2));
	box->addChild(msg);

	auto hint = Label::createWithSystemFont("TAP TO CONTINUE", "Arial", 11);
	hint->setColor(Color3B(200, 200, 100));
	hint->setPosition(Vec2(PW / 2, 20));
	box->addChild(hint);
	hint->runAction(RepeatForever::create(
		Sequence::create(FadeOut::create(0.7f), FadeIn::create(0.7f), nullptr)));

	// 첫판이면 재진입 강제 루프 방지
	if (m_isFirstPlay) {
		UserDefault::getInstance()->setBoolForKey("first_play_seen", true);
		UserDefault::getInstance()->flush();
	}

	// 탭 → MainScene (오버레이 z=tagPopup > 터치레이어 → 먼저 소비)
	auto listener = EventListenerTouchOneByOne::create();
	listener->setSwallowTouches(true);
	listener->onTouchBegan = [](Touch*, Event*) {
		SoundFactory::Instance()->play("efs_click");
		Director::getInstance()->replaceScene(
			TransitionFade::create(0.3f, MainScene::createScene()));
		return true;
	};
	_eventDispatcher->addEventListenerWithSceneGraphPriority(listener, overlay);
}

void PlayScene::DrawTime()
{
	if (m_idleAbandoned) return;   // 자동 포기됨 — 잔여 프레임 방어

	int elapsedTime = getMilliCount() - m_dateTime;
	RecordTime recordTime = getRecordTime(elapsedTime);
	int minutes = recordTime.min;
	int seconds = recordTime.sec;
	int ms = recordTime.ms;

	std::string strTime = StringUtils::format("%02d:%02d.%02d", minutes, seconds, ms) ;
	m_labelTime->setString(strTime);

	if (m_isRace) this->_updateRaceHud();   // 3차: 시간 흐름에 따라 고스트 마커 전진
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

	// 120초 무입력 → 게임 자동 포기
	if (m_isIng == PLAY && !m_idleAbandoned &&
	    (getMilliCount() - m_lastActivityMs) >= (int)REPLAY_MAX_DELTA_MS) {
		abandonByIdle();
		return;
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

	// 재생 중에는 완료 판정을 하지 않는다 — DrawDiscus를 재사용하되 결과 팝업 재발생 방지 (docs §6.5)
	if (!m_isReplaying && true == this->CheckSuccess())
	{
		this->Finished();
	}
}


// ---------------------------------------------------------------------------
// 리플레이 (1차: 내 리플레이 보기 + 로컬 영속화) — docs/replay_ghost_plan.md §5,§6
// ---------------------------------------------------------------------------

// Base64 (자체 구현 — 2차 PlayFab 저장에서도 재사용)
static const char* kB64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string b64encode(const std::vector<unsigned char>& d)
{
	std::string out;
	int val = 0, bits = -6;
	for (unsigned char c : d) {
		val = (val << 8) + c; bits += 8;
		while (bits >= 0) { out.push_back(kB64[(val >> bits) & 0x3F]); bits -= 6; }
	}
	if (bits > -6) out.push_back(kB64[((val << 8) >> (bits + 8)) & 0x3F]);
	while (out.size() % 4) out.push_back('=');
	return out;
}

static bool b64decode(const std::string& s, std::vector<unsigned char>& out)
{
	int T[256]; for (int i = 0; i < 256; ++i) T[i] = -1;
	for (int i = 0; i < 64; ++i) T[(unsigned char)kB64[i]] = i;
	int val = 0, bits = -8;
	for (unsigned char c : s) {
		if (c == '=') break;
		if (T[c] == -1) return false;
		val = (val << 6) + T[c]; bits += 6;
		if (bits >= 0) { out.push_back((unsigned char)((val >> bits) & 0xFF)); bits -= 8; }
	}
	return true;
}

// 리플레이 직렬화 → Base64  (docs/replay_ghost_plan.md §3)
//  헤더 6B: [u8 discus][u16 count][u24 finalMs]
//  이벤트 (병합 delta-varint, ms 해상도):
//    첫 바이트 = [type:2][pole:2][cont:1][Δ[0:2]:3]
//    cont=1 이면 이어서 LEB128 로 (Δ>>3) 의 상위 비트
//  ※ Δ = 직전 이벤트와의 시간차. 정수라 드리프트 0, 누적 합 = finalMs 로 정확 수렴.
//    120초 무입력이면 게임 자동 포기되므로 Δ ≤ 120s → 이벤트 최대 3바이트 (인코딩 시 clamp로 보장).
//    (REPLAY_MAX_DELTA_MS 정의는 파일 상단)
static void putVarint(std::vector<unsigned char>& b, uint32_t v)   // LEB128 unsigned
{
	while (v >= 0x80) { b.push_back((unsigned char)(v | 0x80)); v >>= 7; }
	b.push_back((unsigned char)v);
}

static std::string encodeReplayBlob(const std::vector<ReplayEvent>& ev, int discus, int finalMs)
{
	std::vector<unsigned char> b;
	b.push_back((uint8_t)discus);
	uint16_t cnt = (uint16_t)ev.size();
	b.push_back((uint8_t)(cnt & 0xFF)); b.push_back((uint8_t)((cnt >> 8) & 0xFF));
	uint32_t fm = (uint32_t)finalMs;
	for (int i = 0; i < 3; ++i) b.push_back((uint8_t)((fm >> (i * 8)) & 0xFF));   // u24

	uint32_t encAcc = 0;   // 인코딩 누적(= 디코딩 재현값). clamp 시에도 재현 일관성 유지.
	for (const auto& e : ev) {
		uint32_t d = (e.t_ms > encAcc) ? (e.t_ms - encAcc) : 0;
		if (d > REPLAY_MAX_DELTA_MS) d = REPLAY_MAX_DELTA_MS;   // 방어적 clamp (정상 플레이엔 발생 안 함)
		encAcc += d;

		uint32_t rest = d >> 3;
		uint8_t first = (uint8_t)(((e.type & 0x3) << 6) | ((e.pole & 0x3) << 4)
		                          | ((rest ? 1u : 0u) << 3) | (d & 0x7));
		b.push_back(first);
		if (rest) putVarint(b, rest);
	}
	return b64encode(b);
}

static bool decodeReplayBlob(const std::string& b64, std::vector<ReplayEvent>& ev, int& discus, int& finalMs)
{
	std::vector<unsigned char> b;
	if (!b64decode(b64, b) || b.size() < 6) return false;
	size_t p = 0;
	auto avail = [&](size_t n){ return p + n <= b.size(); };

	discus = b[p++];
	if (discus < 3 || discus > MAX_PLAY_LEVEL) return false;   // 범위 밖 → 손상/구포맷 거부(fallback)
	uint16_t cnt = (uint16_t)(b[p] | (b[p+1] << 8)); p += 2;
	uint32_t fm  = b[p] | (b[p+1] << 8) | ((uint32_t)b[p+2] << 16); p += 3;   // u24
	finalMs = (int)fm;

	ev.clear();
	ev.reserve(cnt);
	uint32_t acc = 0;
	for (uint16_t i = 0; i < cnt; ++i) {
		if (!avail(1)) return false;
		uint8_t first = b[p++];
		uint32_t d = (uint32_t)(first & 0x7);
		if (first & 0x08) {                          // 연속 → LEB128 상위 비트
			uint32_t leb = 0; int shift = 0;
			while (true) {
				if (!avail(1) || shift > 28) return false;
				uint8_t c = b[p++];
				leb |= (uint32_t)(c & 0x7F) << shift;
				if (!(c & 0x80)) break;
				shift += 7;
			}
			d |= leb << 3;
		}
		acc += d;
		ReplayEvent e;
		e.type = (uint8_t)((first >> 6) & 0x3);
		e.pole = (uint8_t)((first >> 4) & 0x3);
		e.t_ms = acc;
		ev.push_back(e);
	}
	return true;
}

static std::string replayKey(int level) { return StringUtils::format("replay_L%d", level); }

void PlayScene::recordReplayEvent(ReplayEventType type, int pole)
{
	if (m_isReplaying) return;            // 재생 중 발생 이벤트는 재녹화 안 함
	if (pole < 0 || pole > 2) return;
	if (m_replayOverflow) return;         // A-2: 이미 캡 초과 → 녹화 중단

	int t = getMilliCount() - m_dateTime;

	// A-1: 잡음 이벤트(FAIL/DESELECT)만 40ms 레이트 디바운스 — 봇 스팸 억제.
	//      게임 동작(사운드/UX)은 그대로, 녹화만 스킵. SELECT/MOVE_OK는 재생 정합성 위해 항상 녹화.
	if ((type == EV_MOVE_FAIL || type == EV_DESELECT) &&
	    (t - m_lastReplayEventMs) < REPLAY_JUNK_DEBOUNCE_MS)
		return;

	// A-2: 레벨별 하드 이벤트 캡. 초과 시 녹화 중단 + 저장불가 플래그(기록은 유효).
	//      cap = (최소이동 2^N-1)×4 + 256 → 정상 Top10(≈이동×2.1)엔 절대 안 걸림.
	int cap = ((1 << m_countOfDiscus) - 1) * 4 + 256;
	if ((int)m_replay.size() >= cap) { m_replayOverflow = true; return; }

	ReplayEvent ev;
	ev.type = (uint8_t)type;
	ev.pole = (uint8_t)pole;
	ev.t_ms = (uint32_t)t;
	m_replay.push_back(ev);
	m_lastReplayEventMs = t;
}

void PlayScene::startReplay()
{
	if (m_isReplaying) return;

	// 저장된 최고기록 리플레이 우선 로드 (없거나 손상 시 현재 세션 폴백)
	std::vector<ReplayEvent> loaded;
	int lvl = 0, finalMs = 0;
	std::string blob = UserDefault::getInstance()->getStringForKey(replayKey(m_countOfDiscus).c_str(), "");
	if (!blob.empty() && decodeReplayBlob(blob, loaded, lvl, finalMs)
	    && lvl == m_countOfDiscus && !loaded.empty()) {
		m_replay        = loaded;         // 최고기록 리플레이로 교체
		m_replayFinalMs = finalMs;
	} else if (!m_replay.empty()) {
		m_replayFinalMs = m_mastTime;     // 폴백: 방금 플레이한 세션
	} else {
		return;                           // 재생할 것이 없음
	}

	m_isReplaying = true;

	// 결과 팝업은 제거하지 않고 숨긴다 — 제거 후 재생성 시 MessagePopup이 점수를 재제출하기 때문 (docs §6.5)
	auto ov = this->getChildByTag(tagPopup);
	if (ov) ov->setVisible(false);

	// 보드 초기 상태 복원 (전 디스크 폴 0) 후 즉시 렌더 — 가드로 Finished 안 뜸
	this->ResetGame();
	this->DrawDiscus();
	this->SelectPole(-1, false);

	// 타이머 재생용으로 다시 표시
	if (m_labelTime) { m_labelTime->setVisible(true); m_labelTime->setString("00:00.00"); }

	if (m_isRace)
	{
		// 레이스 리플레이: 고스트 대결 화면 그대로 재현 — 트랙 바/마커 다시 켜고 재생 클럭에 동기.
		if (m_labelLevel)     m_labelLevel->setVisible(false);
		if (m_hudTickerLabel) m_hudTickerLabel->setVisible(false);   // 티커가 트랙과 겹치므로 숨김
		if (m_labelTime) {                                            // 시간은 레이스처럼 오른쪽 정렬 유지
			m_labelTime->setAnchorPoint(Vec2(1.0f, 0.5f));
			m_labelTime->setPosition(Vec2(RESOURCE_WIDTH - 10, RESOURCE_HEIGHT - 13.0f));
		}
		if (m_raceBar)       m_raceBar->setVisible(true);
		if (m_raceYouIcon)   m_raceYouIcon->setVisible(true);
		if (m_raceGhostIcon) m_raceGhostIcon->setVisible(true);
		m_youTargetX = m_ghostTargetX = -1.0f;   // 첫 프레임에 시작점으로 즉시 스냅 후 스프링
		m_youVel = m_ghostVel = 0.0f;
		m_ghostFinishedShown = true;             // 리플레이 중에는 GHOST FINISHED 연출 생략
		this->schedule([this](float dt){         // 마커 스프링(재스케줄, Finished에서 해제됐으므로)
			if (dt > 0.033f) dt = 0.033f;
			const float K = RACE_SPRING_K, D = RACE_SPRING_D;
			if (m_raceYouIcon && m_youTargetX >= 0.0f) {
				float x = m_raceYouIcon->getPositionX();
				m_youVel += ((m_youTargetX - x) * K - m_youVel * D) * dt;
				if (m_youVel >  RACE_SPRING_VMAX) m_youVel =  RACE_SPRING_VMAX;
				if (m_youVel < -RACE_SPRING_VMAX) m_youVel = -RACE_SPRING_VMAX;
				m_raceYouIcon->setPositionX(x + m_youVel * dt);
			}
			if (m_raceGhostIcon && m_ghostTargetX >= 0.0f) {
				float x = m_raceGhostIcon->getPositionX();
				m_ghostVel += ((m_ghostTargetX - x) * K - m_ghostVel * D) * dt;
				if (m_ghostVel >  RACE_SPRING_VMAX) m_ghostVel =  RACE_SPRING_VMAX;
				if (m_ghostVel < -RACE_SPRING_VMAX) m_ghostVel = -RACE_SPRING_VMAX;
				m_raceGhostIcon->setPositionX(x + m_ghostVel * dt);
			}
		}, "race_anim");
	}
	else
	{
		// 상단바 정리: 결과 텍스트/레벨 숨기고 REPLAY 인디케이터만 표시 (타이머와 겹침 방지)
		if (m_labelLevel) m_labelLevel->setVisible(false);
		if (m_hudTickerLabel)
		{
			m_hudTickerLabel->stopAllActions();
			m_hudTickerLabel->setString("\xE2\x96\xB6 REPLAY");   // ▶ REPLAY
			m_hudTickerLabel->setColor(Color3B(120, 200, 255));
			m_hudTickerLabel->setOpacity(255);
			m_hudTickerLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
			m_hudTickerLabel->setPosition(Vec2(RESOURCE_WIDTH * 0.5f, RESOURCE_HEIGHT - 13.0f));
			m_hudTickerLabel->setVisible(true);
		}
	}

	m_replayClock        = 0.0;
	m_replayIndex        = 0;
	m_replaySelectedPole = -1;
	m_replaySpeed        = 1.0f;

	// 배속 스테퍼 (◀ SPEED xN ▶, x0.5~x4, 글로벌 저장)
	this->_buildReplaySpeedControl();

	this->schedule([this](float dt){ this->_updateReplay(dt); }, 0.0f, "replay_update");
}

void PlayScene::_updateReplay(float dt)
{
	m_replayClock += (double)dt * 1000.0 * (double)m_replaySpeed;   // ms (배속 반영)

	// playbackClock 에 도달한 이벤트를 순서대로 적용
	while (m_replayIndex < m_replay.size() &&
	       (double)m_replay[m_replayIndex].t_ms <= m_replayClock)
	{
		this->applyReplayEvent(m_replay[m_replayIndex]);
		++m_replayIndex;
	}

	// 타이머 갱신 (재생본의 최종 시간을 넘지 않게 캡)
	int shown = (int)m_replayClock;
	if (shown > m_replayFinalMs) shown = m_replayFinalMs;
	if (m_labelTime)
	{
		RecordTime rt = getRecordTime(shown);
		m_labelTime->setString(StringUtils::format("%02d:%02d.%02d", rt.min, rt.sec, rt.ms));
	}

	// 레이스 리플레이: 트랙 바/마커를 재생 클럭에 맞춰 갱신 (고스트도 함께 달림)
	if (m_isRace) this->_updateRaceHud();

	// 모든 이벤트 소진 + 최종 시간 도달 → 종료
	if (m_replayIndex >= m_replay.size() && m_replayClock >= (double)m_replayFinalMs)
	{
		this->endReplay();
	}
}

void PlayScene::applyReplayEvent(const ReplayEvent& ev, bool silent)
{
	// silent=true → 건너뛰기(fast-forward) 시 상태만 갱신, 소리/라이트/딜레이 생략
	int pole = ev.pole;
	switch (ev.type)
	{
	case EV_SELECT:
		m_replaySelectedPole = pole;
		if (!silent) {
			this->SelectPole(pole, true);                   // 라이트 ON
			SoundFactory::Instance()->play("efs_select_disc");
		}
		break;

	case EV_MOVE_OK:
		// 직전 SELECT 폴의 top 디스크를 목적 폴로 이동 (즉시 위치 변경 — 애니 없음)
		if (m_replaySelectedPole >= 0 && m_replaySelectedPole != pole &&
		    m_replaySelectedPole < (int)m_pole.size() && pole < (int)m_pole.size())
		{
			auto& src = m_pole[m_replaySelectedPole];
			if (!src.empty())
			{
				Discus* d = src.back();
				src.pop_back();
				d->SetCurrPoleID(pole);
				m_pole[pole].push_back(d);
			}
		}
		if (!silent) {
			this->DrawDiscus();                             // 위치 갱신 (가드로 Finished 안 뜸)
			this->SelectPole(-1, false);                    // 라이트 OFF
			SoundFactory::Instance()->play("efs_move_disc_ok");
		}
		m_replaySelectedPole = -1;
		break;

	case EV_MOVE_FAIL:
		if (!silent) {
			this->SelectPole(pole, false);                  // 실패 마크 (불 꺼짐 연출)
			SoundFactory::Instance()->play("efs_cancel_select");
			this->scheduleOnce([this](float){ this->SelectPole(-1, false); }, 0.2f, "replay_fail_off");
		}
		m_replaySelectedPole = -1;
		break;

	case EV_DESELECT:
		if (!silent) {
			this->SelectPole(-1, false);                    // 선택 취소, 라이트 OFF
			SoundFactory::Instance()->play("efs_move_disc_ok");
		}
		m_replaySelectedPole = -1;
		break;
	}
}

void PlayScene::skipReplay()
{
	if (!m_isReplaying) return;
	// 남은 이벤트를 무음으로 즉시 적용 → 최종(클리어) 상태로 점프
	for (; m_replayIndex < m_replay.size(); ++m_replayIndex)
		this->applyReplayEvent(m_replay[m_replayIndex], true);

	this->DrawDiscus();                                     // 최종 위치 렌더 (가드로 Finished 안 뜸)
	m_replayClock = (double)m_replayFinalMs;
	this->endReplay();
}

// 배속 스테퍼 ◀ SPEED xN ▶ 생성 — 저장된 배속 로드, 재생/관전 공용
void PlayScene::_buildReplaySpeedControl()
{
	if (m_replaySpeedLabel) { m_replaySpeedLabel->removeFromParent(); m_replaySpeedLabel = nullptr; }
	if (m_replaySpeedMenu)  { m_replaySpeedMenu->removeFromParent();  m_replaySpeedMenu  = nullptr; }

	// 저장된 배속 로드 → 인덱스 매칭 (없거나 불일치 시 x1)
	float saved = UserDefault::getInstance()->getFloatForKey("replay_speed", 1.0f);
	m_replaySpeedIdx = 1;
	for (int i = 0; i < kReplaySpeedCount; ++i) {
		float d = kReplaySpeeds[i] - saved; if (d < 0) d = -d;
		if (d < 0.01f) { m_replaySpeedIdx = i; break; }
	}
	m_replaySpeed = kReplaySpeeds[m_replaySpeedIdx];

	const float cx = RESOURCE_WIDTH / 2, y = 26.0f;

	auto leftLbl = Label::createWithSystemFont("\xE2\x97\x80", "Arial", 16);   // ◀ 감속
	leftLbl->setColor(Color3B(180, 210, 255));
	auto leftItem = MenuItemLabel::create(leftLbl, [this](Ref*){ this->_stepReplaySpeed(-1); });

	auto rightLbl = Label::createWithSystemFont("\xE2\x96\xB6", "Arial", 16);  // ▶ 가속
	rightLbl->setColor(Color3B(180, 210, 255));
	auto rightItem = MenuItemLabel::create(rightLbl, [this](Ref*){ this->_stepReplaySpeed(+1); });

	m_replaySpeedMenu = Menu::create(leftItem, rightItem, nullptr);
	m_replaySpeedMenu->setPosition(Vec2::ZERO);
	leftItem->setPosition(Vec2(cx - 52, y));
	rightItem->setPosition(Vec2(cx + 52, y));

	m_replaySpeedLabel = Label::createWithSystemFont(
		StringUtils::format("SPEED x%g", (double)m_replaySpeed), "Arial", 14);
	m_replaySpeedLabel->setColor(Color3B(255, 220, 120));
	m_replaySpeedLabel->setPosition(Vec2(cx, y));

	this->addChild(m_replaySpeedMenu, 200);   // 터치레이어 위 → 건너뛰기와 분리
	this->addChild(m_replaySpeedLabel, 201);  // 라벨은 Menu 자식 불가(MenuItem만 허용) → 씬에 직접 부착
}

void PlayScene::_stepReplaySpeed(int dir)
{
	int idx = m_replaySpeedIdx + dir;
	if (idx < 0) idx = 0;
	if (idx > kReplaySpeedCount - 1) idx = kReplaySpeedCount - 1;
	if (idx == m_replaySpeedIdx) { SoundFactory::Instance()->play("efs_cancel_select", 0.4f); return; }  // 경계

	m_replaySpeedIdx = idx;
	m_replaySpeed    = kReplaySpeeds[idx];
	if (m_replaySpeedLabel)
		m_replaySpeedLabel->setString(StringUtils::format("SPEED x%g", (double)m_replaySpeed));

	UserDefault::getInstance()->setFloatForKey("replay_speed", m_replaySpeed);   // 글로벌 저장
	UserDefault::getInstance()->flush();
	SoundFactory::Instance()->play("efs_click");
}

// ── 3차: 고스트 레이스 ────────────────────────────────────────────────
// distance-to-solved: poleOfSize[1..N]=디스크 크기별 폴, target=목표폴 → 최소 이동수
static int hanoiRemaining(const int* poleOfSize, int N, int target)
{
	int t = target, c = 0;
	for (int k = N; k >= 1; --k) {
		int p = poleOfSize[k];
		if (p != t) { c += (1 << (k - 1)); t = 3 - p - t; }
	}
	return c;
}

int PlayScene::_movesRemaining()
{
	int N = m_countOfDiscus;
	int ids[16], poles[16], n = 0;
	for (int p = 0; p < 3; ++p)
		for (auto d : m_pole[p]) { ids[n] = d->GetDiscusID(); poles[n] = p; ++n; }
	int poleOfSize[16];
	for (int i = 0; i < n; ++i) {                 // ⚠ 이 게임은 ID 클수록 작은 디스크 → 큰 ID일수록 rank 낮음
		int rank = 1;                             // rank N = 물리적으로 가장 큰 디스크 (= 가장 작은 ID)
		for (int j = 0; j < n; ++j) if (ids[j] > ids[i]) ++rank;
		poleOfSize[rank] = poles[i];
	}
	return hanoiRemaining(poleOfSize, N, 2);      // 목표 = 폴 2
}

void PlayScene::_setupRace()
{
	std::vector<ReplayEvent> ev; int lvl = 0, fm = 0;
	if (!decodeReplayBlob(m_ghostBlob, ev, lvl, fm) || ev.empty()) { m_isRace = false; return; }

	// 블롭과 점수를 항상 일치시킨다. 리더보드 scoreMs는 신기록 직후 캐시 지연으로
	// blob(구 기록)과 어긋날 수 있어 "고스트가 골 도달 전 GHOST FINISHED" 버그를 유발한다.
	// 실제 재생되는 고스트 blob의 최종 시간을 마커/골인/승패 판정의 단일 기준으로 사용. (docs §10)
	m_ghostScoreMs = fm;

	int N = m_countOfDiscus;
	m_raceTotalMoves = (1 << N) - 1;
	m_ghostReach.assign(m_raceTotalMoves + 1, 0x7fffffff);
	m_ghostReach[m_raceTotalMoves] = 0;           // 시작 시 남은수=total, t=0

	// 가상 보드(크기 1..N)로 고스트 이벤트 재생 → 각 MOVE_OK 후 남은수 기록
	std::vector<int> board[3];
	for (int s = N; s >= 1; --s) board[0].push_back(s);   // 폴0에 큰 것부터 바닥
	int sel = -1, minR = m_raceTotalMoves;
	for (const auto& e : ev) {
		if (e.type == EV_SELECT) sel = e.pole;
		else if (e.type == EV_MOVE_OK) {
			if (sel >= 0 && sel < 3 && !board[sel].empty() && e.pole < 3) {
				int d = board[sel].back(); board[sel].pop_back(); board[e.pole].push_back(d);
			}
			sel = -1;
			int poleOfSize[16];
			for (int p = 0; p < 3; ++p) for (int d : board[p]) poleOfSize[d] = p;
			int r = hanoiRemaining(poleOfSize, N, 2);
			if (r < minR) { for (int rr = r; rr < minR; ++rr) m_ghostReach[rr] = (int)e.t_ms; minR = r; }
		} else sel = -1;   // FAIL/DESELECT
	}

	m_ghostFinishedShown = false;

	// HUD: 트랙 바를 상단 정보바 안에. 레벨 숨김 + 시간 맨 오른쪽으로 공간 확보.
	if (m_labelLevel) m_labelLevel->setVisible(false);
	if (m_labelRPM)   m_labelRPM->setVisible(false);
	if (m_labelTime) {
		m_labelTime->setAnchorPoint(Vec2(1.0f, 0.5f));
		m_labelTime->setPosition(Vec2(RESOURCE_WIDTH - 10, RESOURCE_HEIGHT - 13.0f));
	}
	m_raceBar = DrawNode::create();
	this->addChild(m_raceBar, 5);
	m_raceGhostIcon = Label::createWithSystemFont("\xF0\x9F\x91\xBB", "Arial", 14);  // 👻
	this->addChild(m_raceGhostIcon, 6);
	m_raceGapLabel = Label::createWithSystemFont("", "Arial", 9);
	this->addChild(m_raceGapLabel, 6);
	m_raceYouIcon = Label::createWithSystemFont("\xF0\x9F\x8F\x83", "Arial", 14);    // 🏃 (항상 위)
	this->addChild(m_raceYouIcon, 7);
	m_raceYouIcon->setScaleX(-1.0f);     // 진행 방향(왼→오)을 보도록 좌우 반전
	m_raceGhostIcon->setScaleX(-1.0f);
	m_youTargetX = m_ghostTargetX = -1.0f;
	m_youVel = m_ghostVel = 0.0f;

	// 마커 스프링 이동(매 프레임, 임계감쇠) — 부드러운 가속·감속
	this->schedule([this](float dt){
		if (dt > 0.033f) dt = 0.033f;
		const float K = 180.0f, D = 26.0f;
		if (m_raceYouIcon && m_youTargetX >= 0.0f) {
			float x = m_raceYouIcon->getPositionX();
			m_youVel += ((m_youTargetX - x) * K - m_youVel * D) * dt;
			m_raceYouIcon->setPositionX(x + m_youVel * dt);
		}
		if (m_raceGhostIcon && m_ghostTargetX >= 0.0f) {
			float x = m_raceGhostIcon->getPositionX();
			m_ghostVel += ((m_ghostTargetX - x) * K - m_ghostVel * D) * dt;
			m_raceGhostIcon->setPositionX(x + m_ghostVel * dt);
		}
	}, "race_anim");

	_updateRaceHud();
}

void PlayScene::_updateRaceHud()
{
	if (!m_isRace || m_ghostReach.empty() || !m_raceBar) return;
	int R  = _movesRemaining();
	if (R < 0) R = 0; if (R > m_raceTotalMoves) R = m_raceTotalMoves;
	// 라이브: 벽시계 경과. 리플레이: 재생 클럭(배속 반영) — 고스트 마커를 리플레이 시간축에 맞춤.
	int t  = m_isReplaying ? (int)m_replayClock : (getMilliCount() - m_dateTime);

	// 고스트 골인 순간 연출 (내가 아직 플레이 중일 때)
	if (!m_ghostFinishedShown && t >= m_ghostScoreMs && m_isIng == PLAY) {
		m_ghostFinishedShown = true;
		auto f = Label::createWithSystemFont("\xF0\x9F\x91\xBB GHOST FINISHED", "Arial", 20);  // 👻
		f->setColor(Color3B(200, 200, 255));
		f->setPosition(Vec2(RESOURCE_WIDTH / 2, RESOURCE_HEIGHT / 2 + 40));
		f->setScale(0.6f); f->setOpacity(0);
		this->addChild(f, 56);
		f->runAction(Sequence::create(
			Spawn::create(ScaleTo::create(0.2f, 1.0f), FadeIn::create(0.2f), nullptr),
			DelayTime::create(0.9f), FadeOut::create(0.5f), RemoveSelf::create(), nullptr));
		SoundFactory::Instance()->play("efs_cancel_select", 0.5f);
	}

	// 트랙(정보바 내, 최대한 넓게 — 시간 침범 안 함): |끝점|━━ 델타값 ━━
	const float x0 = 18, x1 = 378, ty = RESOURCE_HEIGHT - 13.0f;
	const Color4F trackCol(0.45f, 0.45f, 0.52f, 0.85f);
	float total = (float)m_raceTotalMoves;
	int gR = m_raceTotalMoves;
	for (int r = 0; r <= m_raceTotalMoves; ++r) if (m_ghostReach[r] <= t) { gR = r; break; }
	float youX = x0 + (x1 - x0) * ((total - R)  / total);
	float ghX  = x0 + (x1 - x0) * ((total - gR) / total);
	float mx   = (youX + ghX) * 0.5f;              // 델타값 위치(두 마커 중간)

	int d  = m_ghostReach[R] - t;                  // + = 내가 더 빨리 도달 = 앞섬
	int ad = d < 0 ? -d : d;
	bool showDelta = (ad >= 3000);                 // 3초 이상 차이날 때만 델타값 표시
	if (m_raceGapLabel) {
		m_raceGapLabel->setVisible(showDelta);
		if (showDelta) {
			m_raceGapLabel->setString(StringUtils::format("%d.%01ds", ad / 1000, (ad % 1000) / 100));
			m_raceGapLabel->setColor(d >= 0 ? Color3B(90, 220, 130) : Color3B(255, 100, 100));
			m_raceGapLabel->setPosition(Vec2(mx, ty));
		}
	}
	const float gap = showDelta ? 14.0f : 0.0f;    // 델타 표시 시만 라인 끊김, 아니면 연속

	m_raceBar->clear();
	m_raceBar->drawSegment(Vec2(x0, ty - 4), Vec2(x0, ty + 4), 1.0f, trackCol);  // | 시작 끝점
	m_raceBar->drawSegment(Vec2(x1, ty - 4), Vec2(x1, ty + 4), 1.0f, trackCol);  // | 골 끝점
	if (mx - gap > x0) m_raceBar->drawSegment(Vec2(x0, ty),       Vec2(mx - gap, ty), 1.0f, trackCol);
	if (mx + gap < x1) m_raceBar->drawSegment(Vec2(mx + gap, ty), Vec2(x1, ty),       1.0f, trackCol);

	// 마커: 목표만 설정(실제 이동은 race_anim 스프링이 매 프레임). 최초는 즉시 배치.
	if (m_raceYouIcon   && m_youTargetX   < 0) m_raceYouIcon->setPosition(Vec2(youX, ty));
	if (m_raceGhostIcon && m_ghostTargetX < 0) m_raceGhostIcon->setPosition(Vec2(ghX,  ty));
	m_youTargetX   = youX;
	m_ghostTargetX = ghX;
}

void PlayScene::endReplay()
{
	this->unschedule("replay_update");
	this->unschedule("replay_fail_off");
	m_isReplaying        = false;
	m_replaySelectedPole = -1;
	this->SelectPole(-1, false);

	// 배속 스테퍼 제거 (라벨은 씬에 직접 부착되므로 따로 제거)
	if (m_replaySpeedLabel) { m_replaySpeedLabel->removeFromParent(); m_replaySpeedLabel = nullptr; }
	if (m_replaySpeedMenu)  { m_replaySpeedMenu->removeFromParent();  m_replaySpeedMenu  = nullptr; }

	// 관전 모드: 재생 종료 → 선택 팝업(REPLAY/HOME). 바로 나가지 않음.
	if (m_isSpectate) {
		this->showSpectateEndPopup();
		return;
	}

	// 레이스 리플레이: 트랙 HUD 정리 후 결과 팝업만 복원 (레벨/티커 복원 안 함 — 레이스는 원래 숨김)
	if (m_isRace) {
		this->unschedule("race_anim");
		if (m_labelTime)     m_labelTime->setVisible(false);
		if (m_raceBar)       m_raceBar->setVisible(false);
		if (m_raceYouIcon)   m_raceYouIcon->setVisible(false);
		if (m_raceGhostIcon) m_raceGhostIcon->setVisible(false);
		if (m_raceGapLabel)  m_raceGapLabel->setVisible(false);
		auto ov = this->getChildByTag(tagPopup);
		if (ov) ov->setVisible(true);
		return;
	}

	// 타이머 숨기고 상단바(레벨/결과 텍스트) 복원
	if (m_labelTime) m_labelTime->setVisible(false);
	if (m_labelLevel) m_labelLevel->setVisible(true);
	showHudResult(m_lastIsNewRecord, getRecordTime(m_mastTime));   // 결과 텍스트 원복

	// 결과 팝업 복원 (다시보기 재요청 가능)
	auto ov = this->getChildByTag(tagPopup);
	if (ov) ov->setVisible(true);
}

void PlayScene::onEnterTransitionDidFinish()
{
	Scene::onEnterTransitionDidFinish();
	// 관전 모드: 씬 진입 완료 시 자동 재생 시작
	if (m_isSpectate && !m_spectateStarted)
		this->startSpectate();
}

void PlayScene::startSpectate()
{
	if (m_spectateStarted) return;
	m_spectateStarted = true;

	std::vector<ReplayEvent> loaded;
	int lvl = 0, finalMs = 0;
	if (!decodeReplayBlob(m_spectateBlob, loaded, lvl, finalMs) || loaded.empty()) {
		// 손상/빈 데이터 → 랭킹보드 복귀
		Director::getInstance()->replaceScene(
			TransitionFade::create(0.3f, MainScene::createScene()));
		return;
	}
	m_replay        = loaded;
	m_replayFinalMs = finalMs;

	// 진입 화면 정리 (카운트다운/아이들/티커 억제) + 좌측 도크 숨김(관전 중 메뉴 미표시)
	this->removeChildByTag(tagCountDown, true);
	stopRankTicker();
	stopIdleAnimation();
	if (auto dock = this->getChildByTag(1234)) dock->setVisible(false);

	this->_beginSpectatePlayback();
}

// 관전 재생 시작(재호출 가능 — 종료 팝업의 REPLAY 버튼에서도 호출)
void PlayScene::_beginSpectatePlayback()
{
	m_isReplaying = true;

	// 보드 초기 상태 복원 + 렌더
	this->ResetGame();
	this->DrawDiscus();
	this->SelectPole(-1, false);

	// 상단바: 관전 인디케이터(랭커 이름) + 타이머
	if (m_labelTime)  { m_labelTime->setVisible(true); m_labelTime->setString("00:00.00"); }
	if (m_labelLevel) m_labelLevel->setVisible(false);
	if (m_hudTickerLabel) {
		m_hudTickerLabel->stopAllActions();
		std::string ind = "\xE2\x96\xB6 " + (m_spectateName.empty() ? std::string("REPLAY") : m_spectateName);
		m_hudTickerLabel->setString(ind);
		m_hudTickerLabel->setColor(Color3B(120, 200, 255));
		m_hudTickerLabel->setOpacity(255);
		m_hudTickerLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
		m_hudTickerLabel->setPosition(Vec2(RESOURCE_WIDTH * 0.5f, RESOURCE_HEIGHT - 13.0f));
		m_hudTickerLabel->setVisible(true);
	}

	// 배속 스테퍼 (◀ SPEED xN ▶, x0.5~x4, 글로벌 저장)
	this->_buildReplaySpeedControl();

	m_replayClock        = 0.0;
	m_replayIndex        = 0;
	m_replaySelectedPole = -1;
	this->schedule([this](float dt){ this->_updateReplay(dt); }, 0.0f, "replay_update");
}

// 관전 종료 팝업 — REPLAY(다시 재생) / HOME(랭킹보드 복귀)
void PlayScene::showSpectateEndPopup()
{
	auto overlay = LayerColor::create(Color4B(0, 0, 0, 0), RESOURCE_WIDTH, RESOURCE_HEIGHT);
	overlay->setPosition(Vec2::ZERO);
	this->addChild(overlay, tagPopup, tagPopup);
	overlay->runAction(FadeTo::create(0.25f, 170));

	const float PW = 260, PH = 152;
	auto box = LayerColor::create(Color4B(10, 15, 50, 235), PW, PH);
	box->setPosition(Vec2((RESOURCE_WIDTH - PW) / 2, (RESOURCE_HEIGHT - PH) / 2));
	box->setScale(0.7f);
	overlay->addChild(box);
	box->runAction(Sequence::create(
		ScaleTo::create(0.15f, 1.05f), ScaleTo::create(0.08f, 1.0f), nullptr));

	// 랭커 이름
	std::string who = m_spectateName.empty() ? "REPLAY" : m_spectateName;
	auto title = Label::createWithSystemFont(who, "Arial", 16);
	title->setColor(Color3B(120, 200, 255));
	title->setPosition(Vec2(PW / 2, PH - 26));
	box->addChild(title);

	// LEVEL · RANK
	std::string lr = StringUtils::format("LEVEL %d", m_countOfDiscus);
	if (m_spectateRank > 0) lr += StringUtils::format("   \xC2\xB7   RANK %d", m_spectateRank);  // ·
	auto lrLbl = Label::createWithSystemFont(lr, "Arial", 11);
	lrLbl->setColor(Color3B(170, 175, 185));
	lrLbl->setPosition(Vec2(PW / 2, PH - 48));
	box->addChild(lrLbl);

	// 기록 시간 (헤드라인)
	RecordTime rt = getRecordTime(m_spectateScoreMs);
	auto timeLbl = Label::createWithSystemFont(
		StringUtils::format("%02d:%02d.%02d", rt.min, rt.sec, rt.ms), "Arial", 24);
	timeLbl->setColor(Color3B::WHITE);
	timeLbl->setPosition(Vec2(PW / 2, PH - 80));
	box->addChild(timeLbl);

	// 버튼: 🔁 REPLAY / 🏠 HOME
	auto replayLbl = Label::createWithSystemFont("\xF0\x9F\x94\x81  REPLAY", "Arial", 14);
	replayLbl->setColor(Color3B(120, 200, 255));
	auto replayBtn = MenuItemLabel::create(replayLbl, [this](Ref*) {
		SoundFactory::Instance()->play("efs_click");
		this->removeChildByTag(tagPopup);
		this->_beginSpectatePlayback();
	});
	replayBtn->setPosition(Vec2(PW * 0.30f, 30));

	auto homeLbl = Label::createWithSystemFont("\xF0\x9F\x8F\xA0  HOME", "Arial", 14);
	homeLbl->setColor(Color3B(255, 215, 120));
	auto homeBtn = MenuItemLabel::create(homeLbl, [](Ref*) {
		SoundFactory::Instance()->play("efs_click");
		Director::getInstance()->replaceScene(
			TransitionFade::create(0.3f, MainScene::createScene()));
	});
	homeBtn->setPosition(Vec2(PW * 0.70f, 30));

	auto menu = Menu::create(replayBtn, homeBtn, nullptr);
	menu->setPosition(Vec2::ZERO);
	box->addChild(menu, 5);
}


void PlayScene::DrawInfoText ()
{
	// 레벨 표시 위치: 고스트대결이면 "Race", 복수전이면 "Revenge", 그 외엔 "LV.n / max".
	std::string strLevelInfo;
	Color3B     levelCol;
	if (m_isRace && m_isRevenge) {
		strLevelInfo = "Revenge";
		levelCol     = Color3B(255, 90, 90);     // 붉은색(복수/피격 테마)
	} else if (m_isRace) {
		strLevelInfo = "Race";
		levelCol     = Color3B(120, 200, 255);   // 시안(고스트 대결 테마)
	} else {
		strLevelInfo = StringUtils::format("LV.%d / %d", m_countOfDiscus, MAX_PLAY_LEVEL);
		levelCol     = Color3B::WHITE;
	}

	if (NULL == m_labelLevel)
	{
		m_labelLevel = Label::createWithSystemFont(strLevelInfo, "Arial", 13);
		m_labelLevel->setColor(levelCol);
		m_labelLevel->setPosition(Vec2(120, RESOURCE_HEIGHT - 13));
		this->addChild(m_labelLevel, 4, tagInfoText);
		m_labelLevel->setVisible(false);
	}
	else
	{
		m_labelLevel->setString(strLevelInfo);
		m_labelLevel->setColor(levelCol);
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
		recordReplayEvent(EV_DESELECT, poleID);   // 같은 폴 재탭 = 선택 취소 (docs §6.1)
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
	recordReplayEvent(EV_MOVE_OK, poleID);        // 성공 이동 — 탭/드래그 양 경로가 여기 통과 (docs §6.1)
	if (m_isRace) this->_updateRaceHud();         // 3차: 내 진행도/델타 즉시 갱신
	
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
	if (PLAY != m_isIng) return;                // 플레이 중에만 동작
	if (m_isReplaying || m_isSpectate) return;  // 리플레이/관전 중에는 리셋 금지
	if (m_isTransitioning) return;

	m_isTransitioning = true;
	SoundFactory::Instance()->play("efs_click");

	// 3-2-1 카운트다운 "이전"(대기) 상태로 되돌림 → 같은 레벨 씬을 새로 생성.
	// PLAY 중 세팅된 타이머/이퀄라이저/고스트HUD/예약콜백/BGM을 자동 정리하고,
	// 최초 진입과 동일한 "NONE 대기(안내/아이들 연출) → 사용자가 탭하면 카운트다운 시작" 상태가 됨.
	// (리셋 버튼은 first-play 화면엔 없으므로 isFirstPlay=false. race면 고스트 데이터 유지해 재생성)
	PlayScene* fresh = m_isRace
		? PlayScene::createRaceScene(m_countOfDiscus, m_ghostBlob, m_ghostName,
		                             m_ghostRank, m_ghostScoreMs, m_ghostPlayFabId, m_isRevenge)
		: PlayScene::createScene(m_countOfDiscus);
	Director::getInstance()->replaceScene(TransitionFade::create(0.2f, fresh));
}

void PlayScene::callbackOnPushed_prevMenuItem(Ref* sender)
{
	if (m_isSpectate) return;   // 관전 중 레벨 변경(새 게임 시작) 차단
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
	if (m_isSpectate) return;   // 관전 중 레벨 변경(새 게임 시작) 차단
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
