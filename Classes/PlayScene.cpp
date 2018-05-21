#include "stdafx.h"
#include "PlayScene.h"
#include "PlaySceneTouchHandlerLayer.h"
#include "Discus.h"
#include "SoundFactory.h"
#include "MainScene.h"
#include "UserDataManager.h"
#include "MKStoreManager_cpp.h"


static Point arrPosOfPole[3] ;



bool PlayScene::initWithDiscusNum(int numOfDiscus)
{
	if (!Scene::init())
	{
		return false;
	}

#ifdef LITE_VER
	this->isProgress = false;
	this->isRestored = false;
	CMKStoreManager::Instance()->SetDelegate(this);
#endif //LITE_VER
	
	m_countOfDiscus = numOfDiscus;

	arrPosOfPole[0] = ccp(105, 140);
	arrPosOfPole[1] = ccp(241, 140);
	arrPosOfPole[2] = ccp(377, 140);

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
		ccBlendFunc bfMask = ccBlendFunc();
		bfMask.src = GL_SRC_ALPHA;
		bfMask.dst = GL_ONE;
		m_selectionMark->setBlendFunc(bfMask);
		m_selectionMark->setOpacity(100);
		m_selectionMark->setAnchorPoint(Point(0.5, 0));
		this->addChild(m_selectionMark, tagSelectionMark, tagSelectionMark);
		m_selectionMark->setPosition(500, 500);
	}

	m_deselectionMark = Sprite::create("NewUI/block_selectEff.png") ;
	if (m_deselectionMark)
	{
		ccBlendFunc bfMask = ccBlendFunc();
		bfMask.src = GL_SRC_ALPHA;
		bfMask.dst = GL_ONE;
		m_deselectionMark->setBlendFunc(bfMask);
		m_deselectionMark->setOpacity(100);
		m_deselectionMark->setAnchorPoint(Point(0.5, 0));
		this->addChild(m_deselectionMark, tagSelectionMark, tagSelectionMark);
		m_deselectionMark->setPosition(500, 500);
	}


	std::string timeString = StringUtils::format("00:00:00");
	m_labelTime = Label::create(timeString, "Arial", 20);
	m_labelTime->setColor(ccc3(255, 255, 255));
	m_labelTime->setPosition(ccp(415, 310));
	this->addChild(m_labelTime, tagInfoText, tagInfoText);


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

	
	if (m_countOfDiscus == 3)
	{
		Sprite* imgCount = NULL;
		imgCount = Sprite::create("NewUI/arrow.png");
		this->addChild(imgCount, tagCountDown, tagCountDown);
		imgCount->setAnchorPoint(ccp(0.5, 0.5));
		imgCount->setPosition(ccp(480 / 2, 220));
		

		Sprite* text = NULL;		
		text = Sprite::create("NewUI/text_en.png");
		text->setAnchorPoint(ccp(0.5, 0.5));
		text->setPosition(ccp(imgCount->getContentSize().width / 2, (imgCount->getContentSize().height / 2) - 80));
		imgCount->addChild(text);
	}

#ifdef LITE_VER
	else if(m_countOfDiscus > MAX_LIMIT_LEVEL_FOR_LITE)
	if (false == UserDataManager::Instance()->GetCart())
	{
		MenuItemImage* pLockMenu = MenuItemImage::create("NewUI/lock_icon.png", "NewUI/lock_icon_s.png", CC_CALLBACK_1(PlayScene::callbackLockBtn, this));
		Menu* pMenu = Menu::create(pLockMenu, NULL);		
		pMenu->setPosition(ccp(105, 200));
		this->addChild(pMenu, tagCart, tagCart);

		auto action = Blink::create(10, 10);
		pMenu->runAction(action);
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
	MenuItemImage* resetMenuItem = MenuItemImage::create("NewUI/btn_refresh_n.png", "NewUI/btn_refresh_s.png", CC_CALLBACK_1(PlayScene::callbackOnPushed_resetMenuItem, this));
	MenuItemImage* prevMenuItem = MenuItemImage::create("NewUI/btn_prev.png", "NewUI/btn_prev.png", CC_CALLBACK_1(PlayScene::callbackOnPushed_prevMenuItem, this));
	MenuItemImage* nextMenuItem = MenuItemImage::create("NewUI/btn_next.png", "NewUI/btn_next.png", CC_CALLBACK_1(PlayScene::callbackOnPushed_nextMenuItem, this));
	
	
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
	
	
	Menu* mainMenuTop = Menu::create(homeMenuItem, resetMenuItem, nextMenuItem, prevMenuItem, speakerMenuItem, NULL) ;	
	mainMenuTop->alignItemsVerticallyWithPadding(5);
	mainMenuTop->setPosition(ccp(20, 220));
	this->addChild(mainMenuTop, 1234, 1234);
}

void	PlayScene::InitGame()
{	
	this->removeChildByTag(tagPopup, false);
	m_isIng = NONE ;
	
	
	
	m_countDown=3;
	m_labelTime->setString("00:00:00");

}


PLAY_STATE PlayScene::GetPlayState()
{
	return m_isIng;
}

void PlayScene::Start()
{
	m_isIng = PLAY;
	
	
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
	this->stopAction(m_actionTimeRun);	
	int elapsedTime = getMilliCount() - m_dateTime;	
	SoundFactory::Instance()->stop("BGM2");
	
	
	m_mastTime = elapsedTime ;
		
	
	DelayTime* delay = DelayTime::create(2.0);
	std::function<void()> func = std::bind(&PlayScene::MessagePopup, this);
	CallFunc* callFucn = CallFunc::create(func);
	this->runAction(Sequence::create(delay, callFucn, NULL));
}


void PlayScene::MessagePopup()
{
	SoundFactory::Instance()->play("drop_coin");
	Sprite* pMSGBG = Sprite::create("NewUI/text_empty.png");
	
	if (m_mastTime < 100) m_mastTime = 100;
	RecordTime recordTime = getRecordTime(m_mastTime);
	std::string strRecordTime = StringUtils::format("Record Time  %02d:%02d:%02d", recordTime.min, recordTime.sec, recordTime.sec);
	std::string strMsg = "GOOD!!";
	int bestRecordTime = UserDataManager::Instance()->GetBestRecord(m_countOfDiscus);
	if (bestRecordTime == 0 || (bestRecordTime > 0 && bestRecordTime > m_mastTime))
	{		
		UserDataManager::Instance()->SetBestRecord(m_countOfDiscus, m_mastTime);
		if (m_countOfDiscus < MAX_PLAY_LEVEL)
		{
			UserDataManager::Instance()->SetLevel(m_countOfDiscus);
		}
		UserDataManager::Instance()->SaveUserData();
		strMsg = "NEW RECORD!!!";
	}	
	
	Label* pPrizeMsg = Label::create(strMsg, "Arial" , 30);
	pPrizeMsg->setPosition(ccp(180, 130));

	Label* pRecordTime = Label::create(strRecordTime, "Arial", 30);
	pRecordTime->setPosition(ccp(180, 90));
	
	pMSGBG->addChild(pPrizeMsg);
	pMSGBG->addChild(pRecordTime);

	pMSGBG->setAnchorPoint(ccp(0.5, 0.5));
	pMSGBG->setPosition(ccp(480 / 2, 320 / 2));
	pMSGBG->setScale(0.5);


	auto action = ScaleTo::create(0.1, 1.0);
	pMSGBG->runAction(action);

	this->addChild(pMSGBG, tagPopup, tagPopup);
}



void PlayScene::DrawTime()
{	
	int elapsedTime = getMilliCount() - m_dateTime;
	RecordTime recordTime = getRecordTime(elapsedTime);
	int minutes = recordTime.min;
	int seconds = recordTime.sec;
	int ms = recordTime.ms;
	
	std::string strTime = StringUtils::format("%02d:%02d:%02d", minutes, seconds, ms) ;
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
	
	
	m_selectionMark->setPosition(ccp(500,500));
	m_deselectionMark->setPosition(ccp(500,500));
	
	

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
			Point posOfDiscus = ccp(arrPosOfPole[i].x, arrPosOfPole[i].y + (15 * j));
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
	
	int myLevel = UserDataManager::Instance()->GetLevel();
	std::string strLevelInfo = StringUtils::format("%d / %d",m_countOfDiscus, MAX_PLAY_LEVEL-1);
	if( NULL == m_labelLevel )
	{
		m_labelLevel = Label::create(strLevelInfo, "Arial", 20);
		m_labelLevel->setColor(ccc3(255,255,255));
		m_labelLevel->setPosition(ccp( 110, 310 ));
		this->addChild(m_labelLevel, tagInfoText, tagInfoText);
	}
	else 
	{
		m_labelLevel->setString(strLevelInfo);
	}

	
	int result = pow(2,m_countOfDiscus) - 1 ;
	std::string strMoveCount = StringUtils::format("%d / %d", m_moveCount, result);
	
	if( NULL == m_labelMoveCount )
	{
		m_labelMoveCount = Label::create(strMoveCount, "Arial", 20) ;
		m_labelMoveCount->setColor(ccc3(255,255,255));
		m_labelMoveCount->setAnchorPoint(ccp(0, 0.5));
		m_labelMoveCount->setPosition(ccp( 210, 310 ));
		this->addChild(m_labelMoveCount, tagInfoText, tagInfoText);
	}
	else 
	{
		m_labelMoveCount->setString(strMoveCount);
	}
	
}


void PlayScene::CountDown()
{	
	this->removeChildByTag(tagCountDown, true);
	Sprite* imgCount = NULL;
	if( m_isIng == NONE  )
	{
		
		
		m_isIng = COUNT_DOWN;
		
		
		DelayTime* readyDelay = DelayTime::create(2.0);
		DelayTime* countDelay = DelayTime::create(1.0);
		
		std::function<void()> func_CountDown = std::bind(&PlayScene::CountDown, this);
		CallFunc* callFunc_countDown = CallFunc::create(func_CountDown);
		Sequence* actionToContDown = Sequence::create(callFunc_countDown, countDelay, NULL);
		
		Repeat* countDown = Repeat::create(actionToContDown, m_countDown+1);
		std::function<void()> func_Start = std::bind(&PlayScene::Start, this);
		CallFunc* callFunc_start = CallFunc::create(func_Start);
		
		DelayTime* delayStart = DelayTime::create(m_countDown);
		
		this->runAction(Sequence::create(readyDelay, countDown, NULL));
		this->runAction(Sequence::create(readyDelay, delayStart, callFunc_start, NULL));
		SoundFactory::Instance()->play("FX0145");
		
		
		imgCount = Sprite::create("NewUI/spr_ready.png");
		imgCount->setPosition(ccp( 480/2, 200 ));
		this->addChild(imgCount, tagCountDown, tagCountDown);
		
	}
	else 
	{
		std::string strCount = "";
		if( m_countDown == 0 )
		{
			strCount = "NewUI/spr_go.png";	
			SoundFactory::Instance()->play("go") ;
			
			
			
		}
		else 
		{
			strCount = StringUtils::format("NewUI/spr_%d.png", m_countDown);
			SoundFactory::Instance()->play("count_sec");
		}
		
		
		imgCount = Sprite::create(strCount);
		if( m_countDown )
		{
			imgCount->setPosition(ccp( 480/2, 200 ));
			this->addChild(imgCount, tagCountDown, tagCountDown);
		}
		
		
		if( m_countDown > 0 )
		{
			--m_countDown;
		}
	}
	
	if (imgCount)
	{
		imgCount->setScale(0.2);
		auto scaleAction = ScaleTo::create(0.5, 1.0);
		auto fadeAction = FadeIn::create(0.5);
		imgCount->runAction(Sequence::create(scaleAction, fadeAction, NULL));
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
			m_selectionMark->setPosition(ccp( arrPosOfPole[poleID].x, 110));
			m_deselectionMark->setPosition(ccp(500,500));
			SoundFactory::Instance()->play("select");
			
		}
		else 
		{
			m_deselectionMark->setPosition(ccp( arrPosOfPole[poleID].x, 110));
			m_selectionMark->setPosition(ccp(500,500));
			SoundFactory::Instance()->play("deselect") ;
		}
		
	}
	else 
	{
		m_selectionMark->setPosition(ccp(500,500));
		m_deselectionMark->setPosition(ccp(500,500));
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
	
	
	
	if( m_countOfDiscus > 3)
	{
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
	
		
	if( m_countOfDiscus < MAX_PLAY_LEVEL )
	{
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
	if (true == isProgress) { return;  }
	isProgress = true;
	
	CMKStoreManager::Instance()->ToggleIndicator(true);
	CMKStoreManager::Instance()->buyFeature(kProductIdTotal);

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
		Label* pPrizeMsg = Label::create(strMsg, "Arial", 20);
		pPrizeMsg->setPosition(ccp(RESOURCE_WIDTH / 2, 20));
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

	//Purchase items restored.
	auto director = Director::getInstance();
	auto glview = director->getOpenGLView();
	auto frameSize = glview->getDesignResolutionSize();
	const int		sizeOfFont = FRAME_WIDTH*0.05f;


	Sprite* pMSGBG = Sprite::create("NewUI/text_empty.png");
	std::string strMsg = "Restored all levels you bought.";
	Label* pPrizeMsg = Label::create(strMsg, "Arial", 30);
	pPrizeMsg->setPosition(ccp(180, 130));
	pMSGBG->addChild(pPrizeMsg);
	pMSGBG->setAnchorPoint(ccp(0.5, 0.5));
	pMSGBG->setPosition(ccp(480 / 2, 320 / 2));
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