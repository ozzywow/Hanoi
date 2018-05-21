#pragma once


#include <vector>
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
//CCScene* m_nationRankingBG;

	std::vector<int>	m_arrPrize;
	

	bool isProgress;
	bool isRestored;


	MainScene();
	~MainScene();	
	virtual bool init();
	/*
	void DrawRank(std::vector<int>& scores);
	void DrawNationRank(std::vector<std::string>& nationRank);
	void DrawPrize(std::vector<int>& scores);
	void requestScore(int level);
	void requestPrize();
	*/
	

	void callbackOnPushed_startMenuItem(Ref* pSender);	
	void callbackOnPushed_buyMenuItem(Ref* pSender);
	void callbackLockBtn(Ref* sender);

#ifdef LITE_VER
	virtual void productFetchComplete();
	virtual void productPurchased(std::string productId);
	virtual void transactionCanceled();
	virtual void restorePreviousTransactions(int count);
#endif //LITE_VER
	
};

