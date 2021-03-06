#pragma once

#include <vector>
#include "MKStoreManagerDelegate.h"
#include "cocos2d.h"
using namespace cocos2d;

class PlaySceneTouchHandlerLayer;
class Discus;

class PlayScene : public Scene 
#ifdef LITE_VER
	, public MKStoreManagerDelegate
#endif //LITE_VER
{
public:

#ifdef LITE_VER
	bool isProgress;
	bool isRestored;
#endif //LITE_VER

	int									m_countOfDiscus ;
	std::vector< std::vector<Discus*> >	m_pole ;
	std::vector<Discus*>				m_arrDiscurs ;
	Sprite*				m_selectionMark ;
	Sprite*				m_deselectionMark ;
	int					m_moveCount ;
	
	int					m_countDown;	
	
	Label*			m_labelMoveCount ;
	Label*			m_labelLevel ;
	Label*			m_labelTime;
	Label*			m_pCoinLabel;
	
	
	PLAY_STATE		m_isIng;
	int				m_dateTime;	
	Action*			m_actionTimeRun;	
	Layer*			m_discusLayer;	
	Layer*			m_coinLayer;	
	int				m_mastTime;
	
	PlaySceneTouchHandlerLayer* m_touchHanderLayer ;


	static PlayScene* createScene(int numOfDiscus)
	{
		PlayScene* pRes = PlayScene::create();
		pRes->initWithDiscusNum(numOfDiscus);
		return pRes;
	}
	CREATE_FUNC(PlayScene);

	~PlayScene();

	virtual void onExitTransitionDidStart();

	bool	initWithDiscusNum(int numOfDiscus);
	
	void	DrawMenu(bool SoundOpt);
	void	DrawDiscus();
	void	DrawInfoText();
	void	ResetGame();
	void	InitGame();

	Discus*	GetTopDiscus(int poleID);
	bool IsAbleToMoveDiscus(Discus* pDiscus, int poleID);
	bool AttachDiscusToPole(Discus* pDiscus, int poleID);
	void SelectPole(int poleID, bool bIsAble);
	bool CheckSuccess();

	void Start();
	void Finished();
	void DrawTime();
	void CountDown();
	PLAY_STATE GetPlayState();
	
	
	void MessagePopup();
	

	void callbackOnPushed_homeMenuItem(Ref* pSender);
	void callbackOnPushed_resetMenuItem(Ref* pSender);
	void callbackOnPushed_prevMenuItem(Ref* sender);
	void callbackOnPushed_nextMenuItem(Ref* sender);
	void callbackOnPushed_speakerMenuItem(Ref* sender);
	void callbackNeedCoinBtn(Ref* sender) {};	
	void callbackLockBtn(Ref* sender);

#ifdef LITE_VER
	virtual void productFetchComplete();
	virtual void productPurchased(std::string productId);
	virtual void transactionCanceled();
	virtual void restorePreviousTransactions(int count);
#endif //LITE_VER
};


