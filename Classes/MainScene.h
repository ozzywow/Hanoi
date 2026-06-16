#pragma once

#include "common_define.h"
#include <vector>
#include <memory>
#include "cocos2d.h"
#include "MKStoreManagerDelegate.h"
using namespace cocos2d;



class  MainScene : public Scene
#ifdef LITE_VER
	, public MKStoreManagerDelegate
#endif //LITE_VER
{
public:
	static MainScene* createScene()
	{
		return MainScene::create();
	}
	CREATE_FUNC(MainScene);


	Layer*	m_rankTableLayer;
	Sprite* m_rankBG;

	bool isProgress;
	bool isRestored;


	MainScene();
	~MainScene();
	virtual bool init();
	virtual void onExitTransitionDidStart();


	void callbackOnPushed_startMenuItem(Ref* pSender);
	void callbackOnPushed_buyMenuItem(Ref* pSender);
	void callbackLockBtn(Ref* sender);
	void callbackOnPushed_rankMenuItem(Ref* pSender);
	void callbackRankPrev(Ref* pSender);
	void callbackRankNext(Ref* pSender);
	void callbackOnPushed_resetMenuItem(Ref* pSender);

	void drawOnlineRank(int level, bool retryOnEmpty = true);
	void showNameInputDialog();

	int m_rankLevel = 3;
	std::shared_ptr<bool> m_aliveFlag;

#ifdef LITE_VER
	virtual void productFetchComplete();
	virtual void productPurchased(std::string productId);
	virtual void transactionCanceled();
	virtual void restorePreviousTransactions(int count);
#endif //LITE_VER
	
};

