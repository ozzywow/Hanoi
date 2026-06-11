#include "stdafx.h"
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
	

	MenuItemImage* rankMenuItem = MenuItemImage::create("NewUI/btn_rank_n.png", "NewUI/btn_rank_s.png", CC_CALLBACK_1(MainScene::callbackOnPushed_rankMenuItem, this));

	Menu* mainMenu = Menu::create(startMenuItem, rankMenuItem, NULL);
	mainMenu->alignItemsVerticallyWithPadding(5);
	mainMenu->setAnchorPoint(Point::ZERO);
	mainMenu->setPosition(100, 180);
	this->addChild(mainMenu, tagInfoText, tagInfoText);

	auto resetLabel = Label::createWithSystemFont("RESET", "Arial", 12);
	resetLabel->setColor(Color3B(180, 60, 60));
	auto resetMenuItem = MenuItemLabel::create(resetLabel,
		CC_CALLBACK_1(MainScene::callbackOnPushed_resetMenuItem, this));
	Menu* resetMenu = Menu::create(resetMenuItem, NULL);
	resetMenu->setPosition(Vec2(40, 12));
	this->addChild(resetMenu, tagInfoText, tagInfoText);

	std::string name = UserDataManager::Instance()->GetUserName();
	auto nameLabel = Label::createWithSystemFont(name, "Arial", 14);
	nameLabel->setPosition(Vec2(100, 12)); 
	this->addChild(nameLabel);
 
	
#ifdef LITE_VER
	if (false == UserDataManager::Instance()->GetCart())
	{
		MenuItemImage* cartMenuItem = MenuItemImage::create("NewUI/btn_cart.png", "NewUI/btn_cart_s.png", CC_CALLBACK_1(MainScene::callbackOnPushed_buyMenuItem, this));
		std::string strBuy = "BUY";
		auto labelBuy = Label::createWithSystemFont(strBuy, "Arial", 10);				
		labelBuy->setPosition(cartMenuItem->getContentSize().width/2, -5);
		cartMenuItem->addChild(labelBuy);

		MenuItemImage* pLockMenu = MenuItemImage::create("NewUI/lock_icon.png", "NewUI/lock_icon_s.png", CC_CALLBACK_1(MainScene::callbackLockBtn, this));
		std::string strRestore = "RESTORE";
		auto labelRestore = Label::createWithSystemFont(strRestore, "Arial", 10);
		labelRestore->setPosition(pLockMenu->getContentSize().width / 2, -5);
		pLockMenu->addChild(labelRestore);

		Menu* subMenu = Menu::create(cartMenuItem, pLockMenu, NULL);
		subMenu->alignItemsHorizontallyWithPadding(0);
		subMenu->setAnchorPoint(Point::ZERO);
		subMenu->setPosition(100, 70);
		this->addChild(subMenu, tagCart, tagCart);		
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

	const float DW = 200, DH = 80;

	auto backdrop = LayerColor::create(Color4B(0, 0, 0, 160));
	backdrop->setTag(CONFIRM_TAG);
	this->addChild(backdrop, 999);

	auto dlg = LayerColor::create(Color4B(20, 20, 55, 240), DW, DH);
	dlg->setPosition(Vec2((RESOURCE_WIDTH - DW) / 2, (RESOURCE_HEIGHT - DH) / 2));
	backdrop->addChild(dlg);

	auto msg = Label::createWithSystemFont("Reset all records?", "Arial", 13);
	msg->setPosition(Vec2(DW / 2, 58));
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
			for (int level = 3; level < MAX_PLAY_LEVEL; ++level)
			{
				int record = ud->GetBestRecord(level);
				RecordTime rt = getRecordTime(record);
				std::string str = StringUtils::format("%02d                                         %02d:%02d:%02d",
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
	okBtn->setPosition(Vec2(DW * 0.3f, 20));

	auto cancelLabel = Label::createWithSystemFont("Cancel", "Arial", 14);
	cancelLabel->setColor(Color3B::WHITE);
	auto cancelBtn = MenuItemLabel::create(cancelLabel, [this](Ref*) {
		this->removeChildByTag(198);
	});
	cancelBtn->setPosition(Vec2(DW * 0.7f, 20));

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


	int level = UserDataManager::Instance()->GetLevel();
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
		SoundFactory::Instance()->play("drop_coin");
		std::string strMsg = "You can play all levels.";
		Label* pPrizeMsg = Label::createWithSystemFont(strMsg, "Arial", 20);
		pPrizeMsg->setPosition(Vec2(RESOURCE_WIDTH / 2, 20));
		this->addChild(pPrizeMsg);

		auto action1 = ScaleTo::create(0.1, 1.0);
		auto action2 = Blink::create(4, 4);
		auto actionSeq = Sequence::create(action1, action2, NULL);
		pPrizeMsg->runAction(actionSeq);
		this->removeChildByTag(tagCart);

		UserDataManager::Instance()->SetCart(true);
		UserDataManager::Instance()->SaveUserData();
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
		std::string strMsg = "No item you bought.";
		Label* pPrizeMsg = Label::createWithSystemFont(strMsg, "Arial", 20);
		pPrizeMsg->setPosition(Vec2(RESOURCE_WIDTH / 2, 300));
		this->addChild(pPrizeMsg, tagPopup, tagPopup);

		auto action1 = ScaleTo::create(0.1, 1.0);
		auto action2 = Blink::create(4, 4);
		auto actionSeq = Sequence::create(action1, action2, NULL);
		pPrizeMsg->runAction(actionSeq);
		return; 
	}

	cocos2d::log("restorePreviousTransactions");

	UserDataManager::Instance()->SetCart(true);
	UserDataManager::Instance()->SaveUserData();


	CMKStoreManager::Instance()->ToggleIndicator(false);	
	SoundFactory::Instance()->play("FX0070", 0.4);

	//Purchase items restored.
	auto director = Director::getInstance();
	auto glview = director->getOpenGLView();
	auto frameSize = glview->getDesignResolutionSize();
	const int		sizeOfFont = FRAME_WIDTH*0.05f;

		
	std::string strMsg = "Restored all levels you bought.";
	Label* pPrizeMsg = Label::createWithSystemFont(strMsg, "Arial", 20);
	pPrizeMsg->setPosition(Vec2(RESOURCE_WIDTH/2, 300));
	this->addChild(pPrizeMsg, tagPopup, tagPopup);
	
	auto action1 = ScaleTo::create(0.1, 1.0);
	auto action2 = Blink::create(4, 4);	
	auto actionSeq = Sequence::create(action1, action2, NULL);
	pPrizeMsg->runAction(actionSeq);	

	this->removeChildByTag(tagCart);
		

}
#endif //LITE_VER


void MainScene::callbackOnPushed_rankMenuItem(Ref* pSender)
{
	SoundFactory::Instance()->play("FX0070", 0.4);
	m_rankLevel = UserDataManager::Instance()->GetLevel();
	if (m_rankLevel < 3) m_rankLevel = 3;
	drawOnlineRank(m_rankLevel);
}

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
	if (m_rankLevel >= MAX_PLAY_LEVEL - 1) return;
	SoundFactory::Instance()->play("FX0070", 0.4);
	++m_rankLevel;
	int lv = m_rankLevel;
	scheduleOnce([this, lv](float) { drawOnlineRank(lv); }, 0.0f, "rankNext");
}

void MainScene::drawOnlineRank(int level)
{
	// removeAllChildren 대신 m_rankBG를 직접 해제 (removeAllChildren 사용시 크래시 발생)
	if (m_rankBG)
	{
		m_rankBG->setVisible(false);
		m_rankBG = nullptr;
	}

	// 로딩 표시 (이전 내용 지우기)
	m_rankTableLayer->removeAllChildren();

	const int TAG_LOADING = 50;

	Label* loading = Label::createWithSystemFont("Loading...", "Arial", 16);
	loading->setPosition(Vec2(340, 120));
	m_rankTableLayer->addChild(loading, 0, TAG_LOADING);


	// prev / next 네비게이션 버튼
	MenuItemImage* prevBtn = MenuItemImage::create(
		"NewUI/btn_prev.png", "NewUI/btn_prev.png",
		CC_CALLBACK_1(MainScene::callbackRankPrev, this));
	MenuItemImage* nextBtn = MenuItemImage::create(
		"NewUI/btn_next.png", "NewUI/btn_next.png",
		CC_CALLBACK_1(MainScene::callbackRankNext, this));

	prevBtn->setEnabled(level > 3);
	nextBtn->setEnabled(level < MAX_PLAY_LEVEL - 1);

	Menu* navMenu = Menu::create(prevBtn, nextBtn, NULL);
	navMenu->alignItemsHorizontallyWithPadding(10);
	navMenu->setPosition(Vec2(100, 120));
	m_rankTableLayer->addChild(navMenu);

	auto alive = m_aliveFlag;
	LeaderboardManager::Instance()->fetchLeaderboard(level, 10,
		[this, alive, level](const std::vector<LeaderboardEntry>& entries) {
			if (!alive || !*alive) return;

			const int TAG_LOADING = 50;
			m_rankTableLayer->removeChildByTag(TAG_LOADING);
			m_rankTableLayer->removeChildByTag(tagBG);

			Sprite* bg = Sprite::create("NewUI/top_rank_bg_2.png");
			if (!bg) return;
			bg->setAnchorPoint(Point::ANCHOR_TOP_LEFT);
			bg->setPosition(Vec2(180, 320 - 60));

			MoveTo* action = MoveTo::create(0.3, Vec2(180, 320 - 60));
			bg->runAction(action);		

			m_rankBG = bg;
			m_rankTableLayer->addChild(bg, tagBGRankingBoard, tagBGRankingBoard);

			std::string title = StringUtils::format("LEVEL %d  ONLINE RANK", level);
			Label* titleLabel = Label::createWithSystemFont(title, "Arial", 12);
			titleLabel->setAnchorPoint(Vec2(0, 0));
			titleLabel->setPosition(Vec2(40, 230));
			bg->addChild(titleLabel);

			Label* infoLabel = Label::createWithSystemFont("BEST IN THE WORLD",  "Arial", 20);
			infoLabel->setPosition(160, 135);
			bg->addChild(infoLabel, -1);

			std::string pLevelStr = StringUtils::format("LEVEL %d", level);
			Label* levelLabel = Label::createWithSystemFont(pLevelStr, "Arial", 18);
			levelLabel->setPosition(160, 115);
			bg->addChild(levelLabel, -1);

			if (entries.empty()) {
				Label* noData = Label::createWithSystemFont("No records yet", "Arial", 12);
				noData->setAnchorPoint(Vec2(0, 0));
				noData->setPosition(Vec2(340, 135));
				bg->addChild(noData);
				return;
			}

			for (int i = 0; i < (int)entries.size(); ++i) {
				const auto& e = entries[i];
				RecordTime rt = getRecordTime(e.scoreMs);
				float y = 178 - i * 22;

				Label* rankLabel = Label::createWithSystemFont(
					StringUtils::format("%d", e.rank), "Arial", 14);
				rankLabel->setAnchorPoint(Vec2(0, 0));
				rankLabel->setPosition(Vec2(40, y));
				bg->addChild(rankLabel);

				if (!e.countryCode.empty()) {
					std::string flagPath = "flag_icon/" + e.countryCode + ".png";
					auto flag = Sprite::create(flagPath);
					if (flag) {
						flag->setAnchorPoint(Vec2(0, 0.5f));
						flag->setScale(0.70f);
						flag->setPosition(Vec2(65, y + 7));
						bg->addChild(flag);
					}
				}

				Label* nameLabel = Label::createWithSystemFont(
					e.displayName.substr(0, 10), "Arial", 14);
				nameLabel->setAnchorPoint(Vec2(0, 0));
				nameLabel->setPosition(Vec2(92, y));
				bg->addChild(nameLabel);

				Label* timeLabel = Label::createWithSystemFont(
					StringUtils::format("%02d:%02d:%02d", rt.min, rt.sec, rt.ms), "Arial", 14);
				timeLabel->setAnchorPoint(Vec2(0, 0));
				timeLabel->setPosition(Vec2(220, y));
				bg->addChild(timeLabel);
			}
		});
} 

void MainScene::showNameInputDialog()
{
	const int DIALOG_TAG = 199;
	const float DW = 230, DH = 95;

	auto backdrop = LayerColor::create(Color4B(0, 0, 0, 160));
	backdrop->setTag(DIALOG_TAG);
	this->addChild(backdrop, 999);

	auto dlg = LayerColor::create(Color4B(20, 20, 55, 240), DW, DH);
	dlg->setPosition(Vec2((RESOURCE_WIDTH - DW) / 2, (RESOURCE_HEIGHT - DH) / 2));
	backdrop->addChild(dlg);

	auto prompt = Label::createWithSystemFont("Enter your ranker name!", "Arial", 13);
	prompt->setAnchorPoint(Vec2(0, 0.5f));
	prompt->setPosition(Vec2(10, 75));
	dlg->addChild(prompt);

	auto editBg = cocos2d::ui::Scale9Sprite::create("NewUI/text_empty.png");
	auto editBox = cocos2d::ui::EditBox::create(Size(DW - 20, 24), editBg);
	editBox->setFont("Arial", 13);
	editBox->setFontColor(Color3B::WHITE);
	editBox->setPlaceholderFontColor(Color3B(180, 180, 180));
	editBox->setPlaceHolder("3-12 chars");
	editBox->setMaxLength(12);
	editBox->setInputMode(cocos2d::ui::EditBox::InputMode::SINGLE_LINE);
	editBox->setReturnType(cocos2d::ui::EditBox::KeyboardReturnType::DONE);
	editBox->setAnchorPoint(Vec2(0, 0.5f));
	editBox->setPosition(Vec2(10, 48));
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
	menu->setPosition(Vec2(DW / 2, 15));
	dlg->addChild(menu);
}
