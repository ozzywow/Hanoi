#pragma once

#include "common_define.h"
#include <vector>
#include <memory>
#include "cocos2d.h"
#include "MKStoreManagerDelegate.h"
using namespace cocos2d;

struct LeaderboardEntry;  // 수상소감 카드 표시용 (LeaderboardManager.h)



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

	void drawOnlineRank(int level, bool retryOnEmpty = true);
	void showNameInputDialog();
	void showSettingsMenu();

#ifdef ENABLE_AWARD_COMMENT
	// 랭킹 Top10 수상소감
	void checkAndPromptAward(int level);                                   // 신기록 rank 확정 후 rank≤10이면 입력창
	void showAwardInputDialog(int level, int rank, const std::string& existing);  // 작성/수정 입력창
	void showAwardCardDialog(const LeaderboardEntry& entry, int level);    // 읽기 카드 팝업
#endif

	int m_rankLevel = 3;
	int m_rankGeneration = 0;          // 연속 탭 시 오래된 콜백 무시용
	std::shared_ptr<bool> m_aliveFlag;

	Label* m_topTickerLabel  = nullptr;
	Label* m_botTickerLabel  = nullptr;
	void startTopTicker();
	void tickTopStep();
	void startBotTicker();
	void tickBotStep();

	// BGM Player
	int       m_bgmIndex       = 0;    // 현재 재생 중인 트랙 인덱스 (0-3)
	int       m_bgmSelection   = 0;    // 사용자 선택: 0=Random, 1-4=특정 트랙
	bool      m_bgmPlaying     = false;
	Node*     m_speakerLNode   = nullptr;
	Node*     m_speakerRNode   = nullptr;
	DrawNode* m_playBtnIcon    = nullptr;
	Label*    m_bgmTitleLabel  = nullptr;
	std::string m_topTickerBaseText;
	int         m_topTickerGen = 0;   // 상단 티커 세대 — 늦게 오는 랭킹 콜백이 공지를 덮어쓰지 않게

	void drawBgmPlayer();
	void bgmPlay(int index);
	void bgmPlaySelection();
	void bgmTogglePlayPause();
	void bgmNext();
	void bgmPrev();
	void bgmUpdatePlayBtn();
	void bgmUpdatePlaylistLed();
	void bgmStartSpeakerAnim();
	void bgmStopSpeakerAnim();
	void startBgmTicker(const std::string& title);
	void stopBgmTicker();

	cocos2d::DrawNode*   m_startPixels      = nullptr;  // START 픽셀아트 본체 (색상 순환 시 재드로우)
	int                  m_startColorIdx    = 0;
	bool                 m_startFxBusy      = false;    // 화려한 연출 재생 중 (평상시 페이드 차단)
	void tickStartColor();      // 4초마다: 평상시 단색 페이드 or 가끔 연출
	void playStartGradient();   // 연출1: 무지개 그라데이션 스윕
	void playStartSparkle();    // 연출2: 픽셀별 랜덤색 깜빡임

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

