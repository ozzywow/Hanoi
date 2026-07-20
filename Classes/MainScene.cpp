#include "stdafx.h"
#include <algorithm>
#include <sstream>
#include "MainScene.h"
#include "SoundFactory.h"
#include "PlayScene.h"
#include "UserDataManager.h"
#include "LeaderboardManager.h"
#include "ui/CocosGUI.h"
#include "DrawUtils.h"
#include "PixelFont.h"
#ifdef LITE_VER
#include "IAPManager.h"
#endif //LITE_VER

#include "MainSceneInternal.h"

static const Color3B MINT_C(80, 220, 180);

// c(patch) 업데이트의 소극적 안내는 앱 실행당 1회(최초 진입)만. static이라 MainScene 재진입에도 유지,
// 앱 콜드 재시작 시 리셋. (b=권장은 진입마다, a=강제는 항상)
static bool s_patchPromptShownThisRun = false;

// START 색상 순환 팔레트 (톤다운: 순색 회피). 흰→녹→파→빨→앰버→퍼플.
static const Color3B START_PALETTE[] = {
	Color3B(210, 215, 225),  // 소프트 화이트
	Color3B(100, 215, 140),  // 그린
	Color3B(110, 175, 255),  // 블루
	Color3B(255, 110, 110),  // 레드
	Color3B(255, 200,  95),  // 앰버
	Color3B(200, 150, 255),  // 퍼플
};
static const int START_PALETTE_N = sizeof(START_PALETTE) / sizeof(START_PALETTE[0]);
static const float START_PX = 3.5f;

MainScene::MainScene()
{
	m_rankTableLayer = NULL;
	m_rankBG = NULL;
}

MainScene::~MainScene()
{
}

bool MainScene::init()
{
	if (!Scene::init())
	{
		return false;
	}

	static bool s_preloaded = false;
	if (!s_preloaded) {
		SoundFactory::Instance()->preloadAll();
		s_preloaded = true;
	}

	static bool s_introBgmPlayed = false;
	if (!s_introBgmPlayed) {
		SoundFactory::Instance()->play("bgm_intro", true, true, false);
		s_introBgmPlayed = true;
	}

	this->isProgress = false;
	this->isRestored = false;
	m_aliveFlag = std::make_shared<bool>(true);

	Sprite* backGround = Sprite::create("NewUI/title_bg.png") ;
	if (backGround)
	{
		backGround->setAnchorPoint(Point::ZERO);
		this->addChild(backGround, tagBG, tagBG);
	}
	

	// ── START: 와이드 네온 키캡 + 외곽선 LED 텍스트 (메인 CTA) ──
	const Vec2  START_POS(75, 160);
	const float SK_W = 132, SK_H = 46;

	auto startKeycap = DrawNode::create();
	drawKeycap(startKeycap, 0, 0, SK_W, SK_H);
	startKeycap->setPosition(START_POS);
	this->addChild(startKeycap, tagInfoText);

	// (외곽 펄스 글로우 제거 — 색 순환/연출이 시선 유도, 키캡 테두리 톤 통일)

	// 가장 많이 누르는 버튼 → 픽셀아트로 강조. 키캡 대비 작게(여백 확보) + 색상 순환.
	m_startColorIdx = rand() % START_PALETTE_N;
	Node* startText = makePixelText("START", START_PX, START_PALETTE[m_startColorIdx], &m_startPixels);
	auto startMenuItem = MenuItemLabel::create(startText,
		CC_CALLBACK_1(MainScene::callbackOnPushed_startMenuItem, this));
	startMenuItem->setPosition(START_POS);
	Menu* mainMenu = Menu::create(startMenuItem, NULL);
	mainMenu->setPosition(Vec2::ZERO);
	this->addChild(mainMenu, tagInfoText, tagInfoText);

	// 무지개(4초) → 스파클(2초) 무한 체이닝 시작
	playStartGradient();

	std::string name = UserDataManager::Instance()->GetUserName();

	// LED 전광판 상수
	const Vec2 LED_POS(75, 205);
	const float LP_W = 128, LP_H = 16;

	// ── ⚙ 설정 키캡 (네임플레이트 우측 밀착, 1/4 크기) ──
	// 네임플레이트 우측끝(75+64=139) + gap(1) + 반폭(7) = 147
	const Vec2 GEAR_POS(146, 197);   // 네임플레이트 하단(205-8=197)에 기어 하강
	auto gearKeycap = DrawNode::create();
	drawKeycap(gearKeycap, 0, 0, 14, 14, true); // 회색 스타일
	gearKeycap->setPosition(GEAR_POS);
	this->addChild(gearKeycap, tagInfoText);

	Node* gearText = makeVecIcon(14, 14, [](DrawNode* dn, float cx, float cy) {
		drawVecGear(dn, cx, cy, 4.5f, Color4F(0.55f, 0.58f, 0.62f, 1.f)); // 회색
	}); // ⚙ 소형 벡터 기어
	auto gearMenuItem = MenuItemLabel::create(gearText, [this](Ref*) {
		SoundFactory::Instance()->play("efs_click");
		this->showSettingsMenu();
	});
	gearMenuItem->setPosition(GEAR_POS);
	Menu* gearMenu = Menu::create(gearMenuItem, NULL);
	gearMenu->setPosition(Vec2::ZERO);
	this->addChild(gearMenu, tagInfoText, tagInfoText);
	// 1. 패널 배경 + 디스플레이 프레임
	auto ledBg = DrawNode::create();
	setupLedBackground(ledBg, LP_W, LP_H, 6.0f);
	ledBg->setPosition(LED_POS);
	this->addChild(ledBg, 1);

	// 2. 이름/인삿말 전광판 라벨
	m_playerDisplayName = name.empty() ? "PLAYER" : name;
	m_nameplateLabel = Label::createWithSystemFont(m_playerDisplayName, "Arial", 16);
	m_nameplateLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
	m_nameplateLabel->setColor(Color3B(255, 215, 0));
	m_nameplateLabel->enableOutline(Color4B(0, 0, 0, 200), 1);
	m_nameplateLabel->setPosition(LED_POS);
	this->addChild(m_nameplateLabel, 2);

	// 디바이스 언어 감지 → 자국어 인삿말 우선 배치
	{
		auto langType = Application::getInstance()->getCurrentLanguage();
		std::string nativeLang;
		switch (langType) {
			case LanguageType::KOREAN:     nativeLang = "KOREAN";     break;
			case LanguageType::JAPANESE:   nativeLang = "JAPANESE";   break;
			case LanguageType::CHINESE:    nativeLang = "CHINESE";    break;
			case LanguageType::FRENCH:     nativeLang = "FRENCH";     break;
			case LanguageType::SPANISH:    nativeLang = "SPANISH";    break;
			case LanguageType::GERMAN:     nativeLang = "GERMAN";     break;
			case LanguageType::ITALIAN:    nativeLang = "ITALIAN";    break;
			case LanguageType::PORTUGUESE: nativeLang = "PORTUGUESE"; break;
			case LanguageType::RUSSIAN:    nativeLang = "RUSSIAN";    break;
			case LanguageType::DUTCH:      nativeLang = "DUTCH";      break;
			case LanguageType::TURKISH:    nativeLang = "TURKISH";    break;
			case LanguageType::NORWEGIAN:  nativeLang = "NORWEGIAN";  break;
			case LanguageType::POLISH:     nativeLang = "POLISH";     break;
			case LanguageType::HUNGARIAN:  nativeLang = "HUNGARIAN";  break;
			case LanguageType::ARABIC:     nativeLang = "ARABIC";     break;
			default:                        nativeLang = "ENGLISH";    break;
		}

		std::string nativeText;
		auto data = FileUtils::getInstance()->getValueVectorFromFile("greetings.plist");
		for (auto& v : data) {
			auto& d = v.asValueMap();
			if (d["lang"].asString() == nativeLang)
				nativeText = d["text"].asString();
			else
				m_greetingTexts.push_back(d["text"].asString());
		}
		// 자국어를 맨 앞에 삽입, 없으면 "Hello" 기본값
		if (nativeText.empty()) nativeText = "Hello";
		m_greetingTexts.insert(m_greetingTexts.begin(), nativeText);

		// [1..n] 랜덤 셔플 — 자국어(0번)는 고정
		for (int _i = (int)m_greetingTexts.size() - 1; _i > 1; --_i) {
			int _j = 1 + rand() % _i;
			std::swap(m_greetingTexts[_i], m_greetingTexts[_j]);
		}
	}

	m_greetingIndex     = ((int)m_greetingTexts.size() > 1) ? 1 : 0;
	m_showingPlayerName = false;
	m_showNative        = true;
	showNextNameplateText();

	// 3. LED 도트 그리드
	auto dimDots = DrawNode::create();
	setupLedDotOverlay(dimDots, LP_W, LP_H);
	dimDots->setPosition(LED_POS);
	this->addChild(dimDots, 3);

	// ── 최상단 LED 전광판 (TopInfoBar) — 레벨별 1위 스크롤 (우→좌) ──
	{
		const float TW = RESOURCE_WIDTH - 4, TH = 15;
		const Vec2 TPOS(RESOURCE_WIDTH / 2, RESOURCE_HEIGHT - TH / 2 - 2);
		auto tBg = DrawNode::create();
		setupLedBackground(tBg, TW, TH, 3.0f);
		tBg->setPosition(TPOS);
		this->addChild(tBg, 10);
		m_topTickerLabel = Label::createWithSystemFont("", "Arial", 10);
		m_topTickerLabel->setColor(Color3B::WHITE);
		m_topTickerLabel->setAnchorPoint(Vec2(0, 0.5f));
		m_topTickerLabel->setPosition(Vec2(RESOURCE_WIDTH + 5, TPOS.y));
		m_topTickerLabel->setVisible(false);
		this->addChild(m_topTickerLabel, 13);
		auto tDots = DrawNode::create();
		setupLedDotOverlay(tDots, TW, TH);
		tDots->setPosition(TPOS);
		this->addChild(tDots, 12);   // dots는 라벨 위에 (LED 효과)

		// 앱 버전 — 타이틀 밑줄 오른쪽. "current/latest" 숫자 + [Update] 버튼(current<latest일 때만 표시/활성).
		// win32 개발빌드는 빈 값이라 생략.
		std::string appVer = Application::getInstance()->getVersion();
		if (!appVer.empty()) {
			// 버전 숫자 라벨 (탭 없음). 우측 앵커 → 화면 오른쪽 끝 기준 우측정렬. 위치/문자/색은 refreshVersionLabel에서.
			m_versionLabel = Label::createWithSystemFont(appVer, "Arial", 9);
			m_versionLabel->setAnchorPoint(Vec2(1.0f, 0.5f));
			this->addChild(m_versionLabel, 13);

			// [Update] 버튼 (탭→스토어). current<latest일 때만 표시/활성.
			auto upLbl = Label::createWithSystemFont("[Update]", "Arial", 10);
			upLbl->setColor(Color3B(255, 210, 40));   // 노랑(앰버)
			m_updateItem = MenuItemLabel::create(upLbl, [this](Ref*) {
				SoundFactory::Instance()->play("efs_click");
				Application::getInstance()->openURL(appStoreUrl());
			});
			m_updateItem->setAnchorPoint(Vec2(1.0f, 0.5f));   // 우측 앵커 → 화면 오른쪽 끝 기준 우측정렬
			m_updateItem->setVisible(false);
			auto upMenu = Menu::create(m_updateItem, nullptr);
			upMenu->setPosition(Vec2::ZERO);
			this->addChild(upMenu, 13);

			refreshVersionLabel();              // 초기 상태(위치/문자/색 세팅. 최신버전 미도착이면 current만)
			refreshVersionLabelWhenReady();     // 최신버전 지연 도착 시 확보되는 순간 갱신
		}
	}
	// ── 최하단 LED 전광판 (BottomInfoBar) — 국기 스크롤 (좌→우) ──
	{
		const float BW = RESOURCE_WIDTH - 4, BH = 15;
		const Vec2 BPOS(RESOURCE_WIDTH / 2, BH / 2 + 2);
		auto bBg = DrawNode::create();
		setupLedBackground(bBg, BW, BH, 3.0f);
		bBg->setPosition(BPOS);
		this->addChild(bBg, 10);
		m_botTickerLabel = Label::createWithSystemFont("", "Arial", 10);
		m_botTickerLabel->setColor(Color3B::WHITE);
		m_botTickerLabel->setAnchorPoint(Vec2(0, 0.5f));
		m_botTickerLabel->setPosition(Vec2(-5, BPOS.y));
		m_botTickerLabel->setVisible(false);
		this->addChild(m_botTickerLabel, 13);
		auto bDots = DrawNode::create();
		setupLedDotOverlay(bDots, BW, BH);
		bDots->setPosition(BPOS);
		this->addChild(bDots, 12);   // dots는 라벨 위에 (LED 효과)
	}

#ifdef LITE_VER
	drawBgmPlayer();   // SetDelegate는 drawBgmPlayer 내부에서 호출
#else
	drawBgmPlayer();   // 비LITE 빌드에서도 카세트 표시
#endif //LITE_VER
	

	m_rankTableLayer = Layer::create();
	this->addChild(m_rankTableLayer, tagInfoText, tagInfoText);

	if (UserDataManager::Instance()->GetUserName().empty()) {
		// first_play_seen: 첫판을 완료했거나 SKIP(포기)한 적 있으면 다시 강제하지 않음(무한 루프 방지).
		bool firstPlaySeen = UserDefault::getInstance()->getBoolForKey("first_play_seen", false);
		if (!UserDataManager::Instance()->HasPendingSubmit() && !firstPlaySeen) {
			// 완전 신규 유저 — 첫판(3레벨) 자동 전환 "전에" 업데이트 강제 여부 먼저 확인.
			// 강제 업데이트면 차단(플레이 진입 안 함), 아니면 첫판으로 전환.
			// fetchTitleConfig가 내부에서 로그인까지 처리하고 실패해도 콜백은 항상 호출(fail-open).
			// 단, 오프라인 로그인 타임아웃(~수십초) 동안 타이틀에 묶이지 않도록 4초 폴백 진행.
			auto alive = m_aliveFlag;
			auto done  = std::make_shared<bool>(false);
			auto go = [this, alive, done]() {
				if (!alive || !*alive || *done) return;
				*done = true;
				this->unschedule("firstPlayFallback");
				// 첫판(3레벨)으로 진행하는 동작 — 업데이트 없거나 권장/소극 창을 닫으면 실행.
				auto alive2 = alive;
				auto proceed = [this, alive2]() {
					if (!alive2 || !*alive2) return;
					SoundFactory::Instance()->play("efs_turn_playScene");
					Director::getInstance()->replaceScene(
						TransitionFade::create(0.3f, PlayScene::createScene(3, true)));
				};
				auto lm = LeaderboardManager::Instance();
				if (lm->needsForceUpdate()) {          // a — 차단(플레이 진입 안 함)
					showUpdateDialog(true);
					return;
				}
				// 최초 실행이므로 b(권장)/c(소극) 모두 여기서 안내 → 닫으면 첫판으로.
				// 단 "다시 묻지 않기"로 옵트아웃한 버전이면 안내 생략하고 바로 첫판.
				if (!lm->isOptionalUpdateSuppressed()) {
					if (lm->hasRecommendedUpdate()) {      // b
						m_updatePromptShown = true;
						showUpdateDialog(false, proceed);
						return;
					}
					if (lm->hasPatchUpdate()) {            // c — 앱 실행당 1회
						s_patchPromptShownThisRun = true;
						showUpdateDialog(false, proceed);
						return;
					}
				}
				proceed();                             // 업데이트 없음/옵트아웃 → 바로 첫판
			};
			LeaderboardManager::Instance()->fetchTitleConfig([go]() { go(); });
			// 응답 지연/오프라인 대비: 4초 내 미도착이면 판정 못 해도 첫판 진행(fail-open).
			this->scheduleOnce([go](float) { go(); }, 4.0f, "firstPlayFallback");
			return true;
		}
		// 신규 강제 첫판(위 예외)이 아닌, 이름 없는 모든 MainScene 진입 → 이름 입력.
		// (첫판 클리어 후 복귀 / SKIP 후 복귀 / 이후 이름 없이 재진입 전부 포함)
		// 강제 업데이트가 도착하면 fetchTitleConfig 콜백/showUpdateDialog가 이 창을 닫음.
		showNameInputDialog();
	}

	LeaderboardManager::Instance()->login([](bool ok) {
		log("PlayFab startup login: %s", ok ? "OK" : "FAIL");
	});

	int level = UserDataManager::Instance()->GetLevel();
	m_rankLevel = level;
	SoundFactory::Instance()->play("efs_click");
	this->drawOnlineRank(level);

	// 진입 시 공개 Title Data(마스터 스위치 + 공지) 재조회 → 값이 바뀐 경우에만 UI 갱신.
	// 티커 자체는 아래에서 캐시값으로 이미 즉시 구성되므로, 여기선 도착한 신규 값만 반영.
	{
		auto alive = m_aliveFlag;
		int  lv    = level;
		bool        prevEnabled = LeaderboardManager::Instance()->isAwardEnabled();
		std::string prevNotice  = LeaderboardManager::Instance()->getNotice();
		std::string prevLatest  = LeaderboardManager::Instance()->getLatestVersion();
		LeaderboardManager::Instance()->fetchTitleConfig(
			[this, alive, lv, prevEnabled, prevNotice, prevLatest]() {
				if (!alive || !*alive) return;
#ifdef ENABLE_AWARD_COMMENT
				// 마스터 스위치가 바뀐 경우에만 재그리기(불필요한 재그림 방지)
				if (LeaderboardManager::Instance()->isAwardEnabled() != prevEnabled) {
					LeaderboardManager::Instance()->invalidateComments(lv);
					drawOnlineRank(lv);
				}
#else
				(void)lv; (void)prevEnabled;
#endif
				// 공지 변경 또는 업데이트 안내(latest_version 지연 도착)가 생기면 상단 티커 갱신.
				refreshVersionLabel();   // latest_version 도착 → 버전 라벨을 current/latest 형태로 갱신
				{
					bool noticeChanged =
						LeaderboardManager::Instance()->getNotice() != prevNotice;
					// latest_version이 콜드스타트 후 처음 도착(prevLatest 비었다가 채워짐)했고
					// 공지가 없어 업데이트 안내가 티커에 떠야 하면 재시작. (warm start 중복 재시작 방지)
					std::string        cur    = Application::getInstance()->getVersion();
					const std::string& latest = LeaderboardManager::Instance()->getLatestVersion();
					bool updateJustArrived =
						prevLatest.empty() && !latest.empty()
						&& LeaderboardManager::Instance()->getNotice().empty()
						&& !cur.empty()
						&& LeaderboardManager::compareVersion(cur, latest) < 0;
					if (noticeChanged || updateJustArrived)
						startTopTicker();
				}
				// 버전 게이트 — 서버 조회 성공분으로만 판정(오프라인 오차단 방지).
				// a=강제(항상, 이름창까지 닫음) / b=권장(진입마다) / c=소극적(앱 실행당 1회).
				auto lm = LeaderboardManager::Instance();
				if (lm->needsForceUpdate()) {          // a — 차단
					showUpdateDialog(true);
					return;
				}
				// 권장/소극적 안내는 이름 입력창이 없을 때만(겹침 방지).
				if (this->getChildByTag(199) != nullptr) return;
				// "다시 묻지 않기"로 이 버전을 옵트아웃한 경우 안내 생략.
				if (lm->isOptionalUpdateSuppressed()) return;
				if (lm->hasRecommendedUpdate()) {      // b — 이 MainScene 진입당 1회
					if (!m_updatePromptShown) {
						m_updatePromptShown = true;
						showUpdateDialog(false);
					}
				} else if (lm->hasPatchUpdate()) {     // c — 앱 실행당 1회(최초 진입만)
					if (!s_patchPromptShownThisRun) {
						s_patchPromptShownThisRun = true;
						showUpdateDialog(false);
					}
				}
			});
	}

	// 방금 이름 등록한 경우 PlayFab 전파 지연 보상을 위해 2초 후 캐시 무효화 + 재갱신
	if (UserDataManager::Instance()->m_justRegistered) {
		UserDataManager::Instance()->m_justRegistered = false;
		int lv = level;
		scheduleOnce([this, lv](float) {
			LeaderboardManager::Instance()->invalidateCache(lv);
			drawOnlineRank(lv);
#ifdef ENABLE_AWARD_COMMENT
			// 첫판/이름 등록 직후 pending 점수가 제출됐으므로, Top10이면 로컬 리플레이도 업로드.
			// (일반 신기록은 아래 m_justGotNewRecord 분기가 담당 — 두 분기는 상호배타적)
			checkAndPromptAward(lv);
#endif
		}, 2.0f, "refreshRank");
	}

	// 신기록 갱신 직후 — PlayFab 전파 지연 보상을 위해 2초 후 캐시 무효화 + 재갱신
	if (UserDataManager::Instance()->m_justGotNewRecord) {
		UserDataManager::Instance()->m_justGotNewRecord = false;
		int lv = level;
		scheduleOnce([this, lv](float) {
			LeaderboardManager::Instance()->invalidateCache(lv);
			drawOnlineRank(lv);
#ifdef ENABLE_AWARD_COMMENT
			checkAndPromptAward(lv);   // rank 확정 후 Top10이면 리플레이 업로드(수상소감 자동팝업은 제거됨)
#endif
		}, 2.0f, "refreshNewRecord");
	}

	// 캐시된 공지(직전 실행 저장분)로 상단 티커를 네트워크 대기 없이 즉시 구성.
	// 신규 값이 늦게 도착하면 위 fetchTitleConfig 콜백이 바뀐 경우에만 다시 갱신.
	startTopTicker();
	startBotTicker();

	return true;
}


void MainScene::onExitTransitionDidStart()
{
	if (m_aliveFlag) *m_aliveFlag = false;

#ifdef LITE_VER
	this->isProgress = false;
	this->isRestored = false;
	IAPManager::Instance()->ToggleIndicator(false);
#endif //LITE_VER
}



void MainScene::callbackOnPushed_startMenuItem(Ref* pSender)
{

	SoundFactory::Instance()->play("efs_click");

	if (m_rankBG)
	{
		auto action = MoveTo::create(0.3, Vec2(600, 320 - 60));
		m_rankBG->runAction(action);
		scheduleOnce([](float) {
			SoundFactory::Instance()->play("efs_click");
		}, 0.0f, "start_click2");
	}

	int level = m_rankLevel;
	SoundFactory::Instance()->fadeOutBGM(0.5f);
	SoundFactory::Instance()->play("efs_turn_playScene");
	PlayScene* playScene = PlayScene::createScene(level);
	Director::getInstance()->replaceScene(TransitionFade::create(0.3, playScene));
}


void MainScene::callbackOnPushed_buyMenuItem(Ref* pSender)
{
#ifdef LITE_VER
	if (true == isProgress) { return; }
	isProgress = true;
	IAPManager::Instance()->ToggleIndicator(true);
	IAPManager::Instance()->buyFeature(kProductIdTotal);
#endif //LITE_VER

	SoundFactory::Instance()->play("efs_click");
}




void MainScene::callbackLockBtn(Ref* sender)
{
#ifdef LITE_VER
	if (isRestored) { return; }
	if (true == isProgress) { return; }
	isProgress = true;

	IAPManager::Instance()->ToggleIndicator(true);
	IAPManager::Instance()->restorePreviousTransactions();
#endif //LITE_VER

	SoundFactory::Instance()->play("efs_click");
}



#ifdef LITE_VER
void MainScene::productFetchComplete()
{
	cocos2d::log("productFetchComplete");
	IAPManager::Instance()->ToggleIndicator(false);
	isProgress = false;	
}
void MainScene::productPurchased(std::string productId)
{
	cocos2d::log("productPurchased /%s", productId.c_str());
	IAPManager::Instance()->ToggleIndicator(false);
	isProgress = false;

	if (productId == kProductIdTotal)
	{
		UserDataManager::Instance()->SetCart(true);
		UserDataManager::Instance()->SaveUserData();
		drawBgmPlayer();

		showResultDialog("PURCHASE COMPLETE", Color3B(255, 215, 0), "All levels unlocked!");
	}
}
void MainScene::transactionCanceled()
{
	cocos2d::log("transactionCanceled");
	IAPManager::Instance()->ToggleIndicator(false);
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
		showResultDialog("NOTHING TO RESTORE", Color3B(150, 150, 150), "No previous purchase found.");
		return;
	}

	cocos2d::log("restorePreviousTransactions");

	UserDataManager::Instance()->SetCart(true);
	UserDataManager::Instance()->SaveUserData();
	IAPManager::Instance()->ToggleIndicator(false);
	drawBgmPlayer();

	SoundFactory::Instance()->play("efs_click");
	showResultDialog("RESTORE COMPLETE", Color3B(80, 220, 180), "All levels restored!");
}
#endif //LITE_VER

// 팝업 배경에 모달 터치 차단 리스너 부착 — 뒤 영역 터치를 전부 삼켜(통과 방지),
// 팝업 영역(버튼/EditBox 등 더 깊은 자식)만 상호작용되게 한다.
// attachModalBlocker 는 DrawUtils(공용)로 이동 — docs/popup_design_plan.md §2 모달 일원화.

void MainScene::showResultDialog(const std::string& title, Color3B titleColor, const std::string& msg)
{
	SoundFactory::Instance()->play("efs_click");
	const int TAG = 197;
	this->removeChildByTag(TAG);

	const float DW = 220, DH = 110;

	// 공용 프레임 (유틸 톤 = 그레이 테두리). 제목색은 호출자 지정값 유지.
	auto f = makePopupFrame(title, kBorderGray, titleColor, DW, DH, 14.f);
	f.backdrop->setTag(TAG);
	this->addChild(f.backdrop, 999);
	auto dlg = f.box;

	auto msgLabel = Label::createWithSystemFont(msg, "Arial", 12);
	msgLabel->setColor(kTextMuted);
	msgLabel->setPosition(Vec2(DW / 2, DH - 65));
	dlg->addChild(msgLabel);

	auto okBtn = makePopupChipButton("OK", kBtnFunc, [this, TAG](Ref*) {
		SoundFactory::Instance()->play("efs_click");
		this->removeChildByTag(TAG);
	}, 92.f, 34.f, 13.f);
	okBtn->setPosition(Vec2(DW / 2, 20));

	auto menu = Menu::create(okBtn, nullptr);
	menu->setPosition(Vec2::ZERO);
	dlg->addChild(menu);

	dlg->scheduleOnce([this, TAG](float) {
		this->removeChildByTag(TAG);
	}, 3.0f, "autoClose");
}

void MainScene::showNextNameplateText()
{
	if (!m_nameplateLabel || m_greetingTexts.empty()) return;

	std::string text;
	Color3B     color;

	if (!m_showingPlayerName) {
		// 인삿말 슬롯: 자국어↔랜덤 교대
		color = Color3B(160, 220, 255);
		if (m_showNative || (int)m_greetingTexts.size() <= 1) {
			// 자국어
			text = m_greetingTexts[0];
		} else {
			// 랜덤 인삿말
			text = m_greetingTexts[m_greetingIndex];
			++m_greetingIndex;
			if (m_greetingIndex >= (int)m_greetingTexts.size()) {
				for (int _i = (int)m_greetingTexts.size() - 1; _i > 1; --_i) {
					int _j = 1 + rand() % _i;
					std::swap(m_greetingTexts[_i], m_greetingTexts[_j]);
				}
				m_greetingIndex = ((int)m_greetingTexts.size() > 1) ? 1 : 0;
			}
		}
		m_showNative = !m_showNative;  // 다음 슬롯은 반대
		m_showingPlayerName = true;
	} else {
		// 플레이어 이름 표시 (금색)
		text  = m_playerDisplayName;
		color = Color3B(255, 215, 0);
		m_showingPlayerName = false;
	}

	m_nameplateLabel->setString(text);
	m_nameplateLabel->setColor(color);
	m_nameplateLabel->setOpacity(0);
	m_nameplateLabel->stopAllActions();
	m_nameplateLabel->runAction(Sequence::create(
		FadeIn::create(0.2f),
		DelayTime::create(1.4f),
		FadeOut::create(0.2f),
		CallFunc::create([this]() { showNextNameplateText(); }),
		nullptr
	));
}


// START 픽셀 본체를 단색으로 다시 칠함 (연출 종료 후 평상시 색으로 복귀)
static void redrawStartSolid(DrawNode* node, int idx)
{
	if (!node) return;
	const Color3B& c = START_PALETTE[idx];
	node->clear();
	drawPixelGlyphs(node, "START", START_PX,
		Color4F(c.r / 255.f, c.g / 255.f, c.b / 255.f, 1.0f), Vec2::ZERO);
}

void MainScene::tickStartColor()
{
	if (!m_startPixels || m_startFxBusy) return;

	// 무지개 / 픽셀 스파클 교대
	if (rand() % 2) playStartGradient();
	else            playStartSparkle();
}

// 연출1: 가로로 흐르는 무지개 그라데이션 (4초) → 완료 후 스파클 호출
void MainScene::playStartGradient()
{
	if (!m_startPixels) return;
	m_startFxBusy = true;
	m_startPixels->stopAllActions();
	const float DUR = 4.0f;
	auto fx = ActionFloat::create(DUR, 0.f, 1.f, [this](float t) {
		if (!m_startPixels) return;
		m_startPixels->clear();
		drawPixelGlyphsFn(m_startPixels, "START", START_PX, Vec2::ZERO,
			[t](int gx, int gy, float normX) {
				float hue = normX * 0.9f - t * 2.0f;
				return hsv2rgb(hue, 0.7f, 1.0f);
			});
	});
	auto done = CallFunc::create([this]() {
		m_startFxBusy = false;
		playStartSparkle();
	});
	m_startPixels->runAction(Sequence::create(fx, done, nullptr));
}

// 연출2: 각 픽셀이 랜덤색으로 깜빡이며 점등 (2초) → 완료 후 무지개 호출
void MainScene::playStartSparkle()
{
	if (!m_startPixels) return;
	m_startFxBusy = true;
	m_startPixels->stopAllActions();
	const float DUR = 2.0f;
	auto fx = ActionFloat::create(DUR, 0.f, 1.f, [this, DUR](float t) {
		if (!m_startPixels) return;
		int flick = (int)(t * DUR / 0.1f);   // 0.1초마다 재랜덤
		m_startPixels->clear();
		drawPixelGlyphsFn(m_startPixels, "START", START_PX, Vec2::ZERO,
			[flick](int gx, int gy, float normX) {
				uint32_t h = (uint32_t)(gx * 73856093) ^ (uint32_t)(gy * 19349663) ^ (uint32_t)(flick * 83492791);
				h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
				float hue = (h & 0xffff) / 65535.f;
				float val = 0.55f + 0.45f * (((h >> 16) & 0xff) / 255.f);
				return hsv2rgb(hue, 0.85f, val);
			});
	});
	auto done = CallFunc::create([this]() {
		m_startFxBusy = false;
		playStartGradient();
	});
	m_startPixels->runAction(Sequence::create(fx, done, nullptr));
}
