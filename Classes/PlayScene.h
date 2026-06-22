#pragma once

#include "common_define.h"
#include "LeaderboardManager.h"
#include <vector>
#include <memory>
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
	Label*			m_hudSep         = nullptr;
	Label*			m_hudTickerLabel = nullptr;
	Label*			m_bottomLabelA   = nullptr;
	Label*			m_bottomLabelB   = nullptr;
	Label*			m_bottomLabelC   = nullptr;

	std::shared_ptr<bool> m_aliveFlag;

	
	PLAY_STATE		m_isIng;
	int				m_dateTime;	
	Action*			m_actionTimeRun;
	Layer*			m_discusLayer;
	int				m_mastTime;
	
	PlaySceneTouchHandlerLayer* m_touchHanderLayer ;


	bool m_isFirstPlay = false;
	int  m_popupShownTime = 0;
	bool m_isTransitioning = false;

	static PlayScene* createScene(int numOfDiscus, bool isFirstPlay = false)
	{
		PlayScene* pRes = PlayScene::create();
		pRes->m_isFirstPlay = isFirstPlay;
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
	void	EnterWaitingState();

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
	void callbackLockBtn(Ref* sender);

	void startRankTicker(int level);
	void stopRankTicker();
	void tickerScrollStep();
	void showHudResult(bool isNewRecord, const RecordTime& rt);

	void setBottomPanel(int pole, const std::string& text, Color3B color = Color3B(80, 220, 180), float fontSize = 10.0f);
	void blinkBottomPanel(int pole, float interval = 0.5f);
	void stopBottomPanel(int pole);
	void clearBottomPanels();

	void typingBottomPanel(int pole, const std::string& text, float charInterval = 0.08f, Color3B color = Color3B(80, 220, 180), float fontSize = 10.0f);
	void slideInBottomPanel(int pole, const std::string& text, bool fromLeft = true, Color3B color = Color3B(80, 220, 180), float fontSize = 10.0f);
	void pulseBottomPanel(int pole);
	void colorCycleBottomPanel(int pole);
	void shakeBottomPanel(int pole);
	void demoBottomPanels();

	void startIdleAnimation();
	void stopIdleAnimation();
	void _showNextIdleScene();
	void startGuideAnimation();
	void showPodiumRanking(const std::vector<LeaderboardEntry>& entries);
	void _noRankNextScene(int lv);
	void startCheerAnimation();
	void _showNextCheerScene();

	int m_noRankPhase = 0;

#ifdef LITE_VER
	virtual void productFetchComplete();
	virtual void productPurchased(std::string productId);
	virtual void transactionCanceled();
	virtual void restorePreviousTransactions(int count);
#endif //LITE_VER
};


