#include "stdafx.h"
#include "MainScene.h"
#include "SoundFactory.h"
#include "PlayScene.h"
//#include "RankScene.h"
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
	m_arrPrize.clear();
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
		
	Sprite* backGround = Sprite::create("NewUI/title_bg.png") ;
	if (backGround)
	{
		backGround->setAnchorPoint(Point::ZERO);
		this->addChild(backGround, tagBG, tagBG);
	}
	

	m_arrPrize.assign(11, 0);

	MenuItemImage* startMenuItem = MenuItemImage::create("NewUI/btn_start_n.png", "NewUI/btn_start_s.png", CC_CALLBACK_1(MainScene::callbackOnPushed_startMenuItem, this));
	

	MenuItemImage* rankMenuItem = MenuItemImage::create("NewUI/btn_rank_n.png", "NewUI/btn_rank_s.png", CC_CALLBACK_1(MainScene::callbackOnPushed_rankMenuItem, this));

	Menu* mainMenu = Menu::create(startMenuItem, rankMenuItem, NULL);
	mainMenu->alignItemsVerticallyWithPadding(5);
	mainMenu->setPosition(100, 140);
	mainMenu->setAnchorPoint(Point::ZERO);
	this->addChild(mainMenu, tagInfoText, tagInfoText);

	auto resetLabel = Label::createWithSystemFont("RESET", "Arial", 12);
	resetLabel->setColor(Color3B(180, 60, 60));
	auto resetMenuItem = MenuItemLabel::create(resetLabel,
		CC_CALLBACK_1(MainScene::callbackOnPushed_resetMenuItem, this));
	Menu* resetMenu = Menu::create(resetMenuItem, NULL);
	resetMenu->setPosition(Vec2(40, 12));
	this->addChild(resetMenu, tagInfoText, tagInfoText);

	std::string name = UserDataManager::Instance()->GetUserName();
		auto nameLabel = Label::createWithSystemFont(name, "Arial", 20);
		nameLabel->setPosition(Vec2(100, 14));
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

	if (UserDataManager::Instance()->GetUserName().empty())
		showNameInputDialog();
	else
		LeaderboardManager::Instance()->login([](bool ok) {
			log("PlayFab startup login: %s", ok ? "OK" : "FAIL");
		});


	int level = UserDataManager::Instance()->GetLevel();
	if (NULL == m_rankBG)
	{
		SoundFactory::Instance()->play("FX0108");
		m_rankBG = Sprite::create("NewUI/top_rank_bg_2.png");
		m_rankBG->setAnchorPoint(Point::ANCHOR_TOP_LEFT);
		m_rankBG->setPosition(600, 320 - 60);

		MoveTo* action = MoveTo::create(0.3, Vec2(180, 320 - 60));
		m_rankBG->runAction(action);
		m_rankTableLayer->addChild(m_rankBG, tagInfoText, tagInfoText);
				
		for (int level = 3; level < MAX_PLAY_LEVEL; ++level)
		{
			int record = UserDataManager::Instance()->GetBestRecord(level);
			RecordTime recordTime = getRecordTime(record);
			std::string strRecord = StringUtils::format("%02d                                         %02d:%02d:%02d", level, recordTime.min, recordTime.sec, recordTime.ms);
			Label* labelRecord = Label::createWithSystemFont(strRecord, "Arial", 14);
			labelRecord->setAnchorPoint(Vec2(0, 0));
			labelRecord->setPosition(Vec2(40, (level * 22) - 20));
			m_rankBG->addChild(labelRecord);
		}
		

		Label* infoLabel = Label::createWithSystemFont("BEST OF BEST",  "Arial", 20);
		infoLabel->setPosition(340, 135);
		this->addChild(infoLabel, tagBG, tagBG);		

		std::string pLevelStr = StringUtils::format("LEVEL %d", level);
		Label* levelLabel = Label::createWithSystemFont(pLevelStr, "Arial", 20);
		levelLabel->setPosition(340, 115);
		this->addChild(levelLabel, tagBG, tagBG);
	}

	return true;
}


void MainScene::onExitTransitionDidStart()
{
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
		showNameInputDialog([]() {
			Director::getInstance()->replaceScene(MainScene::create());
		});
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
	// removeAllChildren??m_rankBG瑜??댁젣?섎?濡?癒쇱? ?ъ씤?곕? 臾댄슚??
	if (m_rankBG)
	{
		m_rankBG->setVisible(false);
		m_rankBG = nullptr;
	}

	// 濡쒕뵫 ?쒖떆 (?댁쟾 ?댁슜 吏?곌린)
	m_rankTableLayer->removeAllChildren();

	const int TAG_LOADING = 50;

	Label* loading = Label::createWithSystemFont("Loading...", "Arial", 16);
	loading->setPosition(Vec2(300, 160));
	m_rankTableLayer->addChild(loading, 0, TAG_LOADING);

	// prev / next ?ㅻ퉬寃뚯씠??踰꾪듉
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
	navMenu->setPosition(Vec2(100, 80));
	m_rankTableLayer->addChild(navMenu);

	LeaderboardManager::Instance()->fetchLeaderboard(level, 10,
		[this, level](const std::vector<LeaderboardEntry>& entries) {

			const int TAG_LOADING = 50;
			m_rankTableLayer->removeChildByTag(TAG_LOADING);
			m_rankTableLayer->removeChildByTag(tagBG);

			Sprite* bg = Sprite::create("NewUI/top_rank_bg_2.png");
			if (!bg) return;
			bg->setAnchorPoint(Point::ANCHOR_TOP_LEFT);
			bg->setPosition(Vec2(180, 320 - 60));
			m_rankTableLayer->addChild(bg, 0, tagBG);

			std::string title = StringUtils::format("LEVEL %d  ONLINE RANK", level);
			Label* titleLabel = Label::createWithSystemFont(title, "Arial", 12);
			titleLabel->setAnchorPoint(Vec2(0, 0));
			titleLabel->setPosition(Vec2(40, 230));
			bg->addChild(titleLabel);

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

				Label* nameLabel = Label::createWithSystemFont(
					e.displayName.substr(0, 10), "Arial", 14);
				nameLabel->setAnchorPoint(Vec2(0, 0));
				nameLabel->setPosition(Vec2(120, y));
				bg->addChild(nameLabel);

				Label* timeLabel = Label::createWithSystemFont(
					StringUtils::format("%02d:%02d:%02d", rt.min, rt.sec, rt.ms), "Arial", 14);
				timeLabel->setAnchorPoint(Vec2(0, 0));
				timeLabel->setPosition(Vec2(220, y));
				bg->addChild(timeLabel);
			}
		});
}

void MainScene::showNameInputDialog(std::function<void()> onDone)
{
	const int DIALOG_TAG = 199;
	const float DW = 230, DH = 95;

	auto backdrop = LayerColor::create(Color4B(0, 0, 0, 160));
	backdrop->setTag(DIALOG_TAG);
	this->addChild(backdrop, 999);

	auto dlg = LayerColor::create(Color4B(20, 20, 55, 240), DW, DH);
	dlg->setPosition(Vec2((RESOURCE_WIDTH - DW) / 2, (RESOURCE_HEIGHT - DH) / 2));
	backdrop->addChild(dlg);

	auto prompt = Label::createWithSystemFont("Enter your name:", "Arial", 13);
	prompt->setAnchorPoint(Vec2(0, 0.5f));
	prompt->setPosition(Vec2(10, 75));
	dlg->addChild(prompt);

	auto editBg = cocos2d::ui::Scale9Sprite::create("NewUI/text_empty.png");
	auto editBox = cocos2d::ui::EditBox::create(Size(DW - 20, 24), editBg);
	editBox->setFont("Arial", 13);
	editBox->setFontColor(Color3B::BLACK);
	editBox->setPlaceHolder("Player");
	editBox->setMaxLength(12);
	editBox->setAnchorPoint(Vec2(0, 0.5f));
	editBox->setPosition(Vec2(10, 48));
	dlg->addChild(editBox);

	auto okLabel = Label::createWithSystemFont("  OK  ", "Arial", 14);
	okLabel->setColor(Color3B::YELLOW);
	auto okBtn = MenuItemLabel::create(okLabel, [this, editBox, onDone](Ref*) {
		std::string name = editBox->getText();
		if (name.empty()) name = "Player";
		UserDataManager::Instance()->SetUserName(name);
		UserDataManager::Instance()->SaveUserData();
		this->removeChildByTag(199);
		auto* lm = LeaderboardManager::Instance();
		if (lm->isLoggedIn())
			lm->updateDisplayName(name);
		else
			lm->login([](bool ok) {
				log("PlayFab startup login: %s", ok ? "OK" : "FAIL");
			});
		if (onDone) onDone();
	});
	auto menu = Menu::create(okBtn, nullptr);
	menu->setPosition(Vec2(DW / 2, 15));
	dlg->addChild(menu);
}
