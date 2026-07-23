#pragma once

#include "common_define.h"
#include "LeaderboardManager.h"
#include <vector>
#include <memory>
#include <cstdint>
#include "IAPDelegate.h"
#include "cocos2d.h"
using namespace cocos2d;

class PlaySceneTouchHandlerLayer;
class Discus;

// --- 리플레이(터치노트) ---
// 플레이어의 의사결정 3+1종을 타임스탬프와 함께 기록해 재생한다. (docs/replay_ghost_plan.md)
enum ReplayEventType : uint8_t {
	EV_SELECT    = 0,  // 폴 선택, 라이트 ON        (efs_select_disc)
	EV_MOVE_OK   = 1,  // 성공 이동                 (efs_move_disc_ok)
	EV_MOVE_FAIL = 2,  // 이동 실패, 라이트 OFF+실패음 (efs_cancel_select)
	EV_DESELECT  = 3,  // 같은 폴 재탭 = 선택 취소     (efs_move_disc_ok, 이동 없음)
};

struct ReplayEvent {
	uint8_t  type;   // ReplayEventType
	uint8_t  pole;   // 관련 폴 (0~2)
	uint32_t t_ms;   // m_dateTime 기준 경과 ms
};

class PlayScene : public Scene
#ifdef LITE_VER
	, public IAPDelegate
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
	Label*			m_hudTickerLabel = nullptr;
	DrawNode*		m_trafficDot[3]  = {};
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
	std::string m_bgmName;

	// 첫판(강제 3레벨) 탈출 SKIP 버튼 — 30초 경과 / 15초 무활동 / 이동 25수↑ 중 하나면 노출.
	// 포기해도 first_play_seen 저장 → MainScene 재진입 시 강제 첫판 루프 방지.
	bool m_skipShown      = false;
	int  m_lastActivityMs = 0;
	void showFirstPlaySkipButton();

	// 120초 무입력 시 게임 자동 포기 (정상 플레이 아님 + 관전자 정지 착각 방지)
	bool m_idleAbandoned  = false;
	void abandonByIdle();
	void showIdleAbandonPopup();

	// RPM meter
	int    m_rpmTouchCount = 0;
	int    m_rpmStartTime  = 0;
	float  m_bgmCurrentVol = 0.0f;
	float  m_rpmSmoothed   = 0.0f;
	Label* m_labelRPM      = nullptr;
	bool   m_rpmBlinking   = false;
	int    m_finalRPM      = 0;

	// Equalizer
	DrawNode* m_eqNode       = nullptr;
	float     m_eqH[3][6]    = {};
	float     m_eqPeak[3][6] = {};

	// Replay (1차: 내 리플레이 보기)
	std::vector<ReplayEvent> m_replay;
	bool   m_isReplaying        = false;
	int    m_replaySelectedPole = -1;
	bool   m_replayOverflow     = false;  // A-2: 이벤트 캡 초과 → 저장/업로드 불가(기록은 유효)
	int    m_lastReplayEventMs  = 0;      // A-1: 직전 녹화 이벤트 시각(잡음 디바운스용)
	double m_replayClock        = 0.0;   // ms
	size_t m_replayIndex        = 0;
	int    m_replayFinalMs      = 0;     // 현재 재생 중인 리플레이의 최종 시간(저장본은 현재 기록과 다를 수 있음)
	float  m_replaySpeed        = 1.0f;  // 배속 (x0.5~x4)
	int    m_replaySpeedIdx     = 1;     // kReplaySpeeds 인덱스 (기본 x1)
	Menu*  m_replaySpeedMenu    = nullptr;   // 하단 컨트롤 바 전체(일시정지·배속·홈)
	Label* m_replaySpeedLabel   = nullptr;
	Label* m_replaySpeedArrowL  = nullptr;   // 배속 감속 화살표 (경계에서 속 빈 ◁)
	Label* m_replaySpeedArrowR  = nullptr;   // 배속 가속 화살표 (경계에서 속 빈 ▷)
	MenuItemSprite* m_replayPauseItem = nullptr;   // ❚❚PAUSE ↔ ▶PLAY 토글 칩
	bool   m_replayPaused       = false;     // 일시정지 중 → _updateReplay 정지
	void   _buildReplayControlBar();     // 하단 바 [❚❚PAUSE][◀ SPEED xN ▶][■ STOP] 생성
	void   _stepReplaySpeed(int dir);    // 배속 단계 이동(-1/+1) + 글로벌 저장
	void   _updateReplaySpeedArrows();   // 배속 경계에 따라 화살표 채움/비움 갱신
	void   _toggleReplayPause();         // 일시정지 ↔ 재개 토글 (칩 라벨 교체)
	bool   m_lastIsNewRecord    = false; // 재생 종료 후 상단 결과 텍스트 복원용

	// 관전 모드 (2차: 랭커 리플레이 관전) — 외부 blob 주입 후 즉시 재생, 종료 시 MainScene 복귀
	bool        m_isSpectate      = false;
	bool        m_spectateStarted = false;
	std::string m_spectateBlob;
	std::string m_spectateName;
	int         m_spectateRank    = 0;   // 종료 팝업 표시용
	int         m_spectateScoreMs = 0;   // 종료 팝업 표시용(기록 시간)
	std::string m_spectatePlayFabId;     // 관전 대상 id (좋아요 대상, replay_like)
	void startSpectate();

	// 3차: 고스트 레이스 (라이브 플레이 + 진행 HUD, docs §10)
	bool        m_isRace        = false;
	bool        m_isRevenge     = false;          // 복수전(피격 상대에게 재도전) 여부 — 레벨표시에 Revenge/Race 구분
	std::string m_ghostBlob;
	std::string m_ghostName;
	std::string m_ghostPlayFabId;               // 고스트(B) 소유자 id — 격파 기록 대상 (battle_reward P0)
	int         m_ghostRank     = 0;
	int         m_ghostScoreMs  = 0;
	int         m_raceTotalMoves= 0;              // 2^N - 1
	bool        m_ghostFinishedShown = false;     // "GHOST FINISHED" 연출 1회
	std::vector<int> m_ghostReach;                // r(0..total) → 고스트가 처음 남은수≤r 된 t_ms
	cocos2d::DrawNode* m_raceBar        = nullptr;
	cocos2d::Label*    m_raceYouIcon    = nullptr;   // 🏃 (내 진행)
	cocos2d::Label*    m_raceGhostIcon  = nullptr;   // 👻 (고스트 진행)
	cocos2d::Label*    m_raceGapLabel   = nullptr;   // 두 마커 사이 델타값(초)
	float m_youTargetX   = -1.0f;                    // 마커 스프링 이동 목표(가속감속)
	float m_ghostTargetX = -1.0f;
	float m_youVel       = 0.0f;                     // 스프링 속도
	float m_ghostVel     = 0.0f;
	void _setupRace();          // 고스트 테이블 precompute + HUD 생성 (Start에서 호출)
	void _updateRaceHud();      // 델타/마커 갱신 (내 이동 후 + DrawTime)
	int  _movesRemaining();     // 내 보드 distance-to-solved

	static PlayScene* createRaceScene(int level, const std::string& ghostBlob,
	                                  const std::string& name, int rank, int scoreMs,
	                                  const std::string& playFabId = "", bool isRevenge = false)
	{
		PlayScene* p = PlayScene::create();
		p->m_isRace         = true;
		p->m_isRevenge      = isRevenge;
		p->m_ghostBlob      = ghostBlob;
		p->m_ghostName      = name;
		p->m_ghostPlayFabId = playFabId;
		p->m_ghostRank      = rank;
		p->m_ghostScoreMs   = scoreMs;
		p->initWithDiscusNum(level);
		return p;
	}
	void _beginSpectatePlayback();   // 관전 재생 시작(재호출 가능)
	void showSpectateEndPopup();     // 관전 종료 팝업(REPLAY/HOME)

	void recordReplayEvent(ReplayEventType type, int pole);
	void startReplay();
	void _updateReplay(float dt);
	void applyReplayEvent(const ReplayEvent& ev, bool silent = false);
	void skipReplay();
	void endReplay();

	void startEqualizerAnimation();
	void stopEqualizerAnimation();
	void _updateEqualizer(float dt);

	static PlayScene* createScene(int numOfDiscus, bool isFirstPlay = false)
	{
		PlayScene* pRes = PlayScene::create();
		pRes->m_isFirstPlay = isFirstPlay;
		pRes->initWithDiscusNum(numOfDiscus);
		return pRes;
	}

	// 관전 모드 씬 생성 (2차) — 랭커 리플레이 blob 주입, 진입 시 자동 재생
	static PlayScene* createSpectateScene(int level, const std::string& blob,
	                                      const std::string& rankerName,
	                                      int rank = 0, int scoreMs = 0,
	                                      const std::string& targetId = "")
	{
		PlayScene* pRes = PlayScene::create();
		pRes->m_isSpectate       = true;
		pRes->m_spectateBlob     = blob;
		pRes->m_spectateName     = rankerName;
		pRes->m_spectateRank     = rank;
		pRes->m_spectateScoreMs  = scoreMs;
		pRes->m_spectatePlayFabId = targetId;
		pRes->initWithDiscusNum(level);
		return pRes;
	}
	CREATE_FUNC(PlayScene);

	~PlayScene();

	virtual void onExitTransitionDidStart();
	virtual void onEnterTransitionDidFinish() override;

	bool	initWithDiscusNum(int numOfDiscus);
	
	void	DrawMenu(bool SoundOpt);
	void	DrawDiscus();
	void	DrawInfoText();
	void	ResetGame();
	void	InitGame();
	void	EnterWaitingState();

	// 중앙 START 버튼 (NONE 상태 CTA) — 빈 화면 탭으로는 시작되지 않는다
	Menu*	m_startMenu = nullptr;
	void	_buildStartButton();
	void	_removeStartButton();

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

	void setTrafficDot(int i, Color4F color);

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


