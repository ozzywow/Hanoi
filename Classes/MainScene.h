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
	Node*   m_rankBG;

	bool isProgress;
	bool isRestored;


	MainScene();
	~MainScene();
	virtual bool init();
	virtual void onExitTransitionDidStart();


	void callbackOnPushed_startMenuItem(Ref* pSender);
	void callbackOnPushed_buyMenuItem(Ref* pSender);
	void callbackLockBtn(Ref* sender);
	void callbackRankPrev(Ref* pSender);
	void callbackRankNext(Ref* pSender);
	void callbackOnPushed_resetMenuItem(Ref* pSender);

	void drawOnlineRank(int level, bool retryOnEmpty = true);
	void showNameInputDialog();

	int m_rankLevel = 3;
	std::shared_ptr<bool> m_aliveFlag;

	Label*               m_nameplateLabel   = nullptr;
	std::string          m_playerDisplayName;
	std::vector<std::string> m_greetingTexts;
	int                  m_greetingIndex    = 1;
	bool                 m_showingPlayerName = false;
	bool                 m_showNative       = true;
	void showNextNameplateText();
	void showResultDialog(const std::string& title, Color3B titleColor, const std::string& msg);

#ifdef LITE_VER
	virtual void productFetchComplete();
	virtual void productPurchased(std::string productId);
	virtual void transactionCanceled();
	virtual void restorePreviousTransactions(int count);
#endif //LITE_VER
	
};

