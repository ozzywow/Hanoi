#include "stdafx.h"
#include <algorithm>
#include "MainScene.h"
#include "SoundFactory.h"
#include "PlayScene.h"
#include "UserDataManager.h"
#include "LeaderboardManager.h"
#include "ui/CocosGUI.h"
#ifdef LITE_VER
#include "MKStoreManager_cpp.h"
#endif //LITE_VER

MainScene::MainScene()
{
	m_rankTableLayer = NULL;
	m_rankBG = NULL;
}

MainScene::~MainScene()
{
}

bool MainScene::init()
{
	if (!Scene::init())
	{
		return false;
	}

	this->isProgress = false;
	this->isRestored = false;
	m_aliveFlag = std::make_shared<bool>(true);

	Sprite* backGround = Sprite::create("NewUI/title_bg.png") ;
	if (backGround)
	{
		backGround->setAnchorPoint(Point::ZERO);
		this->addChild(backGround, tagBG, tagBG);
	}
	

	MenuItemImage* startMenuItem = MenuItemImage::create("NewUI/btn_start_n.png", "NewUI/btn_start_s.png", CC_CALLBACK_1(MainScene::callbackOnPushed_startMenuItem, this));
	startMenuItem->setScale(0.85f);

	Menu* mainMenu = Menu::create(startMenuItem, NULL);
	mainMenu->setAnchorPoint(Point::ZERO);
	mainMenu->setPosition(90, 150);
	this->addChild(mainMenu, tagInfoText, tagInfoText);

	std::string name = UserDataManager::Instance()->GetUserName();

	// LED 전광판 상수 (RESET 위치 계산에도 사용)
	const Vec2 LED_POS(90, 205);
	const float LP_W = 128, LP_H = 16;

	auto resetLabel = Label::createWithSystemFont("RESET", "Arial", 10);
	resetLabel->setColor(Color3B(130, 130, 130));
	auto resetMenuItem = MenuItemLabel::create(resetLabel,
		CC_CALLBACK_1(MainScene::callbackOnPushed_resetMenuItem, this));
	Menu* resetMenu = Menu::create(resetMenuItem, NULL);
	resetMenu->setPosition(Vec2(LED_POS.x, 20)); // 패널 중앙 정렬
	this->addChild(resetMenu, tagInfoText, tagInfoText);
	// 1. 패널 배경 + 디스플레이 프레임
	auto ledBg = DrawNode::create();
	setupLedBackground(ledBg, LP_W, LP_H, 6.0f);
	ledBg->setPosition(LED_POS);
	this->addChild(ledBg, 1);

	// 2. 이름/인삿말 전광판 라벨
	m_playerDisplayName = name.empty() ? "PLAYER" : name;
	m_nameplateLabel = Label::createWithSystemFont(m_playerDisplayName, "Arial", 16);
	m_nameplateLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
	m_nameplateLabel->setColor(Color3B(255, 215, 0));
	m_nameplateLabel->enableOutline(Color4B(0, 0, 0, 200), 1);
	m_nameplateLabel->setPosition(LED_POS);
	this->addChild(m_nameplateLabel, 2);

	// 디바이스 언어 감지 → 자국어 인삿말 우선 배치
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

		std::string nativeText;
		auto data = FileUtils::getInstance()->getValueVectorFromFile("greetings.plist");
		for (auto& v : data) {
			auto& d = v.asValueMap();
			if (d["lang"].asString() == nativeLang)
				nativeText = d["text"].asString();
			else
				m_greetingTexts.push_back(d["text"].asString());
		}
		// 자국어를 맨 앞에 삽입, 없으면 "Hello" 기본값
		if (nativeText.empty()) nativeText = "Hello";
		m_greetingTexts.insert(m_greetingTexts.begin(), nativeText);

		// [1..n] 랜덤 셔플 — 자국어(0번)는 고정
		if (m_greetingTexts.size() > 1)
			std::random_shuffle(m_greetingTexts.begin() + 1, m_greetingTexts.end());
	}

	m_greetingIndex     = ((int)m_greetingTexts.size() > 1) ? 1 : 0;
	m_showingPlayerName = false;
	m_showNative        = true;
	showNextNameplateText();

	// 3. LED 도트 그리드
	auto dimDots = DrawNode::create();
	setupLedDotOverlay(dimDots, LP_W, LP_H);
	dimDots->setPosition(LED_POS);
	this->addChild(dimDots, 3);

	
 
	
#ifdef LITE_VER
	if (false == UserDataManager::Instance()->GetCart())
	{
		cocos2d::Node* buyRestoreNode = cocos2d::Node::create();
		buyRestoreNode->setPosition(cocos2d::Vec2(0, 0));
		this->addChild(buyRestoreNode, tagCart, tagCart);

		const float CART_X = 48, LOCK_X = 118;
		const float ICON_Y = 78, LABEL_Y = 52;

		MenuItemImage* cartMenuItem = MenuItemImage::create("NewUI/btn_cart.png", "NewUI/btn_cart_s.png", CC_CALLBACK_1(MainScene::callbackOnPushed_buyMenuItem, this));
		cartMenuItem->setScale(0.75f);
		cartMenuItem->setPosition(Vec2(CART_X, ICON_Y));

		MenuItemImage* pLockMenu = MenuItemImage::create("NewUI/lock_icon.png", "NewUI/lock_icon_s.png", CC_CALLBACK_1(MainScene::callbackLockBtn, this));
		pLockMenu->setScale(0.52f);
		pLockMenu->setPosition(Vec2(LOCK_X, ICON_Y));

		Menu* subMenu = Menu::create(cartMenuItem, pLockMenu, NULL);
		subMenu->setPosition(Vec2(0, 0));
		buyRestoreNode->addChild(subMenu);

		cocos2d::Label* labelBuy = cocos2d::Label::createWithSystemFont("BUY", "Arial", 10);
		labelBuy->setColor(Color3B(160, 160, 160));
		labelBuy->setPosition(Vec2(CART_X, LABEL_Y));
		buyRestoreNode->addChild(labelBuy);

		cocos2d::Label* labelRestore = cocos2d::Label::createWithSystemFont("RESTORE", "Arial", 10);
		labelRestore->setColor(Color3B(160, 160, 160));
		labelRestore->setPosition(Vec2(LOCK_X, LABEL_Y));
		buyRestoreNode->addChild(labelRestore);
	}	
	CMKStoreManager::Instance()->SetDelegate(this);
#endif //LITE_VER
	

	m_rankTableLayer = Layer::create();
	this->addChild(m_rankTableLayer, tagInfoText, tagInfoText);

	if (UserDataManager::Instance()->GetUserName().empty()) {
		if (!UserDataManager::Instance()->HasPendingSubmit()) {
			// 완전 신규 유저 — 첫판 플레이 후 이름 입력
			scheduleOnce([](float) {
				Director::getInstance()->replaceScene(
					TransitionFade::create(0.3f, PlayScene::createScene(3, true)));
			}, 0.0f, "firstPlay");
			return true;
		}
		// 첫판 완료, 이름 미입력 — 이름 입력창 표시
		showNameInputDialog();
	}

	LeaderboardManager::Instance()->login([](bool ok) {
		log("PlayFab startup login: %s", ok ? "OK" : "FAIL");
	});

	int level = UserDataManager::Instance()->GetLevel();
	m_rankLevel = level;
	SoundFactory::Instance()->play("FX0108");
	this->drawOnlineRank(level);

	// 방금 이름 등록한 경우 PlayFab 전파 지연 보상을 위해 2초 후 재갱신
	if (UserDataManager::Instance()->m_justRegistered) {
		UserDataManager::Instance()->m_justRegistered = false;
		int lv = level;
		scheduleOnce([this, lv](float) { drawOnlineRank(lv); }, 2.0f, "refreshRank");
	}

	return true;
}


void MainScene::onExitTransitionDidStart()
{
	if (m_aliveFlag) *m_aliveFlag = false;

#ifdef LITE_VER
	this->isProgress = false;
	this->isRestored = false;
	CMKStoreManager::Instance()->ToggleIndicator(false);
#endif //LITE_VER
}


void MainScene::callbackOnPushed_resetMenuItem(Ref* pSender)
{
	const int CONFIRM_TAG = 198;
	if (this->getChildByTag(CONFIRM_TAG)) return;

	const float DW = 220, DH = 110;

	auto backdrop = LayerColor::create(Color4B(0, 0, 0, 0));
	backdrop->setTag(CONFIRM_TAG);
	this->addChild(backdrop, 999);
	backdrop->runAction(FadeTo::create(0.2f, 160));

	auto dlg = LayerColor::create(Color4B(10, 15, 50, 230), DW, DH);
	dlg->setPosition(Vec2((RESOURCE_WIDTH - DW) / 2, (RESOURCE_HEIGHT - DH) / 2));
	dlg->setScale(0.7f);
	backdrop->addChild(dlg);
	dlg->runAction(Sequence::create(
		ScaleTo::create(0.15f, 1.05f),
		ScaleTo::create(0.08f, 1.0f),
		nullptr
	));

	auto titleLabel = Label::createWithSystemFont("RESET ALL?", "Arial", 15);
	titleLabel->setColor(Color3B(255, 100, 80));
	titleLabel->setPosition(Vec2(DW / 2, DH - 22));
	dlg->addChild(titleLabel);

	auto divider = DrawNode::create();
	divider->drawLine(Vec2(15, DH - 40), Vec2(DW - 15, DH - 40), Color4F(0.5f, 0.5f, 0.5f, 0.8f));
	dlg->addChild(divider);

	auto msg = Label::createWithSystemFont("Reset all records?", "Arial", 12);
	msg->setColor(Color3B(200, 200, 200));
	msg->setPosition(Vec2(DW / 2, DH - 65));
	dlg->addChild(msg);

	auto doReset = [this]() {
		auto* ud = UserDataManager::Instance();
		ud->ResetRecords();
		std::string empty;
		ud->SetUserName(empty);
		ud->SaveUserData();
		LeaderboardManager::Instance()->resetStats();

		if (m_rankBG)
		{
			m_rankBG->removeAllChildren();
			for (int level = 3; level <= MAX_PLAY_LEVEL; ++level)
			{
				int record = ud->GetBestRecord(level);
				RecordTime rt = getRecordTime(record);
				std::string str = StringUtils::format("%02d                                         %02d:%02d.%02d",
					level, rt.min, rt.sec, rt.ms);
				Label* lbl = Label::createWithSystemFont(str, "Arial", 14);
				lbl->setAnchorPoint(Vec2(0, 0));
				lbl->setPosition(Vec2(40, (level * 22) - 20));
				m_rankBG->addChild(lbl);
			}
		}
		showNameInputDialog();
	};

	auto okLabel = Label::createWithSystemFont("OK", "Arial", 14);
	okLabel->setColor(Color3B(255, 80, 80));
	auto okBtn = MenuItemLabel::create(okLabel, [this, doReset](Ref*) {
		this->removeChildByTag(198);
		doReset();
	});
	okBtn->setPosition(Vec2(DW * 0.3f, 22));

	auto cancelLabel = Label::createWithSystemFont("Cancel", "Arial", 14);
	cancelLabel->setColor(Color3B(180, 180, 180));
	auto cancelBtn = MenuItemLabel::create(cancelLabel, [this](Ref*) {
		this->removeChildByTag(198);
	});
	cancelBtn->setPosition(Vec2(DW * 0.7f, 22));

	auto menu = Menu::create(okBtn, cancelBtn, nullptr);
	menu->setPosition(Vec2::ZERO);
	dlg->addChild(menu);
}

void MainScene::callbackOnPushed_startMenuItem(Ref* pSender)
{

	SoundFactory::Instance()->play("FX0066");

	if (m_rankBG)
	{
		auto action = MoveTo::create(0.3, Vec2(600, 320 - 60));
		m_rankBG->runAction(action);
		SoundFactory::Instance()->play("FX0108");
	}


	int level = m_rankLevel;
	PlayScene* playScene = PlayScene::createScene(level);		
	Director::getInstance()->replaceScene(TransitionFade::create(0.3, playScene));
}


void MainScene::callbackOnPushed_buyMenuItem(Ref* pSender)
{
#ifdef LITE_VER
	if (true == isProgress) { return; }
	isProgress = true;
	CMKStoreManager::Instance()->ToggleIndicator(true);
	CMKStoreManager::Instance()->buyFeature(kProductIdTotal);
#endif //LITE_VER

	SoundFactory::Instance()->play("FX0070", 0.4);
}




void MainScene::callbackLockBtn(Ref* sender)
{
#ifdef LITE_VER
	if (isRestored) { return; }
	if (true == isProgress) { return; }
	isProgress = true;	

	CMKStoreManager::Instance()->ToggleIndicator(true);
	CMKStoreManager::Instance()->restorePreviousTransactions();
#endif //LITE_VER

	SoundFactory::Instance()->play("FX0070", 0.4);
}



#ifdef LITE_VER
void MainScene::productFetchComplete()
{
	cocos2d::log("productFetchComplete");
	CMKStoreManager::Instance()->ToggleIndicator(false);
	isProgress = false;	
}
void MainScene::productPurchased(std::string productId)
{
	cocos2d::log("productPurchased /%s", productId.c_str());
	CMKStoreManager::Instance()->ToggleIndicator(false);
	isProgress = false;

	if (productId == kProductIdTotal)
	{
		UserDataManager::Instance()->SetCart(true);
		UserDataManager::Instance()->SaveUserData();
		this->removeChildByTag(tagCart);

		SoundFactory::Instance()->play("drop_coin");
		showResultDialog("PURCHASE COMPLETE", Color3B(255, 215, 0), "All levels unlocked!");
	}
}
void MainScene::transactionCanceled()
{
	cocos2d::log("transactionCanceled");
	CMKStoreManager::Instance()->ToggleIndicator(false);
	isProgress = false;	
}

void MainScene::restorePreviousTransactions(int count)
{
	if (true == isRestored) { return; }

	isRestored = true;
	isProgress = false;
	this->removeChildByTag(tagPopup);

	if (count == 0)
	{
		showResultDialog("NOTHING TO RESTORE", Color3B(150, 150, 150), "No previous purchase found.");
		return;
	}

	cocos2d::log("restorePreviousTransactions");

	UserDataManager::Instance()->SetCart(true);
	UserDataManager::Instance()->SaveUserData();
	CMKStoreManager::Instance()->ToggleIndicator(false);
	this->removeChildByTag(tagCart);

	SoundFactory::Instance()->play("FX0070", 0.4);
	showResultDialog("RESTORE COMPLETE", Color3B(80, 220, 180), "All levels restored!");
}
#endif //LITE_VER


void MainScene::callbackRankPrev(Ref* pSender)
{
	if (m_rankLevel <= 3) return;
	SoundFactory::Instance()->play("FX0070", 0.4);
	--m_rankLevel;
	int lv = m_rankLevel;
	scheduleOnce([this, lv](float) { drawOnlineRank(lv); }, 0.0f, "rankPrev");
}

void MainScene::callbackRankNext(Ref* pSender)
{
	if (m_rankLevel >= MAX_PLAY_LEVEL) return;
	SoundFactory::Instance()->play("FX0070", 0.4);
	++m_rankLevel;
	int lv = m_rankLevel;
	scheduleOnce([this, lv](float) { drawOnlineRank(lv); }, 0.0f, "rankNext");
}

void MainScene::drawOnlineRank(int level, bool retryOnEmpty)
{
	this->unschedule("rankRetry");

	if (m_rankBG)
	{
		m_rankBG->setVisible(false);
		m_rankBG = nullptr;
	}

	m_rankTableLayer->removeAllChildren();

	const int TAG_LOADING = 50;
	const float PX = 178, PY = 8;
	const float PW = 292, PH = 210;   // top=223 → 타이틀과 간격 확보

	Label* loading = Label::createWithSystemFont("Loading...", "Arial", 14);
	loading->setColor(Color3B(180, 180, 180));
	loading->setPosition(Vec2(PX + PW / 2, PY + PH / 2));
	m_rankTableLayer->addChild(loading, 0, TAG_LOADING);

	auto alive = m_aliveFlag;
	LeaderboardManager::Instance()->fetchLeaderboard(level, 10,
		[this, alive, level, retryOnEmpty, PX, PY, PW, PH](const std::vector<LeaderboardEntry>& entries) {
			if (!alive || !*alive) return;

			const int TAG_LOADING = 50;
			m_rankTableLayer->removeChildByTag(TAG_LOADING);
			m_rankTableLayer->removeChildByTag(tagBG);

			auto panel = LayerColor::create(Color4B(10, 15, 50, 230), PW, PH);
			panel->setPosition(Vec2(RESOURCE_WIDTH + PW, PY));
			panel->runAction(MoveTo::create(0.3f, Vec2(PX, PY)));

			auto outline = DrawNode::create();
			// 외곽 글로우
			outline->drawRect(Vec2(-3, -3), Vec2(PW + 3, PH + 3), Color4F(0.4f, 0.6f, 1.0f, 0.15f));
			outline->drawRect(Vec2(-2, -2), Vec2(PW + 2, PH + 2), Color4F(0.5f, 0.7f, 1.0f, 0.30f));
			// 메인 보더 3px 두께
			outline->drawRect(Vec2(-1, -1), Vec2(PW + 1, PH + 1), Color4F(0.75f, 0.85f, 1.0f, 0.85f));
			outline->drawRect(Vec2(0, 0),   Vec2(PW, PH),          Color4F(0.80f, 0.90f, 1.0f, 1.0f));
			outline->drawRect(Vec2(1, 1),   Vec2(PW - 1, PH - 1),  Color4F(0.75f, 0.85f, 1.0f, 0.85f));
			// 내부 액센트
			outline->drawRect(Vec2(3, 3), Vec2(PW - 3, PH - 3), Color4F(0.4f, 0.55f, 0.95f, 0.35f));
			panel->addChild(outline);

			m_rankBG = panel;
			m_rankTableLayer->addChild(panel, tagBGRankingBoard, tagBGRankingBoard);

			// nav 버튼: ◀ ▶ 심볼 레이블 (LEVEL X 좌우)
			bool canPrev = (level > 3);
			bool canNext = (level < MAX_PLAY_LEVEL);

			auto prevLbl = Label::createWithSystemFont(u8"◀◀", "Arial", 20);
			prevLbl->setColor(canPrev ? Color3B(255, 215, 0) : Color3B(70, 70, 70));
			auto prevBtn = MenuItemLabel::create(prevLbl,
				CC_CALLBACK_1(MainScene::callbackRankPrev, this));
			prevBtn->setEnabled(canPrev);

			auto nextLbl = Label::createWithSystemFont(u8"▶▶", "Arial", 20);
			nextLbl->setColor(canNext ? Color3B(255, 215, 0) : Color3B(70, 70, 70));
			auto nextBtn = MenuItemLabel::create(nextLbl,
				CC_CALLBACK_1(MainScene::callbackRankNext, this));
			nextBtn->setEnabled(canNext);

			auto navMenu = Menu::create(prevBtn, nextBtn, NULL);
			navMenu->setPosition(Vec2::ZERO);
			prevBtn->setPosition(Vec2(60, PH - 14));
			nextBtn->setPosition(Vec2(PW - 60, PH - 14));
			panel->addChild(navMenu);

			// 헤더: 레벨 타이틀 (nav 버튼 사이)
			Label* titleLabel = Label::createWithSystemFont(
				StringUtils::format("LEVEL  %d", level), "Arial", 15);
			titleLabel->setColor(Color3B(255, 215, 0));
			titleLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
			titleLabel->setPosition(Vec2(PW / 2, PH - 14));
			panel->addChild(titleLabel);

			Label* subTitle = Label::createWithSystemFont("ONLINE  RANK", "Arial", 9);
			subTitle->setColor(Color3B(160, 160, 200));
			subTitle->setAnchorPoint(Vec2(0.5f, 0.5f));
			subTitle->setPosition(Vec2(PW / 2, PH - 26));
			panel->addChild(subTitle);

			auto hDiv = DrawNode::create();
			hDiv->drawLine(Vec2(10, PH - 35), Vec2(PW - 10, PH - 35),
				Color4F(0.5f, 0.5f, 0.5f, 0.8f));
			panel->addChild(hDiv);

			// 컬럼 헤더
			auto makeColHeader = [&](const char* text, float x) {
				Label* lbl = Label::createWithSystemFont(text, "Arial", 9);
				lbl->setColor(Color3B(130, 130, 150));
				lbl->setAnchorPoint(Vec2(0, 0.5f));
				lbl->setPosition(Vec2(x, PH - 45));
				panel->addChild(lbl);
			};
			makeColHeader("#",    12);
			makeColHeader("NAME", 50);
			makeColHeader("TIME", 216);

			auto colDiv = DrawNode::create();
			colDiv->drawLine(Vec2(10, PH - 52), Vec2(PW - 10, PH - 52),
				Color4F(0.3f, 0.3f, 0.4f, 0.6f));
			panel->addChild(colDiv);

			if (entries.empty()) {
				const char* msg = retryOnEmpty ? "Refreshing..." : "No records yet";
				Label* emptyLabel = Label::createWithSystemFont(msg, "Arial", 12);
				emptyLabel->setColor(Color3B(180, 180, 180));
				emptyLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
				emptyLabel->setPosition(Vec2(PW / 2, PH / 2 - 15));
				panel->addChild(emptyLabel);
				if (retryOnEmpty) {
					this->scheduleOnce([this, alive, level](float) {
						if (!alive || !*alive) return;
						drawOnlineRank(level, false);
					}, 2.0f, "rankRetry");
				}
				return;
			}

			const std::string myId = LeaderboardManager::Instance()->getPlayFabId();
			const float FIRST_ROW_Y = PH - 62;
			const float ROW_STEP = 15;

			for (int i = 0; i < (int)entries.size(); ++i) {
				const auto& e = entries[i];
				RecordTime rt = getRecordTime(e.scoreMs);
				float y = FIRST_ROW_Y - i * ROW_STEP;
				bool isMe = !myId.empty() && (e.playFabId == myId);
				Color3B rowColor = isMe ? Color3B(255, 215, 0) : Color3B::WHITE;
				int rowFont = isMe ? 13 : 12;

				Label* rankLabel = Label::createWithSystemFont(
					StringUtils::format("%d", e.rank), "Arial", rowFont);
				rankLabel->setAnchorPoint(Vec2(0, 0.5f));
				rankLabel->setPosition(Vec2(12, y));
				rankLabel->setColor(rowColor);
				panel->addChild(rankLabel);

				if (!e.countryCode.empty()) {
					auto flagLabel = Label::createWithSystemFont(
						countryToFlag(e.countryCode), "Arial", rowFont + 1);
					flagLabel->setAnchorPoint(Vec2(0, 0.5f));
					flagLabel->setPosition(Vec2(32, y));
					panel->addChild(flagLabel);
				}

				Label* nameLabel = Label::createWithSystemFont(
					e.displayName.substr(0, 10), "Arial", rowFont);
				nameLabel->setAnchorPoint(Vec2(0, 0.5f));
				nameLabel->setPosition(Vec2(55, y));
				nameLabel->setColor(rowColor);
				panel->addChild(nameLabel);

				Label* timeLabel = Label::createWithSystemFont(
					StringUtils::format("%02d:%02d.%02d", rt.min, rt.sec, rt.ms), "Arial", rowFont);
				timeLabel->setAnchorPoint(Vec2(0, 0.5f));
				timeLabel->setPosition(Vec2(216, y));
				timeLabel->setColor(rowColor);
				panel->addChild(timeLabel);
			}
		});
}

void MainScene::showResultDialog(const std::string& title, Color3B titleColor, const std::string& msg)
{
	const int TAG = 197;
	this->removeChildByTag(TAG);

	const float DW = 220, DH = 110;

	auto backdrop = LayerColor::create(Color4B(0, 0, 0, 0));
	backdrop->setTag(TAG);
	this->addChild(backdrop, 999);
	backdrop->runAction(FadeTo::create(0.15f, 150));

	auto dlg = LayerColor::create(Color4B(10, 15, 50, 230), DW, DH);
	dlg->setPosition(Vec2((RESOURCE_WIDTH - DW) / 2, (RESOURCE_HEIGHT - DH) / 2));
	dlg->setScale(0.7f);
	backdrop->addChild(dlg);
	dlg->runAction(Sequence::create(
		ScaleTo::create(0.15f, 1.05f),
		ScaleTo::create(0.08f, 1.0f),
		nullptr
	));

	auto outline = DrawNode::create();
	outline->drawRect(Vec2(-1, -1), Vec2(DW + 1, DH + 1), Color4F(0.75f, 0.85f, 1.0f, 0.85f));
	outline->drawRect(Vec2(0, 0),   Vec2(DW, DH),          Color4F(0.80f, 0.90f, 1.0f, 1.0f));
	outline->drawRect(Vec2(1, 1),   Vec2(DW - 1, DH - 1),  Color4F(0.75f, 0.85f, 1.0f, 0.85f));
	dlg->addChild(outline);

	auto titleLabel = Label::createWithSystemFont(title, "Arial", 14);
	titleLabel->setColor(titleColor);
	titleLabel->setPosition(Vec2(DW / 2, DH - 22));
	dlg->addChild(titleLabel);

	auto divider = DrawNode::create();
	divider->drawLine(Vec2(15, DH - 40), Vec2(DW - 15, DH - 40), Color4F(0.5f, 0.5f, 0.5f, 0.8f));
	dlg->addChild(divider);

	auto msgLabel = Label::createWithSystemFont(msg, "Arial", 12);
	msgLabel->setColor(Color3B(200, 200, 200));
	msgLabel->setPosition(Vec2(DW / 2, DH - 65));
	dlg->addChild(msgLabel);

	auto okLabel = Label::createWithSystemFont("  OK  ", "Arial", 13);
	okLabel->setColor(Color3B(255, 215, 0));
	auto okBtn = MenuItemLabel::create(okLabel, [this, TAG](Ref*) {
		this->removeChildByTag(TAG);
	});
	okBtn->setPosition(Vec2(DW / 2, 18));

	auto menu = Menu::create(okBtn, nullptr);
	menu->setPosition(Vec2::ZERO);
	dlg->addChild(menu);

	dlg->scheduleOnce([this, TAG](float) {
		this->removeChildByTag(TAG);
	}, 3.0f, "autoClose");
}

void MainScene::showNextNameplateText()
{
	if (!m_nameplateLabel || m_greetingTexts.empty()) return;

	std::string text;
	Color3B     color;

	if (!m_showingPlayerName) {
		// 인삿말 슬롯: 자국어↔랜덤 교대
		color = Color3B(160, 220, 255);
		if (m_showNative || (int)m_greetingTexts.size() <= 1) {
			// 자국어
			text = m_greetingTexts[0];
		} else {
			// 랜덤 인삿말
			text = m_greetingTexts[m_greetingIndex];
			++m_greetingIndex;
			if (m_greetingIndex >= (int)m_greetingTexts.size()) {
				if (m_greetingTexts.size() > 1)
					std::random_shuffle(m_greetingTexts.begin() + 1, m_greetingTexts.end());
				m_greetingIndex = ((int)m_greetingTexts.size() > 1) ? 1 : 0;
			}
		}
		m_showNative = !m_showNative;  // 다음 슬롯은 반대
		m_showingPlayerName = true;
	} else {
		// 플레이어 이름 표시 (금색)
		text  = m_playerDisplayName;
		color = Color3B(255, 215, 0);
		m_showingPlayerName = false;
	}

	m_nameplateLabel->setString(text);
	m_nameplateLabel->setColor(color);
	m_nameplateLabel->setOpacity(0);
	m_nameplateLabel->stopAllActions();
	m_nameplateLabel->runAction(Sequence::create(
		FadeIn::create(0.2f),
		DelayTime::create(1.4f),
		FadeOut::create(0.2f),
		CallFunc::create([this]() { showNextNameplateText(); }),
		nullptr
	));
}

void MainScene::showNameInputDialog()
{
	const int DIALOG_TAG = 199;
	const float DW = 250, DH = 110;

	auto backdrop = LayerColor::create(Color4B(0, 0, 0, 0));
	backdrop->setTag(DIALOG_TAG);
	this->addChild(backdrop, 999);
	backdrop->runAction(FadeTo::create(0.2f, 160));

	auto dlg = LayerColor::create(Color4B(10, 15, 50, 230), DW, DH);
	dlg->setPosition(Vec2((RESOURCE_WIDTH - DW) / 2, (RESOURCE_HEIGHT - DH) / 2));
	dlg->setScale(0.7f);
	backdrop->addChild(dlg);
	dlg->runAction(Sequence::create(
		ScaleTo::create(0.15f, 1.05f),
		ScaleTo::create(0.08f, 1.0f),
		nullptr
	));

	auto titleLabel = Label::createWithSystemFont("SET YOUR NAME", "Arial", 14);
	titleLabel->setColor(Color3B(255, 215, 0));
	titleLabel->setPosition(Vec2(DW / 2, DH - 22));
	dlg->addChild(titleLabel);

	auto divider = DrawNode::create();
	divider->drawLine(Vec2(15, DH - 40), Vec2(DW - 15, DH - 40), Color4F(0.5f, 0.5f, 0.5f, 0.8f));
	dlg->addChild(divider);

	auto editBoxBg = DrawNode::create();
	editBoxBg->drawSolidRect(Vec2(8, DH - 84), Vec2(DW - 8, DH - 56),
		Color4F(0.05f, 0.07f, 0.15f, 0.9f));
	editBoxBg->drawRect(Vec2(8, DH - 84), Vec2(DW - 8, DH - 56),
		Color4F(0.4f, 0.4f, 0.6f, 0.7f));
	dlg->addChild(editBoxBg);

	auto editBox = cocos2d::ui::EditBox::create(Size(DW - 20, 24),
		cocos2d::ui::Scale9Sprite::create());
	editBox->setFont("Arial", 13);
	editBox->setFontColor(Color3B::WHITE);
	editBox->setPlaceholderFontColor(Color3B(180, 180, 180));
	editBox->setPlaceHolder("3-12 chars");
	editBox->setMaxLength(12);
	editBox->setInputMode(cocos2d::ui::EditBox::InputMode::SINGLE_LINE);
	editBox->setReturnType(cocos2d::ui::EditBox::KeyboardReturnType::DONE);
	editBox->setAnchorPoint(Vec2(0, 0.5f));
	editBox->setPosition(Vec2(10, DH - 70));
	dlg->addChild(editBox);

	auto okLabel = Label::createWithSystemFont("  OK  ", "Arial", 14);
	okLabel->setColor(Color3B(100, 100, 100));
	auto okBtn = MenuItemLabel::create(okLabel, [editBox](Ref*) {
		std::string name = editBox->getText();
		UserDataManager::Instance()->SetUserName(name);
		UserDataManager::Instance()->SaveUserData();

		auto* ud = UserDataManager::Instance();
		int lv = ud->m_pendingSubmitLevel;
		int tm = ud->m_pendingSubmitTime;
		bool hasPending = ud->HasPendingSubmit();
		ud->ClearPendingSubmit();

		// updateDisplayName 완료 → submitScore 완료 → replaceScene 순서로 체이닝
		LeaderboardManager::Instance()->updateDisplayName(name, [lv, tm, hasPending](bool) {
			auto goToMain = []() {
				UserDataManager::Instance()->m_justRegistered = true;
				Director::getInstance()->replaceScene(
					TransitionFade::create(0.3f, MainScene::createScene()));
			};
			if (hasPending) {
				LeaderboardManager::Instance()->submitScore(lv, tm, [goToMain](bool) {
					goToMain();
				});
			} else {
				goToMain();
			}
		});
	});
	okBtn->setEnabled(false);

	dlg->schedule([editBox, okBtn, okLabel](float) {
		bool valid = strlen(editBox->getText()) >= 3;
		okBtn->setEnabled(valid);
		okLabel->setColor(valid ? Color3B::YELLOW : Color3B(100, 100, 100));
	}, 0.05f, CC_REPEAT_FOREVER, 0.f, "nameCheck");

	auto menu = Menu::create(okBtn, nullptr);
	menu->setPosition(Vec2(DW / 2, 18));
	dlg->addChild(menu);
}
