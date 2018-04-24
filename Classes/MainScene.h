#pragma once


#include <vector>
#include "cocos2d.h"
using namespace cocos2d;



class  MainScene : public Scene
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
	
};

