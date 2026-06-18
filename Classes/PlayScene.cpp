#include "stdafx.h"
#include "PlayScene.h"
#include "PlaySceneTouchHandlerLayer.h"
#include "Discus.h"
#include "SoundFactory.h"
#include "MainScene.h"
#include "UserDataManager.h"
#include "LeaderboardManager.h"
#ifdef LITE_VER
#include "MKStoreManager_cpp.h"
#endif

using namespace cocos2d;

static Point arrPosOfPole[3];
static const float BOTTOM_PANEL_Y      = 28.0f;
static const float BOTTOM_FONT_DEFAULT = 10.0f;
static const int   TAG_GUIDE_ANIM      = 201;
static const int   TAG_PODIUM_BASE     = 310; // 310~315: 시상대 동적 레이블



void PlayScene::onExitTransitionDidStart()
{
	stopIdleAnimation();    // 패널 텍스트 잔류 방지: 가이드/아이들 중단 + clearBottomPanels
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

	// 구분선 (PLAY 상태에서만 표시)
	m_hudSep = Label::createWithSystemFont("|", "Arial", 14);
	m_hudSep->setColor(Color3B(40, 80, 80));
	m_hudSep->setPosition(Vec2(240, RESOURCE_HEIGHT - 13));
	m_hudSep->setVisible(false);
	this->addChild(m_hudSep, tagInfoText, 0);

	// 타이머 (PLAY 상태에서만 표시)
	m_labelTime = Label::createWithSystemFont("00:00.00", "Arial", 13);
	m_labelTime->setColor(Color3B(80, 220, 180));
	m_labelTime->setPosition(Vec2(362, RESOURCE_HEIGHT - 13));
	m_labelTime->setVisible(false);
	this->addChild(m_labelTime, tagInfoText, tagInfoText);

	// 티커 레이블 (NONE: 랭킹 슬라이딩, COMPLATE: 결과 깜빡임)
	m_hudTickerLabel = Label::createWithSystemFont("", "Arial", 11);
	m_hudTickerLabel->setAnchorPoint(Vec2(0, 0.5f));
	m_hudTickerLabel->setColor(Color3B(80, 220, 180));
	m_hudTickerLabel->setPosition(Vec2(RESOURCE_WIDTH + 5, RESOURCE_HEIGHT - 13));
	this->addChild(m_hudTickerLabel, tagInfoText);

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
	this->DrawMenu(soundOpt);
	this->InitGame();
	this->ResetGame();
	this->DrawDiscus();
	this->DrawInfoText();
	if (m_labelLevel) m_labelLevel->setVisible(false);
	this->startRankTicker(numOfDiscus);
	if (m_countOfDiscus == 3)
		this->startGuideAnimation();
	else
		this->startIdleAnimation();


	if (m_countOfDiscus == 3)
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
			MenuItemImage* pLockMenu = MenuItemImage::create("NewUI/lock_icon.png", "NewUI/lock_icon_s.png", CC_CALLBACK_1(PlayScene::callbackLockBtn, this));
			Menu* pMenu = Menu::create(pLockMenu, NULL);
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
	SoundFactory::Instance()->stop("BGM2");
	m_pole.clear();
	m_arrDiscurs.clear();	
}



void	PlayScene::DrawMenu(bool SoundOpt)
{
	
	this->removeChildByTag(1234, false);
	
	MenuItemImage* homeMenuItem = MenuItemImage::create("NewUI/btn_home_n.png", "NewUI/btn_home_s.png", CC_CALLBACK_1(PlayScene::callbackOnPushed_homeMenuItem, this));	
	homeMenuItem->setScale(1.0f);

	float scaleForSubUI = 0.7f;
	MenuItemImage* resetMenuItem = MenuItemImage::create("NewUI/btn_refresh_n.png", "NewUI/btn_refresh_s.png", CC_CALLBACK_1(PlayScene::callbackOnPushed_resetMenuItem, this));
	resetMenuItem->setScale(scaleForSubUI);
	MenuItemImage* prevMenuItem = MenuItemImage::create("NewUI/btn_prev.png", "NewUI/btn_prev.png", CC_CALLBACK_1(PlayScene::callbackOnPushed_prevMenuItem, this));
	prevMenuItem->setScale(scaleForSubUI);
	MenuItemImage* nextMenuItem = MenuItemImage::create("NewUI/btn_next.png", "NewUI/btn_next.png", CC_CALLBACK_1(PlayScene::callbackOnPushed_nextMenuItem, this));
	nextMenuItem->setScale(scaleForSubUI);
	
	
	std::string iconName;
	if( false == SoundOpt )
	{
		iconName = "NewUI/btn_speaker_off.png";		
	}
	else 
	{
		iconName = "NewUI/btn_speaker_on.png";
	}
	
	MenuItemImage* speakerMenuItem = MenuItemImage::create(iconName, iconName, CC_CALLBACK_1(PlayScene::callbackOnPushed_speakerMenuItem, this));
	speakerMenuItem->setScale(0.7f);
	
	
	Menu* mainMenuTop = Menu::create(homeMenuItem, resetMenuItem, nextMenuItem, prevMenuItem, speakerMenuItem, NULL) ;	
	mainMenuTop->alignItemsVerticallyWithPadding(12);
	mainMenuTop->setPosition(Vec2(20, 200));
	this->addChild(mainMenuTop, 1234, 1234);
}

void	PlayScene::InitGame()
{	
	this->removeChildByTag(tagPopup, false);
	m_isIng = NONE ;
	
	
	
	m_countDown=3;
	m_labelTime->setString("00:00.00");

}


PLAY_STATE PlayScene::GetPlayState()
{
	return m_isIng;
}

void PlayScene::Start()
{
	m_isIng = PLAY;
	stopRankTicker();
	if (m_labelLevel) m_labelLevel->setVisible(true);
	if (m_hudSep)  m_hudSep->setVisible(true);
	if (m_labelTime) m_labelTime->setVisible(true);
	// GO! 패널 잠깐 표시 후 응원 연출 시작
	scheduleOnce([this](float){ startCheerAnimation(); }, 0.7f, "cheer_start");

	SoundFactory::Instance()->play("FX0070", 0.4) ;
	bool bSoundOpt = UserDataManager::Instance()->GetSoundOpt();
	if( true == bSoundOpt )
	{
		SoundFactory::Instance()->play("BGM2", true, true);		
	}
	else 
	{
		SoundFactory::Instance()->play("BGM2", false, true);		
	}
	
	
	
	m_dateTime = getMilliCount();
	
	
	
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
	unschedule("cheer_anim");
	unschedule("cheer_start");
	clearBottomPanels();
	this->stopAction(m_actionTimeRun);
	int elapsedTime = getMilliCount() - m_dateTime;
	SoundFactory::Instance()->stop("BGM2");

	m_mastTime = elapsedTime;
	RecordTime rt = getRecordTime(m_mastTime);

	// HUD: 타이머/구분선 숨기고 결과 텍스트 표시
	if (m_labelTime) m_labelTime->setVisible(false);
	if (m_hudSep)    m_hudSep->setVisible(false);
	int bestRecord = UserDataManager::Instance()->GetBestRecord(m_countOfDiscus);
	bool isNewRecord = (bestRecord == 0 || bestRecord > m_mastTime);
	showHudResult(isNewRecord, rt);

	DelayTime* delay = DelayTime::create(2.0);
	std::function<void()> func = std::bind(&PlayScene::MessagePopup, this);
	CallFunc* callFucn = CallFunc::create(func);
	this->runAction(Sequence::create(delay, callFucn, NULL));
}


void PlayScene::MessagePopup()
{
	m_popupShownTime = getMilliCount();
	SoundFactory::Instance()->play("drop_coin");

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
		if (!m_isFirstPlay)
			LeaderboardManager::Instance()->submitScore(m_countOfDiscus, m_mastTime);
		isNewRecord = true;
	}

	// 딤 오버레이
	auto overlay = LayerColor::create(Color4B(0, 0, 0, 0), RESOURCE_WIDTH, RESOURCE_HEIGHT);
	overlay->setPosition(Vec2::ZERO);
	this->addChild(overlay, tagPopup, tagPopup);
	overlay->runAction(FadeTo::create(0.25f, 160));

	// 팝업 박스
	const float PW = 280, PH = 150;
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
	titleLabel->setPosition(Vec2(PW / 2, PH - 30));
	popupBox->addChild(titleLabel);

	// 구분선
	auto divider = DrawNode::create();
	divider->drawLine(Vec2(20, PH - 50), Vec2(PW - 20, PH - 50), Color4F(0.5f, 0.5f, 0.5f, 0.8f));
	popupBox->addChild(divider);

	// 기록 시간
	std::string timeStr = StringUtils::format("%02d:%02d.%02d", recordTime.min, recordTime.sec, recordTime.ms);
	auto timeLabel = Label::createWithSystemFont(timeStr, "Arial", 26);
	timeLabel->setColor(Color3B::WHITE);
	timeLabel->setPosition(Vec2(PW / 2, PH - 82));
	popupBox->addChild(timeLabel);

	// 레벨 정보
	auto levelLabel = Label::createWithSystemFont(
		StringUtils::format("LEVEL %d", m_countOfDiscus), "Arial", 11);
	levelLabel->setColor(Color3B(160, 160, 160));
	levelLabel->setPosition(Vec2(PW / 2, PH - 108));
	popupBox->addChild(levelLabel);

	// 힌트 (깜빡임)
	std::string hintStr = m_isFirstPlay ? "TAP TO CONTINUE" : "TAP TO PLAY AGAIN";
	auto hintLabel = Label::createWithSystemFont(hintStr, "Arial", 11);
	hintLabel->setColor(Color3B(200, 200, 100));
	hintLabel->setPosition(Vec2(PW / 2, 18));
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
		SoundFactory::Instance()->play("FX0066", 0.3);		
		MainScene* mainScene = MainScene::createScene();
		Director::getInstance()->replaceScene(TransitionFade::create(0.2, mainScene));
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
		SoundFactory::Instance()->play("levelup");
	}	
}


void PlayScene::DrawInfoText ()
{
	
	std::string strLevelInfo = StringUtils::format("LV.%d / %d", m_countOfDiscus, MAX_PLAY_LEVEL);
	if (NULL == m_labelLevel)
	{
		m_labelLevel = Label::createWithSystemFont(strLevelInfo, "Arial", 13);
		m_labelLevel->setColor(Color3B(255, 215, 80));
		m_labelLevel->setPosition(Vec2(120, RESOURCE_HEIGHT - 13));
		this->addChild(m_labelLevel, tagInfoText, tagInfoText);
	}
	else
	{
		m_labelLevel->setString(strLevelInfo);
	}
	
}


void PlayScene::CountDown()
{
	this->removeChildByTag(tagCountDown, true);  // arrow/rules 이미지 제거
	if( m_isIng == NONE )
	{
		stopIdleAnimation();
		stopRankTicker();
		if (m_labelLevel) m_labelLevel->setVisible(true);
		if (m_hudSep)     m_hudSep->setVisible(true);
		if (m_labelTime)  m_labelTime->setVisible(true);
		m_isIng = COUNT_DOWN;

		DelayTime* readyDelay = DelayTime::create(2.0f);
		DelayTime* countDelay = DelayTime::create(1.0f);

		std::function<void()> func_CountDown = std::bind(&PlayScene::CountDown, this);
		CallFunc* callFunc_countDown = CallFunc::create(func_CountDown);
		Sequence* actionToContDown = Sequence::create(callFunc_countDown, countDelay, NULL);

		Repeat* countDown = Repeat::create(actionToContDown, m_countDown + 1);
		std::function<void()> func_Start = std::bind(&PlayScene::Start, this);
		CallFunc* callFunc_start = CallFunc::create(func_Start);

		// GO! 0.5초 표시 후 Start 호출
		DelayTime* delayStart = DelayTime::create(m_countDown + 0.5f);

		this->runAction(Sequence::create(readyDelay, countDown, NULL));
		this->runAction(Sequence::create(readyDelay, delayStart, callFunc_start, NULL));
		SoundFactory::Instance()->play("FX0145");
	}
	else
	{
		if( m_countDown == 0 )
		{
			// GO! → A·B·C 동시 표시
			SoundFactory::Instance()->play("go");
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
			// 3 → Panel A(red), 2 → Panel B(orange), 1 → Panel C(gold)
			SoundFactory::Instance()->play("count_sec");
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
	if( poleID > -1 && poleID < 3)
	{
		if( bIsAble )
		{
			if (m_selectionMark)   m_selectionMark->setPosition(Vec2(arrPosOfPole[poleID].x, 110));
			if (m_deselectionMark) m_deselectionMark->setPosition(Vec2(500,500));
			SoundFactory::Instance()->play("select");
		}
		else
		{
			if (m_deselectionMark) m_deselectionMark->setPosition(Vec2(arrPosOfPole[poleID].x, 110));
			if (m_selectionMark)   m_selectionMark->setPosition(Vec2(500,500));
			SoundFactory::Instance()->play("deselect") ;
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

void PlayScene::callbackOnPushed_homeMenuItem(Ref* pSender)
{
	if (m_isTransitioning) return;
	m_isTransitioning = true;

#ifdef LITE_VER
	CMKStoreManager::Instance()->SetDelegate(NULL);
#endif //LITE_VER

	SoundFactory::Instance()->play("FX0066", 0.3) ;

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
	SoundFactory::Instance()->play("FX0070", 0.4) ;
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
		SoundFactory::Instance()->play("FX0070", 0.4) ;
		m_countOfDiscus = m_countOfDiscus-1;
		PlayScene* playScene = PlayScene::createScene(m_countOfDiscus) ;
		auto director = Director::getInstance();
		director->replaceScene(TransitionFade::create(0.2, playScene));
	}
	else
	{
		SoundFactory::Instance()->play("Cancel", 0.4);
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
		SoundFactory::Instance()->play("FX0070", 0.4);
		m_countOfDiscus = m_countOfDiscus+1;

		PlayScene* playScene = PlayScene::createScene(m_countOfDiscus);
		auto director = Director::getInstance();
		director->replaceScene(TransitionFade::create(0.2, playScene));
	}
	else
	{
		SoundFactory::Instance()->play("Cancel", 0.4);
	}
}


void PlayScene::callbackOnPushed_speakerMenuItem(Ref* sender)
{
	SoundFactory::Instance()->play("FX0070", 0.4);	
	if( false == UserDataManager::Instance()->GetSoundOpt())
	{		
		UserDataManager::Instance()->SetSoundOpt(true);
		this->DrawMenu(true);
		if (m_isIng == PLAY)
		{
			SoundFactory::Instance()->play("BGM2", true, true);
		}
	}
	else 
	{
		UserDataManager::Instance()->SetSoundOpt(false);
		this->DrawMenu(false);
		if (m_isIng == PLAY)
		{
			SoundFactory::Instance()->play("BGM2", false, true);
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

	SoundFactory::Instance()->play("FX0070", 0.4);
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
		SoundFactory::Instance()->play("drop_coin");
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
	SoundFactory::Instance()->play("FX0070", 0.4);

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
void PlayScene::typingBottomPanel(int pole, const std::string& text, float charInterval, Color3B color, float fontSize)
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
void PlayScene::slideInBottomPanel(int pole, const std::string& text, bool fromLeft, Color3B color, float fontSize)
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
