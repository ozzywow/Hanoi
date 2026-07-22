#include "stdafx.h"
#include "PlayScene.h"
#include "PlaySceneTouchHandlerLayer.h"
#include "Discus.h"
#include "SoundFactory.h"
#include "MainScene.h"
#include "UserDataManager.h"
#include "LeaderboardManager.h"
#include "ReviewManager.h"
#include "DrawUtils.h"
#include "PixelFont.h"
#ifdef LITE_VER
#include "IAPManager.h"
#endif

using namespace cocos2d;

#include "PlaySceneInternal.h"

// 폴 3개의 화면 좌표 (여러 구현 파일이 공유 — 선언은 PlaySceneInternal.h)
Point arrPosOfPole[3];




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
	IAPManager::Instance()->ToggleIndicator(false);
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
	IAPManager::Instance()->SetDelegate(this);
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

	// (구 폴대 자물쇠 아이콘 제거 — 잠금 안내는 START 자리의 🔒 버튼이 대신한다. _buildStartButton 참조)

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

	// 시상대(showPodiumRanking)가 중앙 셀 점유 여부를 보고 2칸/3칸을 고르므로,
	// 랭킹 조회보다 먼저 만든다 (캐시 히트로 콜백이 즉시 도는 경우 대비). 관전은 자동 재생 → 생략.
	if (!m_isSpectate) this->_buildStartButton();

	this->startRankTicker(m_countOfDiscus);
	// 하단 바 연출은 전 레벨 동일(아이들 → 시상대). 규칙 가이드는 첫 플레이 튜토리얼에서만.
	if (m_isFirstPlay && m_countOfDiscus == 3)
		this->startGuideAnimation();
	else
		this->startIdleAnimation();
}


// ─── START / 🔒 버튼 (NONE 상태 CTA, BottomInfoBar 중앙 셀 오버레이) ─────────
// 빈 화면 탭으로 게임이 시작되면 미스터치·절전 해제 탭과 겹치므로, 시작은 이 버튼으로만 받는다.
// 디자인: 타이틀과 같은 시그니처 키캡 + 픽셀 START. 색은 카운트다운 "GO!"와 같은 네온 민트로
//        맞춰 [START] → [GO!] 가 하나의 흐름으로 읽히게 한다.
// 연출: 심박(두근-쿵 후 쉼) 스케일 루프 — 키캡·글자를 한 노드에 담아 통째로 뛰게 한다.
void PlayScene::_buildStartButton()
{
	this->_removeStartButton();

	// BottomInfoBar 중앙 셀(디바이더 사이 폭 136, 바 높이 50)에 오버레이.
	// 심박 최대(1.09배)에도 셀 안에 머물도록 126×42로 잡았다.
	const float BW = 126.f, BH = 42.f;
	const Vec2  POS(RESOURCE_WIDTH * 0.5f, BOTTOM_PANEL_Y);

	auto face = Node::create();
	face->setContentSize(Size(BW, BH));
	face->setCascadeOpacityEnabled(true);

	// 미구매 잠금 레벨이면 같은 자리에 🔒 버튼 — 누르면 구매 플로우, 해제되면 START로 교체된다.
	const bool locked = (m_countOfDiscus > MAX_LIMIT_LEVEL_FOR_LITE &&
	                     false == UserDataManager::Instance()->GetCart());

	auto keycap = DrawNode::create();
	drawKeycap(keycap, BW * 0.5f, BH * 0.5f, BW, BH, locked);   // 잠금은 회색 키캡
	face->addChild(keycap, 0);

	if (locked)
	{
		// 자물쇠: 어두운 그림자 한 겹 + 민트 본체 (픽셀 텍스트의 음영 처리와 톤 통일)
		auto lock = DrawNode::create();
		drawVecLock(lock, BW * 0.5f + 1.5f, BH * 0.5f - 1.5f, 36.f, Color4F(0, 0, 0, 0.6f));
		drawVecLock(lock, BW * 0.5f,        BH * 0.5f,        36.f,
		            Color4F(80 / 255.f, 220 / 255.f, 180 / 255.f, 1.f));
		face->addChild(lock, 1);
	}
	else
	{
		Node* text = makePixelText("START", 3.3f, Color3B(80, 220, 180));   // GO! 와 같은 네온 민트
		const Size ts = text->getContentSize();
		text->setPosition(Vec2((BW - ts.width) * 0.5f, (BH - ts.height) * 0.5f));
		face->addChild(text, 1);
	}

	auto item = MenuItemLabel::create(face, [this, locked](Ref* sender) {
		if (locked) { this->callbackLockBtn(sender); return; }   // 잠금 → 구매 요청
		this->CountDown();
	});
	// MenuItemLabel이 face 앵커를 (0,0)으로 돌려놓으므로, 중앙 기준으로 뛰도록 되돌린다
	face->setAnchorPoint(Vec2(0.5f, 0.5f));
	face->setPosition(Vec2(BW * 0.5f, BH * 0.5f));
	item->setPosition(POS);

	m_startMenu = Menu::create(item, nullptr);
	m_startMenu->setPosition(Vec2::ZERO);
	this->addChild(m_startMenu, 200);   // 터치레이어(z=tagTouchingLayer) 위

	// lub-dub: 큰 박동 → 짧은 여진 → 쉼. 사람 심박 리듬이라 "누르라"는 신호로 읽힌다.
	face->runAction(RepeatForever::create(Sequence::create(
		EaseSineOut::create(ScaleTo::create(0.11f, 1.09f)),
		EaseSineIn::create (ScaleTo::create(0.10f, 1.00f)),
		EaseSineOut::create(ScaleTo::create(0.09f, 1.05f)),
		EaseSineIn::create (ScaleTo::create(0.12f, 1.00f)),
		DelayTime::create(0.62f),
		nullptr)));
}

void PlayScene::_removeStartButton()
{
	if (m_startMenu) { m_startMenu->removeFromParent(); m_startMenu = nullptr; }
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

	// 인앱 리뷰: 매 클리어마다 누적 플레이 카운트 증가 (게이팅 재료)
	ReviewManager::Instance()->NotifyGamePlayed();

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

			// 인앱 리뷰 유도 — 신기록이라는 긍정적 순간 직후.
			// 게이팅(누적 3회+버전당 1회)은 ReviewManager가 판정. 결과 팝업이 먼저 뜨도록 1.6초 지연.
			this->runAction(Sequence::create(
				DelayTime::create(1.6f),
				CallFunc::create([]() { ReviewManager::Instance()->MaybeRequestReview(); }),
				nullptr));
		}
		isNewRecord = true;
	}

	// 팝업 박스 크기 (리플레이 버튼 공간 확보 위해 세로 확장)
	const float PW = 280, PH = 218;

	// 톤: 성취(CLEAR/RECORD/WIN)=골드 테두리·타이틀 / 패배(LOSE)=그레이 테두리·레드 타이틀
	bool raceWon = false;
	std::string titleStr;  Color3B titleColor, borderCol;
	if (m_isRace) {
		raceWon    = (m_mastTime < m_ghostScoreMs);
		titleStr   = raceWon ? "YOU WIN!" : "YOU LOSE";
		titleColor = raceWon ? kTextTitle : kTextTitleBad;
		borderCol  = raceWon ? kBorderGold : kBorderGray;
	} else {
		titleStr   = isNewRecord ? "NEW RECORD!" : "LEVEL CLEAR";
		titleColor = kTextTitle;
		borderCol  = kBorderGold;
	}

	// 공용 프레임 (모달은 아래 커스텀 스왈로우가 담당 → modal=false)
	auto f = makePopupFrame(titleStr, borderCol, titleColor, PW, PH, 22.f, kDimStd, false, true);
	this->addChild(f.backdrop, tagPopup, tagPopup);
	auto overlay  = f.backdrop;
	auto popupBox = f.box;
	popupBox->setTag(4242);   // battle_reward: RANK STOLEN 배너 비동기 부착용 핸들

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

	// (구분선은 공용 프레임이 그림)

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

	// 결과 팝업 버튼: [REPLAY][CLOSE] — 레이스 패배 시엔 [REPLAY][RETRY]. 좌우 2버튼 구성.
	bool raceLose  = (m_isRace && !raceWon);
	bool hasReplay = (!m_isFirstPlay && !m_replay.empty());
	if (hasReplay)
	{
		auto replayItem = makePopupChipButton("\xE2\x96\xB6 REPLAY", kBtnFunc, [this](Ref*) {
			if (m_isReplaying) return;
			SoundFactory::Instance()->play("efs_click");
			this->startReplay();
		}, 112.f, 38.f);
		auto replayMenu = Menu::create(replayItem, nullptr);
		replayMenu->setPosition(Vec2(74.f, kPopupBtnY));   // 좌측
		popupBox->addChild(replayMenu);
	}

	// 레이스 패배 시: RETRY 버튼 (같은 고스트에게 다시 도전)
	if (m_isRace && !raceWon)
	{
		int lvl = m_countOfDiscus, gr = m_ghostRank, gs = m_ghostScoreMs;
		std::string gb = m_ghostBlob, gn = m_ghostName, gpid = m_ghostPlayFabId;
		bool rev = m_isRevenge;
		auto retryItem = makePopupChipButton("\xE2\x86\xBB RETRY", kBtnRace, [lvl, gb, gn, gr, gs, gpid, rev](Ref*) {
			SoundFactory::Instance()->play("efs_click");
			Director::getInstance()->replaceScene(
				TransitionFade::create(0.3f, PlayScene::createRaceScene(lvl, gb, gn, gr, gs, gpid, rev)));
		}, 112.f, 38.f);
		retryItem->setPosition(Vec2(PW - 74.f, kPopupBtnY));
		auto retryMenu = Menu::create(retryItem, nullptr);
		retryMenu->setPosition(Vec2::ZERO);
		popupBox->addChild(retryMenu);
	}

	// CLOSE — 팝업만 닫기(완료 보드+도크 노출; 보드 탭 시 현재 레벨 재시작은 터치레이어가 처리).
	//         레이스 패배는 RETRY가 우측 자리를 대신하므로 CLOSE 생략.
	if (!m_isFirstPlay && !raceLose)
	{
		auto closeItem = makePopupChipButton("\xE2\x9C\x95 CLOSE", kBtnDismiss, [this](Ref*) {
			SoundFactory::Instance()->play("efs_click");
			this->removeChildByTag(tagPopup);
		}, 112.f, 38.f);
		auto closeMenu = Menu::create(closeItem, nullptr);
		closeMenu->setPosition(Vec2(hasReplay ? (PW - 74.f) : PW / 2, kPopupBtnY));   // 우측(리플레이 없으면 중앙)
		popupBox->addChild(closeMenu);
	}

	// 힌트 — 첫판만 "TAP TO CONTINUE"(오버레이 탭으로 진행). 그 외엔 명시 버튼이 있어 힌트 없음.
	if (m_isFirstPlay)
	{
		auto hintLabel = Label::createWithSystemFont("TAP TO CONTINUE", "Arial", 11);
		hintLabel->setColor(Color3B(200, 200, 100));
		hintLabel->setPosition(Vec2(PW / 2, 20));
		popupBox->addChild(hintLabel);
		hintLabel->runAction(RepeatForever::create(
			Sequence::create(FadeOut::create(0.7f), FadeIn::create(0.7f), nullptr)
		));
	}

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
	} else {
		// (B) 팝업이 떠 있는 동안 오버레이가 모든 터치를 흡수 — 명시 버튼([REPLAY][CLOSE])만 동작.
		//     버튼 Menu는 더 깊은 노드라 이 리스너보다 먼저 처리되므로 버튼 탭은 정상 동작.
		//     재생 중엔 터치레이어의 스킵 핸들러에 양보(return false).
		auto swallow = EventListenerTouchOneByOne::create();
		swallow->setSwallowTouches(true);
		swallow->onTouchBegan = [this](Touch*, Event*) -> bool {
			return !m_isReplaying;   // 재생 중=false(양보) / 그 외=true(흡수)
		};
		_eventDispatcher->addEventListenerWithSceneGraphPriority(swallow, overlay);
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
	const float PW = 280, PH = 150;
	// 부정(비파괴) 톤 = 그레이 테두리 + 레드 타이틀, 배경 네이비(적갈색 폐기).
	auto f = makePopupFrame("TIME OUT", kBorderGray, kTextTitleBad, PW, PH, 22.f, kDimStd, false, false);
	this->addChild(f.backdrop, tagPopup, tagPopup);
	auto overlay = f.backdrop;
	auto box     = f.box;

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

void PlayScene::onEnterTransitionDidFinish()
{
	Scene::onEnterTransitionDidFinish();
	// 관전 모드: 씬 진입 완료 시 자동 재생 시작
	if (m_isSpectate && !m_spectateStarted)
		this->startSpectate();
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
		this->_removeStartButton();
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
		IAPManager::Instance()->SetDelegate(NULL);
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
	IAPManager::Instance()->SetDelegate(NULL);
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

	IAPManager::Instance()->ToggleIndicator(true);
	IAPManager::Instance()->buyFeature(kProductIdTotal);
#endif //LITE_VER

	SoundFactory::Instance()->play("efs_click");
}




#ifdef LITE_VER
void PlayScene::productFetchComplete()
{
	cocos2d::log("productFetchComplete");
	IAPManager::Instance()->ToggleIndicator(false);
	isProgress = false;	
}
void PlayScene::productPurchased(std::string productId)
{
	cocos2d::log("productPurchased /%s", productId.c_str());
	IAPManager::Instance()->ToggleIndicator(false);
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

		// 🔒 → START (잠금 해제 즉시 이 자리에서 바로 시작 가능)
		if (!m_isSpectate && m_isIng == NONE) this->_buildStartButton();
	}
}
void PlayScene::transactionCanceled()
{
	cocos2d::log("transactionCanceled");
	IAPManager::Instance()->ToggleIndicator(false);
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


	IAPManager::Instance()->ToggleIndicator(false);
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

	// 🔒 → START (복원으로 잠금이 풀린 경우)
	if (!m_isSpectate && m_isIng == NONE) this->_buildStartButton();
}
#endif //LITE_VER
