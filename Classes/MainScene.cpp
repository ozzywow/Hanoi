#include "stdafx.h"
#include "MainScene.h"
#include "SoundFactory.h"
#include "PlayScene.h"
//#include "RankScene.h"
#include "UserDataManager.h"
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
#ifdef LITE_VER
	CMKStoreManager::Instance()->SetDelegate(NULL);
#endif //LITE_VER
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
	

	Menu* mainMenu = Menu::create(startMenuItem, NULL);
	mainMenu->alignItemsVerticallyWithPadding(5);
	mainMenu->setAnchorPoint(Point::ZERO);
	mainMenu->setPosition(100, 140);
	this->addChild(mainMenu, tagInfoText, tagInfoText);

	
#ifdef LITE_VER
	if (false == UserDataManager::Instance()->GetCart())
	{
		MenuItemImage* cartMenuItem = MenuItemImage::create("NewUI/btn_cart.png", "NewUI/btn_cart_s.png", CC_CALLBACK_1(MainScene::callbackOnPushed_buyMenuItem, this));
		std::string strBuy = "BUY";
		auto labelBuy = Label::create(strBuy, "Arial", 10);				
		labelBuy->setPosition(cartMenuItem->getContentSize().width/2, -5);
		cartMenuItem->addChild(labelBuy);

		MenuItemImage* pLockMenu = MenuItemImage::create("NewUI/lock_icon.png", "NewUI/lock_icon_s.png", CC_CALLBACK_1(MainScene::callbackLockBtn, this));
		std::string strRestore = "RESTORE";
		auto labelRestore = Label::create(strRestore, "Arial", 10);
		labelRestore->setPosition(pLockMenu->getContentSize().width / 2, -5);
		pLockMenu->addChild(labelRestore);

		Menu* subMenu = Menu::create(cartMenuItem, pLockMenu, NULL);
		subMenu->alignItemsHorizontallyWithPadding(0);
		subMenu->setAnchorPoint(Point::ZERO);
		subMenu->setPosition(100, 70);
		this->addChild(subMenu, tagCart, tagCart);

		CMKStoreManager::Instance()->SetDelegate(this);
	}	
	
#endif //LITE_VER
	

	m_rankTableLayer = Layer::create();
	this->addChild(m_rankTableLayer, tagInfoText, tagInfoText);


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
			Label* labelRecord = Label::create(strRecord, "Arial", 14);
			labelRecord->setAnchorPoint(ccp(0, 0));
			labelRecord->setPosition(ccp(40, (level * 22) - 20));
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



void MainScene::callbackOnPushed_startMenuItem(Ref* pSender)
{
	SoundFactory::Instance()->play("FX0066");

	if (m_rankBG)
	{
		auto action = MoveTo::create(0.3, ccp(600, 320 - 60));
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

	SoundFactory::Instance()->play("FX0070", 0.4);
#endif //LITE_VER
}




void MainScene::callbackLockBtn(Ref* sender)
{
	if (isRestored) { return; }
	if (true == isProgress) { return; }
	isProgress = true;	

	CMKStoreManager::Instance()->ToggleIndicator(true);
	CMKStoreManager::Instance()->restorePreviousTransactions();

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
		Label* pPrizeMsg = Label::create(strMsg, "Arial", 20);
		pPrizeMsg->setPosition(ccp(RESOURCE_WIDTH / 2, 20));
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
		Label* pPrizeMsg = Label::create(strMsg, "Arial", 20);
		pPrizeMsg->setPosition(ccp(RESOURCE_WIDTH / 2, 300));
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
	Label* pPrizeMsg = Label::create(strMsg, "Arial", 20);
	pPrizeMsg->setPosition(ccp(RESOURCE_WIDTH/2, 300));
	this->addChild(pPrizeMsg, tagPopup, tagPopup);
	
	auto action1 = ScaleTo::create(0.1, 1.0);
	auto action2 = Blink::create(4, 4);	
	auto actionSeq = Sequence::create(action1, action2, NULL);
	pPrizeMsg->runAction(actionSeq);	

	this->removeChildByTag(tagCart);
		

}
#endif //LITE_VER
