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
#include "MKStoreManager_cpp.h"
#endif //LITE_VER

static const Color3B MINT_C(80, 220, 180);

// BGM Player ─────────────────────────────────────────────────────────────────
struct BgmTrack { const char* file; const char* title; };
// 선택 순서: selection 1=Space, 2=Universe, 3=Cosmos, 4=Nova, 5=Moon, 6=Earth
static const BgmTrack s_bgmTracks[] = {
    {"bgm_space",    "The Space"},
    {"bgm_universe", "The Universe"},
    {"bgm_cosmos",   "The Cosmos"},
    {"bgm_nova",     "The Nova"},
    {"bgm_moon",     "The Moon"},
    {"bgm_earth",    "The Earth"},
};
static const int BGM_TRACK_COUNT = (int)(sizeof(s_bgmTracks) / sizeof(s_bgmTracks[0]));
static const int BGM_SEL_COUNT   = BGM_TRACK_COUNT + 1;   // 0=Random + 8 tracks
static const char* KEY_BGM_INDEX = "bgm_player_index";
static const char* KEY_BGM_SEL   = "bgm_selection";       // 0=Random, 1-4=specific
static const char* s_selNames[]  = {
    "RANDOM",
    "\xe2\x99\xaa THE SPACE",
    "\xe2\x99\xaa THE UNIVERSE",
    "\xe2\x99\xaa THE COSMOS",
    "\xe2\x99\xaa THE NOVA",
    "\xe2\x99\xaa THE MOON",
    "\xe2\x99\xaa THE EARTH",
};
static const int TAG_CASSETTE = 78;

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
		if (!UserDataManager::Instance()->HasPendingSubmit()) {
			// 완전 신규 유저 — 첫판 플레이 후 이름 입력
			scheduleOnce([](float) {
				SoundFactory::Instance()->play("efs_turn_playScene");
				Director::getInstance()->replaceScene(
					TransitionFade::create(0.3f, PlayScene::createScene(3, true)));
			}, 0.0f, "firstPlay");
			return true;
		}
		// 첫판 완료, 이름 미입력 — 이름 입력창 표시
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
		LeaderboardManager::Instance()->fetchTitleConfig(
			[this, alive, lv, prevEnabled, prevNotice]() {
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
				// 공지가 바뀐 경우에만 상단 티커 갱신(네트워크 지연 도착분/변경 반영)
				if (LeaderboardManager::Instance()->getNotice() != prevNotice)
					startTopTicker();
			});
	}

	// 방금 이름 등록한 경우 PlayFab 전파 지연 보상을 위해 2초 후 캐시 무효화 + 재갱신
	if (UserDataManager::Instance()->m_justRegistered) {
		UserDataManager::Instance()->m_justRegistered = false;
		int lv = level;
		scheduleOnce([this, lv](float) {
			LeaderboardManager::Instance()->invalidateCache(lv);
			drawOnlineRank(lv);
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
			checkAndPromptAward(lv);   // rank 확정 후 Top10이면 수상소감 입력창
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
	CMKStoreManager::Instance()->ToggleIndicator(false);
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
	CMKStoreManager::Instance()->ToggleIndicator(true);
	CMKStoreManager::Instance()->buyFeature(kProductIdTotal);
#endif //LITE_VER

	SoundFactory::Instance()->play("efs_click");
}




void MainScene::callbackLockBtn(Ref* sender)
{
#ifdef LITE_VER
	if (isRestored) { return; }
	if (true == isProgress) { return; }
	isProgress = true;

	CMKStoreManager::Instance()->ToggleIndicator(true);
	CMKStoreManager::Instance()->restorePreviousTransactions();
#endif //LITE_VER

	SoundFactory::Instance()->play("efs_click");
}



#ifdef LITE_VER
void MainScene::productFetchComplete()
{
	cocos2d::log("productFetchComplete");
	CMKStoreManager::Instance()->ToggleIndicator(false);
	isProgress = false;	
}
void MainScene::productPurchased(std::string productId)
{
	cocos2d::log("productPurchased /%s", productId.c_str());
	CMKStoreManager::Instance()->ToggleIndicator(false);
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
	CMKStoreManager::Instance()->ToggleIndicator(false);
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
	CMKStoreManager::Instance()->ToggleIndicator(false);
	drawBgmPlayer();

	SoundFactory::Instance()->play("efs_click");
	showResultDialog("RESTORE COMPLETE", Color3B(80, 220, 180), "All levels restored!");
}
#endif //LITE_VER


void MainScene::callbackRankPrev(Ref* pSender)
{
	if (m_rankLevel <= 3) return;
	SoundFactory::Instance()->play("efs_click");
	--m_rankLevel;
	int lv = m_rankLevel;
	scheduleOnce([this, lv](float) { drawOnlineRank(lv); }, 0.0f, "rankPrev");
}

void MainScene::callbackRankNext(Ref* pSender)
{
	if (m_rankLevel >= MAX_PLAY_LEVEL) return;
	SoundFactory::Instance()->play("efs_click");
	++m_rankLevel;
	int lv = m_rankLevel;
	scheduleOnce([this, lv](float) { drawOnlineRank(lv); }, 0.0f, "rankNext");
}

// ────────────────────────────────────────────────────────────────────────────
// drawOnlineRank  ─  첫 진입: 패널 슬라이드 인 / 재진입(◀▶): LED 내용 전환
// ────────────────────────────────────────────────────────────────────────────
// UTF-8 문자열을 최대 maxCP 코드포인트까지 자른다 (경계 유지). 이름/소감 truncate 공용.
static std::string utf8TruncateCP(const std::string& s, int maxCP)
{
	int cp = 0; size_t i = 0;
	while (i < s.size() && cp < maxCP) {
		unsigned char c = (unsigned char)s[i];
		size_t adv = 1;
		if      (c >= 0xF0) adv = 4;
		else if (c >= 0xE0) adv = 3;
		else if (c >= 0xC0) adv = 2;
		i += adv; ++cp;
	}
	if (i > s.size()) i = s.size();
	return s.substr(0, i);
}

#ifdef ENABLE_AWARD_COMMENT
// writeAwardComment/clientFilterComment reason → 영어 안내
static std::string awardReasonMessage(const std::string& r)
{
	if (r == "empty")     return "Please enter a message";
	if (r == "too_long")  return StringUtils::format("Max %d characters", LeaderboardManager::AWARD_MAX_CP);
	if (r == "link")      return "Links are not allowed";
	if (r == "profanity") return "Inappropriate language";
	if (r == "network")   return "Network error, try again";
	if (r == "disabled")  return "Comments are currently unavailable";
	return "Failed, please try again";
}

// UGC 신고/문의 수신 이메일 (App Store 심사 1.2: 신고 수단 + 연락처)
static const char* AWARD_SUPPORT_EMAIL = "ozzywow2@gmail.com";

// mailto용 퍼센트 인코딩 (unreserved 문자만 통과)
static std::string urlEncode(const std::string& s)
{
	static const char* HEX = "0123456789ABCDEF";
	std::string out;
	out.reserve(s.size() * 3);
	for (unsigned char c : s) {
		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
			(c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
			out += (char)c;
		} else {
			out += '%';
			out += HEX[c >> 4];
			out += HEX[c & 0xF];
		}
	}
	return out;
}

// 문제 소감 신고 — 기본 메일 클라이언트로 사전 작성된 신고 메일 열기.
// 심사관이 요구하는 "신고 수단 + 개발자 연락처"를 버튼 하나로 동시 충족.
static void openAwardReportEmail(int level, int rank,
                                 const std::string& playFabId, const std::string& comment)
{
	std::string subject = StringUtils::format("[Hanoi] Report award comment (L%02d #%d)", level, rank);
	std::string body = StringUtils::format(
		"I'd like to report the following winning speech as inappropriate.\n\n"
		"Level: %d\nRank: %d\nUser ID: %s\nComment: \"%s\"\n\n"
		"Reason (please describe):\n",
		level, rank, playFabId.c_str(), comment.c_str());
	std::string url = std::string("mailto:") + AWARD_SUPPORT_EMAIL
		+ "?subject=" + urlEncode(subject)
		+ "&body="    + urlEncode(body);
	Application::getInstance()->openURL(url);
}
#endif // ENABLE_AWARD_COMMENT

void MainScene::drawOnlineRank(int level, bool retryOnEmpty)
{
	this->unschedule("rankRetry");

	const float PX = 188, PY = 22;   // 좌우 여백 균형 (좌 ~9px ↔ 우 ~8px)
	const float PW = 284, PH = 200;
	const float HDR_H = 34;
	const float DATA_H = PH - HDR_H;

	// 패널 내부 가변 요소 태그
	const int TAG_D_NAV   = 151;   // 내비 메뉴 (◀▶)
	const int TAG_D_TITLE = 152;   // LEVEL N 타이틀
	const int TAG_D_ROWS  = 153;   // 데이터 행 컨테이너 (LED 전환 대상)

	++m_rankGeneration;
	int  gen   = m_rankGeneration;
	auto alive = m_aliveFlag;

	// ── 헤더 내 가변 요소 빌드 헬퍼 (공통: 첫 진입 / 재진입 모두 호출) ──
	auto buildHeader = [=](Node* panel) {
		float hdrMidY = DATA_H + HDR_H / 2.f;

		panel->removeChildByTag(TAG_D_TITLE);
		auto title = Label::createWithSystemFont(
			StringUtils::format("LEVEL  %d", level), "Arial", 14);
		title->setColor(Color3B(255, 215, 0));
		title->setAnchorPoint(Vec2(0.5f, 0.5f));
		title->setPosition(Vec2(PW / 2, hdrMidY + 5));
		panel->addChild(title, 4, TAG_D_TITLE);

		panel->removeChildByTag(TAG_D_NAV);
		bool canPrev = (level > 3), canNext = (level < MAX_PLAY_LEVEL);
		const float BTN_W = 22, BTN_H = 22;
		auto pn = makeVecNavKeycap(BTN_W, BTN_H, true,  canPrev);
		auto pb = MenuItemLabel::create(pn, CC_CALLBACK_1(MainScene::callbackRankPrev, this));
		pb->setEnabled(canPrev); pb->setPosition(Vec2(55, hdrMidY));
		auto nn = makeVecNavKeycap(BTN_W, BTN_H, false, canNext);
		auto nb = MenuItemLabel::create(nn, CC_CALLBACK_1(MainScene::callbackRankNext, this));
		nb->setEnabled(canNext); nb->setPosition(Vec2(PW - 55, hdrMidY));
		auto nav = Menu::create(pb, nb, nullptr);
		nav->setPosition(Vec2::ZERO);
		panel->addChild(nav, 4, TAG_D_NAV);
	};

	// ── 데이터 행 빌드 헬퍼 (콜백 안에서 호출) ──
	// ledScan=true 이면 행별 FadeIn 스캔 효과 적용
	auto buildRows = [=](Node* panel, const std::vector<LeaderboardEntry>& entries, bool ledScan) {
		panel->removeChildByTag(TAG_D_ROWS);
		auto rowsNode = Node::create();

		if (entries.empty()) {
			const char* msg = retryOnEmpty ? "Refreshing..." : "No records yet";
			auto lbl = Label::createWithSystemFont(msg, "Arial", 12);
			lbl->setColor(Color3B(180, 180, 180));
			lbl->setAnchorPoint(Vec2(0.5f, 0.5f));
			lbl->setPosition(Vec2(PW / 2, DATA_H / 2 - 10));
			if (ledScan) { lbl->setOpacity(0); lbl->runAction(FadeIn::create(0.15f)); }
			rowsNode->addChild(lbl, 2);
			panel->addChild(rowsNode, 2, TAG_D_ROWS);
			if (retryOnEmpty) {
				this->scheduleOnce([this, alive, level](float) {
					if (!alive || !*alive) return;
					drawOnlineRank(level, false);
				}, 2.0f, "rankRetry");
			}
			return;
		}

		const std::string myId = LeaderboardManager::Instance()->getPlayFabId();
		const float FIRST_ROW_Y = DATA_H - 30, ROW_STEP = 14;

		for (int i = 0; i < (int)entries.size(); ++i) {
			const auto& e  = entries[i];
			RecordTime   rt = getRecordTime(e.scoreMs);
			float y         = FIRST_ROW_Y - i * ROW_STEP;
			bool  isMe      = !myId.empty() && (e.playFabId == myId);
			Color3B rowCol  = isMe ? Color3B(255, 215, 0) : Color3B::WHITE;
			int rowFont     = isMe ? 13 : 12;
			float scanDelay = ledScan ? (0.07f + i * 0.035f) : 0.0f;

			auto addLbl = [&](Label* lbl) {
				if (ledScan) {
					lbl->setOpacity(0);
					lbl->runAction(Sequence::create(
						DelayTime::create(scanDelay),
						FadeIn::create(0.06f), nullptr));
				}
				rowsNode->addChild(lbl, 2);
			};

			auto rnk = Label::createWithSystemFont(
				StringUtils::format("%d", e.rank), "Arial", rowFont);
			rnk->setAnchorPoint(Vec2(0, 0.5f)); rnk->setPosition(Vec2(12, y));
			rnk->setColor(rowCol); addLbl(rnk);

			if (!e.countryCode.empty()) {
				auto flg = Label::createWithSystemFont(
					countryToFlag(e.countryCode), "Arial", rowFont + 1);
				flg->setAnchorPoint(Vec2(0, 0.5f)); flg->setPosition(Vec2(32, y));
				addLbl(flg);
			}

			auto nm = Label::createWithSystemFont(
				utf8TruncateCP(e.displayName, 10), "Arial", rowFont);  // 바이트 아닌 코드포인트로 자름 (한글 깨짐 방지)
			nm->setAnchorPoint(Vec2(0, 0.5f)); nm->setPosition(Vec2(55, y));
			nm->setColor(rowCol); addLbl(nm);

			auto tm = Label::createWithSystemFont(
				StringUtils::format("%02d:%02d.%02d", rt.min, rt.sec, rt.ms), "Arial", rowFont);
			tm->setAnchorPoint(Vec2(1.0f, 0.5f)); tm->setPosition(Vec2(PW - 10, y));
			tm->setColor(rowCol); addLbl(tm);

#ifdef ENABLE_AWARD_COMMENT
			// ── 수상소감: 이름~시간 사이. 다 들어가면 전문, 초과 시 코드포인트 말줄임 ──
			{
				float nameRight = 55.f + nm->getContentSize().width;
				float timeLeft  = (PW - 10.f) - tm->getContentSize().width;
				float availL    = nameRight + 6.f;
				float availW    = timeLeft - 6.f - availL;
				if (availW > 12.f) {
					if (!e.comment.empty()) {
						auto cmt = Label::createWithSystemFont(e.comment, "Arial", rowFont - 1);
						cmt->setAnchorPoint(Vec2(0, 0.5f));
						cmt->setColor(isMe ? Color3B(255, 235, 150) : Color3B(150, 200, 180));
						if (cmt->getContentSize().width > availW) {
							int cp = LeaderboardManager::utf8Length(e.comment);
							while (cp > 1 && cmt->getContentSize().width > availW) {
								--cp;
								cmt->setString(utf8TruncateCP(e.comment, cp) + "...");
							}
						}
						cmt->setPosition(Vec2(availL, y));
						addLbl(cmt);
					} else if (isMe && LeaderboardManager::Instance()->isAwardEnabled()) {
						auto pen = Label::createWithSystemFont("✎", "Arial", rowFont);  // ✎ 미작성 힌트
						pen->setAnchorPoint(Vec2(0, 0.5f));
						pen->setColor(Color3B(255, 215, 0));
						pen->setPosition(Vec2(availL, y));
						addLbl(pen);
					}
				}
			}
#endif // ENABLE_AWARD_COMMENT

			if (isMe) {
				float blinkDelay = ledScan ? (scanDelay + 0.06f) : 0.0f;
				auto makeBlink = [blinkDelay](Label* lbl) {
					auto doBlink = CallFunc::create([lbl]() {
						lbl->runAction(RepeatForever::create(Sequence::create(
							DelayTime::create(0.4f),
							FadeTo::create(0.1f, 60),
							DelayTime::create(0.4f),
							FadeTo::create(0.1f, 255),
							nullptr)));
					});
					lbl->runAction(Sequence::create(
						DelayTime::create(blinkDelay),
						doBlink,
						nullptr));
				};
				makeBlink(rnk);
				makeBlink(nm);
				makeBlink(tm);
			}
		}

#ifdef ENABLE_AWARD_COMMENT
		// 행 탭 → 소감 카드 (소감 있는 행) / 내 미작성 행 탭 → 입력창
		{
			auto entriesCopy = entries;
			auto touchLs = EventListenerTouchOneByOne::create();
			touchLs->setSwallowTouches(true);
			touchLs->onTouchBegan =
				[this, entriesCopy, rowsNode, FIRST_ROW_Y, ROW_STEP, PW, level](Touch* t, Event*) -> bool {
					Vec2 p = rowsNode->convertToNodeSpace(t->getLocation());
					if (p.x < 10 || p.x > PW - 10) return false;
					for (int i = 0; i < (int)entriesCopy.size(); ++i) {
						float ry = FIRST_ROW_Y - i * ROW_STEP;
						if (p.y >= ry - ROW_STEP / 2 && p.y <= ry + ROW_STEP / 2) {
							const auto& e = entriesCopy[i];
							std::string myId = LeaderboardManager::Instance()->getPlayFabId();
							bool isMe = !myId.empty() && e.playFabId == myId;
							if (!e.comment.empty()) { showAwardCardDialog(e, level); return true; }
							if (isMe && LeaderboardManager::Instance()->isAwardEnabled()) {
								showAwardInputDialog(level, e.rank, e.comment); return true;
							}
							return false;
						}
					}
					return false;
				};
			Director::getInstance()->getEventDispatcher()
				->addEventListenerWithSceneGraphPriority(touchLs, rowsNode);
		}
#endif // ENABLE_AWARD_COMMENT

		panel->addChild(rowsNode, 2, TAG_D_ROWS);
	};

	// ══════════════════════════════════════════════════════════════════
	// [A] 재진입 경로: 패널 프레임 유지, 내용만 LED 전환
	// ══════════════════════════════════════════════════════════════════
	if (m_rankBG) {
		auto* panel = m_rankBG;

		// 기존 행 LED 페이드 아웃 후 제거
		if (auto* old = panel->getChildByTag(TAG_D_ROWS)) {
			old->runAction(Sequence::create(
				FadeOut::create(0.10f),
				RemoveSelf::create(), nullptr));
		}

		// 헤더 즉시 업데이트 (레벨/내비)
		buildHeader(panel);

		// 새 데이터 비동기 fetch → LED 스캔 인
		LeaderboardManager::Instance()->fetchLeaderboard(level, 10,
			[this, alive, gen, level, panel, buildRows](const std::vector<LeaderboardEntry>& entries) {
				if (!alive || !*alive || gen != m_rankGeneration) return;
				buildRows(panel, entries, true);   // ledScan = true
			});

		return;
	}

	// ══════════════════════════════════════════════════════════════════
	// [B] 첫 진입: 로딩 표시 → 데이터 수신 후 패널 슬라이드 인
	// ══════════════════════════════════════════════════════════════════
	const int TAG_LOADING = 50;
	auto loading = Label::createWithSystemFont("Loading...", "Arial", 14);
	loading->setColor(Color3B(180, 180, 180));
	loading->setPosition(Vec2(PX + PW / 2, PY + PH / 2));
	m_rankTableLayer->addChild(loading, 0, TAG_LOADING);

	LeaderboardManager::Instance()->fetchLeaderboard(level, 10,
		[this, alive, gen, level, PX, PY, PW, PH, HDR_H, DATA_H,
		 buildHeader, buildRows, TAG_LOADING]
		(const std::vector<LeaderboardEntry>& entries) {
			if (!alive || !*alive || gen != m_rankGeneration) return;

			m_rankTableLayer->removeChildByTag(TAG_LOADING);
			m_rankTableLayer->removeChildByTag(tagBG);

			// 패널 생성 + 슬라이드 인
			auto panel = LayerColor::create(Color4B(0, 0, 0, 0), PW, PH);
			panel->setPosition(Vec2(RESOURCE_WIDTH + PW, PY));
			panel->runAction(MoveTo::create(0.3f, Vec2(PX, PY)));
			m_rankBG = panel;
			m_rankTableLayer->addChild(panel, tagBGRankingBoard, tagBGRankingBoard);

			// 영구 구조 요소
			auto dataBg = DrawNode::create();
			setupLedBackground(dataBg, PW, DATA_H, 0.f);
			dataBg->setPosition(Vec2(PW / 2, DATA_H / 2));
			panel->addChild(dataBg, 0);

			auto dataDots = DrawNode::create();
			setupLedDotOverlay(dataDots, PW, DATA_H, 3.0f, 0.5f);
			dataDots->setPosition(Vec2(PW / 2, DATA_H / 2));
			panel->addChild(dataDots, 1);

			auto headerBg = DrawNode::create();
			headerBg->drawSolidRect(Vec2(0, DATA_H), Vec2(PW, PH),
				Color4F(0.05f, 0.08f, 0.20f, 1.0f));
			panel->addChild(headerBg, 0);

			auto border = DrawNode::create();
			border->drawRect(Vec2(0, 0), Vec2(PW, PH),
				Color4F(0.31f, 0.86f, 0.70f, 0.85f));
			border->drawLine(Vec2(0, DATA_H), Vec2(PW, DATA_H),
				Color4F(0.31f, 0.86f, 0.70f, 0.85f));
			panel->addChild(border, 6);

			float hdrMidY = DATA_H + HDR_H / 2.f;
			auto subTitle = Label::createWithSystemFont("ONLINE  RANK", "Arial", 8);
			subTitle->setColor(Color3B(160, 160, 200));
			subTitle->setAnchorPoint(Vec2(0.5f, 0.5f));
			subTitle->setPosition(Vec2(PW / 2, hdrMidY - 8));
			panel->addChild(subTitle, 4);

			// 영구 컬럼 헤더 + 구분선 (레벨이 바뀌어도 변하지 않음)
			float colHdrY = DATA_H - 12;
			auto addColHdr = [&](const char* t, float x, bool ra) {
				auto lbl = Label::createWithSystemFont(t, "Arial", 9);
				lbl->setColor(Color3B(130, 130, 150));
				lbl->setAnchorPoint(ra ? Vec2(1.0f, 0.5f) : Vec2(0, 0.5f));
				lbl->setPosition(Vec2(x, colHdrY));
				panel->addChild(lbl, 2);
			};
			addColHdr("#",    12,      false);
			addColHdr("NAME", 50,      false);
			addColHdr("TIME", PW - 10, true);

			auto colDiv = DrawNode::create();
			colDiv->drawLine(Vec2(10, DATA_H - 20), Vec2(PW - 10, DATA_H - 20),
				Color4F(0.3f, 0.5f, 0.5f, 0.5f));
			panel->addChild(colDiv, 2);

			// 헤더 가변 요소 (레벨 타이틀 + 내비)
			buildHeader(panel);

			// 데이터 행 (첫 진입은 스캔 없이 바로 표시)
			buildRows(panel, entries, false);
		});
}

// 팝업 배경에 모달 터치 차단 리스너 부착 — 뒤 영역 터치를 전부 삼켜(통과 방지),
// 팝업 영역(버튼/EditBox 등 더 깊은 자식)만 상호작용되게 한다.
static void attachModalBlocker(Node* backdrop)
{
	auto blockLs = EventListenerTouchOneByOne::create();
	blockLs->setSwallowTouches(true);
	blockLs->onTouchBegan = [](Touch*, Event*) -> bool { return true; };
	Director::getInstance()->getEventDispatcher()
		->addEventListenerWithSceneGraphPriority(blockLs, backdrop);
}

void MainScene::showResultDialog(const std::string& title, Color3B titleColor, const std::string& msg)
{
	SoundFactory::Instance()->play("efs_click");
	const int TAG = 197;
	this->removeChildByTag(TAG);

	const float DW = 220, DH = 110;

	auto backdrop = LayerColor::create(Color4B(0, 0, 0, 0));
	backdrop->setTag(TAG);
	this->addChild(backdrop, 999);
	backdrop->runAction(FadeTo::create(0.15f, 150));
	attachModalBlocker(backdrop);

	auto dlg = LayerColor::create(Color4B(10, 15, 50, 230), DW, DH);
	dlg->setPosition(Vec2((RESOURCE_WIDTH - DW) / 2, (RESOURCE_HEIGHT - DH) / 2));
	dlg->setScale(0.7f);
	backdrop->addChild(dlg);
	dlg->runAction(Sequence::create(
		ScaleTo::create(0.15f, 1.05f),
		ScaleTo::create(0.08f, 1.0f),
		nullptr
	));

	auto outline = DrawNode::create();
	outline->drawRect(Vec2(-1, -1), Vec2(DW + 1, DH + 1), Color4F(0.75f, 0.85f, 1.0f, 0.85f));
	outline->drawRect(Vec2(0, 0),   Vec2(DW, DH),          Color4F(0.80f, 0.90f, 1.0f, 1.0f));
	outline->drawRect(Vec2(1, 1),   Vec2(DW - 1, DH - 1),  Color4F(0.75f, 0.85f, 1.0f, 0.85f));
	dlg->addChild(outline);

	auto titleLabel = Label::createWithSystemFont(title, "Arial", 14);
	titleLabel->setColor(titleColor);
	titleLabel->setPosition(Vec2(DW / 2, DH - 22));
	dlg->addChild(titleLabel);

	auto divider = DrawNode::create();
	divider->drawLine(Vec2(15, DH - 40), Vec2(DW - 15, DH - 40), Color4F(0.5f, 0.5f, 0.5f, 0.8f));
	dlg->addChild(divider);

	auto msgLabel = Label::createWithSystemFont(msg, "Arial", 12);
	msgLabel->setColor(Color3B(200, 200, 200));
	msgLabel->setPosition(Vec2(DW / 2, DH - 65));
	dlg->addChild(msgLabel);

	auto okLabel = Label::createWithSystemFont("  OK  ", "Arial", 13);
	okLabel->setColor(Color3B(255, 215, 0));
	auto okBtn = MenuItemLabel::create(okLabel, [this, TAG](Ref*) {
		SoundFactory::Instance()->play("efs_click");
		this->removeChildByTag(TAG);
	});
	okBtn->setPosition(Vec2(DW / 2, 18));

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

static std::vector<std::string>& getRandomNamePool() {
	static std::vector<std::string> pool;
	if (pool.empty()) {
		std::string content = FileUtils::getInstance()->getStringFromFile("random_names.txt");
		std::istringstream ss(content);
		std::string line;
		while (std::getline(ss, line)) {
			while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' '))
				line.pop_back();
			if (!line.empty())
				pool.push_back(line);
		}
		if (pool.empty()) {
			pool = {"NOVA","ACE","BLAZE","ECHO","PIXEL","SWIFT","STORM","VORTEX","FLASH","NEXUS"};
		}
	}
	return pool;
}

// 이름 입력 EditBox 델리게이트 — 엔터를 빈칸으로 누르면 랜덤 이름 자동 입력
class NameInputDelegate : public Ref, public cocos2d::ui::EditBoxDelegate {
public:
	cocos2d::ui::EditBox* box = nullptr;
	void editBoxEditingDidBegin(cocos2d::ui::EditBox*) override {}
	void editBoxEditingDidEndWithAction(cocos2d::ui::EditBox*, EditBoxEndAction) override {}
	void editBoxTextChanged(cocos2d::ui::EditBox*, const std::string&) override {}
	void editBoxReturn(cocos2d::ui::EditBox*) override {
		if (box && strlen(box->getText()) < 3) {
			auto& pool = getRandomNamePool();
			box->setText(pool[rand() % pool.size()].c_str());
		}
	}
};
static Ref* s_nameDelegate = nullptr;

void MainScene::showNameInputDialog()
{
	SoundFactory::Instance()->play("efs_click");
	const int DIALOG_TAG = 199;
	const float DW = 250, DH = 130;

	auto backdrop = LayerColor::create(Color4B(0, 0, 0, 0));
	backdrop->setTag(DIALOG_TAG);
	this->addChild(backdrop, 999);
	backdrop->runAction(FadeTo::create(0.2f, 160));
	attachModalBlocker(backdrop);

	auto dlg = LayerColor::create(Color4B(10, 15, 50, 230), DW, DH);
	dlg->setPosition(Vec2((RESOURCE_WIDTH - DW) / 2, (RESOURCE_HEIGHT - DH) / 2+40));
	dlg->setScale(0.7f);
	backdrop->addChild(dlg);
	dlg->runAction(Sequence::create(
		ScaleTo::create(0.15f, 1.05f),
		ScaleTo::create(0.08f, 1.0f),
		nullptr
	));

	auto titleLabel = Label::createWithSystemFont("SET YOUR NAME FOR RANKING", "Arial", 14);
	titleLabel->setColor(Color3B(255, 215, 0));
	titleLabel->setPosition(Vec2(DW / 2, DH - 22));
	dlg->addChild(titleLabel);

	auto divider = DrawNode::create();
	divider->drawLine(Vec2(15, DH - 40), Vec2(DW - 15, DH - 40), Color4F(0.5f, 0.5f, 0.5f, 0.8f));
	dlg->addChild(divider);

	auto editBoxBg = DrawNode::create();
	editBoxBg->drawSolidRect(Vec2(8, DH - 84), Vec2(DW - 8, DH - 56),
		Color4F(0.05f, 0.07f, 0.15f, 0.9f));
	editBoxBg->drawRect(Vec2(8, DH - 84), Vec2(DW - 8, DH - 56),
		Color4F(0.4f, 0.4f, 0.6f, 0.7f));
	dlg->addChild(editBoxBg);

	auto editBox = cocos2d::ui::EditBox::create(Size(DW - 20, 24),
		cocos2d::ui::Scale9Sprite::create());
	editBox->setFont("Arial", 13);
	editBox->setFontColor(Color3B::WHITE);
	editBox->setPlaceholderFontColor(Color3B(180, 180, 180));
	editBox->setPlaceHolder("3-12 chars");
	editBox->setMaxLength(12);
	editBox->setInputMode(cocos2d::ui::EditBox::InputMode::SINGLE_LINE);
	editBox->setReturnType(cocos2d::ui::EditBox::KeyboardReturnType::DONE);
	editBox->setAnchorPoint(Vec2(0, 0.5f));
	editBox->setPosition(Vec2(10, DH - 70));
	dlg->addChild(editBox);

	// 엔터→랜덤이름 델리게이트
	if (s_nameDelegate) { s_nameDelegate->release(); s_nameDelegate = nullptr; }
	auto nd = new NameInputDelegate();
	nd->box = editBox;
	s_nameDelegate = nd;  // ref count=1, static이 소유
	editBox->setDelegate(nd);

	// 다이얼로그 애니메이션 후 키보드 자동 표시
	dlg->scheduleOnce([editBox](float) {
		editBox->openKeyboard();
	}, 0.35f, "autoKb");

	// 욕설/링크 필터 거부 메시지 표시용
	auto status = Label::createWithSystemFont("", "Arial", 10);
	status->setColor(Color3B(255, 120, 120));
	status->setPosition(Vec2(DW / 2, 36));
	dlg->addChild(status);

	auto okLabel = Label::createWithSystemFont("  OK  ", "Arial", 15);
	okLabel->setColor(Color3B(100, 100, 100));
	// 재진입(더블탭) 방지 플래그 — 검증 성공 시 scene 전환하므로 중복 실행 차단
	auto submitting = std::make_shared<bool>(false);
	auto okBtn = MenuItemLabel::create(okLabel, [this, editBox, status, submitting](Ref*) {
		if (*submitting) return;
		std::string name = editBox->getText();
		*submitting = true;
		status->setColor(Color3B(180, 180, 180));
		status->setString("Checking...");

		auto alive = m_aliveFlag;
		// 클라 경량 필터 + 서버 banned_words 검증 → 통과 시에만 이름 확정
		LeaderboardManager::Instance()->validateName(name,
			[this, alive, name, status, submitting](bool ok, const std::string& reason) {
				if (!alive || !*alive) return;
				if (!ok) {
					*submitting = false;
					status->setColor(Color3B(255, 120, 120));
					status->setString(
						reason == "profanity" ? "Inappropriate name" :
						reason == "link"      ? "Links are not allowed" :
						reason == "too_long"  ? "Max 12 characters" :
						                        "Please choose another name");
					return;
				}

				std::string nameCopy = name;  // SetUserName은 non-const ref를 받음
				UserDataManager::Instance()->SetUserName(nameCopy);
				UserDataManager::Instance()->SaveUserData();

				auto* ud = UserDataManager::Instance();
				int lv = ud->m_pendingSubmitLevel;
				int tm = ud->m_pendingSubmitTime;
				bool hasPending = ud->HasPendingSubmit();
				ud->ClearPendingSubmit();

				// updateDisplayName 완료 → submitScore 완료 → replaceScene 순서로 체이닝
				LeaderboardManager::Instance()->updateDisplayName(name, [lv, tm, hasPending](bool) {
					auto goToMain = []() {
						UserDataManager::Instance()->m_justRegistered = true;
						Director::getInstance()->replaceScene(
							TransitionFade::create(0.3f, MainScene::createScene()));
					};
					if (hasPending) {
						LeaderboardManager::Instance()->submitScore(lv, tm, [goToMain](bool) {
							goToMain();
						});
					} else {
						goToMain();
					}
				});
			});
	});
	okBtn->setEnabled(false);

	dlg->schedule([editBox, okBtn, okLabel](float) {
		bool valid = strlen(editBox->getText()) >= 3;
		okBtn->setEnabled(valid);
		okLabel->setColor(valid ? Color3B::YELLOW : Color3B(100, 100, 100));
	}, 0.05f, CC_REPEAT_FOREVER, 0.f, "nameCheck");

	auto menu = Menu::create(okBtn, nullptr);
	menu->setPosition(Vec2(DW / 2, 18));
	dlg->addChild(menu);
}

#ifdef ENABLE_AWARD_COMMENT
// ─────────────────────────────────────────────────────────────
//  랭킹 Top10 수상소감
// ─────────────────────────────────────────────────────────────

// 신기록 rank 확정 후 — 내 순위가 Top10이면 입력창 표시
void MainScene::checkAndPromptAward(int level)
{
	auto alive = m_aliveFlag;
	LeaderboardManager::Instance()->fetchLeaderboard(level, 10,
		[this, alive, level](const std::vector<LeaderboardEntry>& entries) {
			if (!alive || !*alive) return;
			if (!LeaderboardManager::Instance()->isAwardEnabled()) return;  // 마스터 OFF → 작성창 생략
			std::string myId = LeaderboardManager::Instance()->getPlayFabId();
			if (myId.empty()) return;
			for (const auto& e : entries) {
				if (e.playFabId == myId) {
					if (e.rank >= 1 && e.rank <= 10)
						showAwardInputDialog(level, e.rank, e.comment);
					return;
				}
			}
		});
}

// 수상소감 작성/수정 입력창
void MainScene::showAwardInputDialog(int level, int rank, const std::string& existing)
{
	SoundFactory::Instance()->play("efs_click");
	const int TAG = 193;
	this->removeChildByTag(TAG);
	const float DW = 264, DH = 152;

	auto backdrop = LayerColor::create(Color4B(0, 0, 0, 0));
	backdrop->setTag(TAG);
	this->addChild(backdrop, 999);
	backdrop->runAction(FadeTo::create(0.2f, 160));

	auto dlg = LayerColor::create(Color4B(10, 15, 50, 235), DW, DH);
	dlg->setPosition(Vec2((RESOURCE_WIDTH - DW) / 2, (RESOURCE_HEIGHT - DH) / 2 + 40));
	dlg->setScale(0.7f);
	backdrop->addChild(dlg);
	dlg->runAction(Sequence::create(
		ScaleTo::create(0.15f, 1.05f), ScaleTo::create(0.08f, 1.0f), nullptr));

	// 모달 — 박스 안 빈 곳은 삼키기만, 박스 밖(배경) 탭하면 닫힘.
	// (EditBox/POST 버튼은 더 깊은 자식이라 먼저 터치를 받아 정상 동작)
	auto closeLs = EventListenerTouchOneByOne::create();
	closeLs->setSwallowTouches(true);
	closeLs->onTouchBegan = [this, TAG, dlg, DW, DH](Touch* t, Event*) -> bool {
		Vec2 p = dlg->convertToNodeSpace(t->getLocation());
		if (p.x >= 0 && p.x <= DW && p.y >= 0 && p.y <= DH)
			return true;  // 박스 내부 — 삼키기만 (닫지 않음)
		SoundFactory::Instance()->play("efs_click");
		this->removeChildByTag(TAG);
		return true;       // 박스 밖(배경) — 닫기
	};
	Director::getInstance()->getEventDispatcher()
		->addEventListenerWithSceneGraphPriority(closeLs, backdrop);

	auto outline = DrawNode::create();
	outline->drawRect(Vec2(0, 0), Vec2(DW, DH), Color4F(1.0f, 0.84f, 0.0f, 0.9f));
	dlg->addChild(outline);

	auto titleLabel = Label::createWithSystemFont(
		StringUtils::format("CONGRATULATIONS!  LEVEL %d  ·  RANK %d", level, rank), "Arial", 13);
	titleLabel->setColor(Color3B(255, 215, 0));
	titleLabel->setPosition(Vec2(DW / 2, DH - 20));
	dlg->addChild(titleLabel);

	auto sub = Label::createWithSystemFont("Leave your winning speech to the world", "Arial", 10);
	sub->setColor(Color3B(180, 200, 220));
	sub->setPosition(Vec2(DW / 2, DH - 38));
	dlg->addChild(sub);

	auto editBoxBg = DrawNode::create();
	editBoxBg->drawSolidRect(Vec2(8, DH - 86), Vec2(DW - 8, DH - 58), Color4F(0.05f, 0.07f, 0.15f, 0.9f));
	editBoxBg->drawRect(Vec2(8, DH - 86), Vec2(DW - 8, DH - 58), Color4F(0.4f, 0.4f, 0.6f, 0.7f));
	dlg->addChild(editBoxBg);

	auto editBox = cocos2d::ui::EditBox::create(Size(DW - 20, 24),
		cocos2d::ui::Scale9Sprite::create());
	editBox->setFont("Arial", 13);
	editBox->setFontColor(Color3B::WHITE);
	editBox->setPlaceholderFontColor(Color3B(170, 170, 170));
	editBox->setPlaceHolder(StringUtils::format("Your speech (max %d)",
		LeaderboardManager::AWARD_MAX_CP).c_str());
	// 코드포인트 제한은 아래 스케줄(utf8TruncateCP)이 실제로 강제. maxLength(UTF-16 코드유닛 기준)는
	// 이모지(서로게이트 2유닛)로도 항상 코드포인트 강제가 먼저 걸리도록 여유롭게(=서로게이트 분할 방지).
	editBox->setMaxLength(LeaderboardManager::AWARD_MAX_CP * 3);
	editBox->setInputMode(cocos2d::ui::EditBox::InputMode::SINGLE_LINE);
	editBox->setReturnType(cocos2d::ui::EditBox::KeyboardReturnType::DONE);
	editBox->setAnchorPoint(Vec2(0, 0.5f));
	editBox->setPosition(Vec2(10, DH - 72));
	// 기존 소감이 있으면 그대로, 없으면(신규 작성) 자국어 인삿말을 기본값으로 채움
	if (!existing.empty()) editBox->setText(existing.c_str());
	else                   editBox->setText(getLocalGreeting().c_str());
	dlg->addChild(editBox);

	dlg->scheduleOnce([editBox](float) { editBox->openKeyboard(); }, 0.35f, "awardKb");

	auto counter = Label::createWithSystemFont(
		StringUtils::format("0 / %d", LeaderboardManager::AWARD_MAX_CP), "Arial", 10);
	counter->setColor(Color3B(150, 150, 150));
	counter->setAnchorPoint(Vec2(1.0f, 0.5f));
	counter->setPosition(Vec2(DW - 10, DH - 98));
	dlg->addChild(counter);

	auto status = Label::createWithSystemFont("", "Arial", 10);
	status->setColor(Color3B(255, 120, 120));
	status->setAnchorPoint(Vec2(0, 0.5f));
	status->setPosition(Vec2(10, DH - 98));
	dlg->addChild(status);

	// 코드포인트 제한 강제 + 카운터 갱신 (iOS/Android 공용)
	dlg->schedule([editBox, counter](float) {
		const int MAXCP = LeaderboardManager::AWARD_MAX_CP;
		std::string t = editBox->getText();
		int len = LeaderboardManager::utf8Length(t);
		if (len > MAXCP) {
			editBox->setText(utf8TruncateCP(t, MAXCP).c_str());
			len = MAXCP;
		}
		counter->setString(StringUtils::format("%d / %d", len, MAXCP));
		counter->setColor(len >= MAXCP ? Color3B(255, 180, 80) : Color3B(150, 150, 150));
	}, 0.1f, CC_REPEAT_FOREVER, 0.f, "awardCount");

	// POST
	auto okLabel = Label::createWithSystemFont(" POST ", "Arial", 14);
	okLabel->setColor(Color3B(255, 215, 0));
	auto okBtn = MenuItemLabel::create(okLabel,
		[this, TAG, editBox, status, level, existing](Ref*) {
			std::string text = editBox->getText();
			// 변경 사항 없으면 서버 전송 없이 그냥 닫기
			auto trimws = [](std::string s) -> std::string {
				size_t a = s.find_first_not_of(" \t\r\n");
				if (a == std::string::npos) return std::string();
				size_t b = s.find_last_not_of(" \t\r\n");
				return s.substr(a, b - a + 1);
			};
			if (trimws(text) == trimws(existing)) {
				this->removeChildByTag(TAG);
				return;
			}
			std::string reason;
			if (!LeaderboardManager::clientFilterComment(text, reason)) {
				status->setColor(Color3B(255, 120, 120));
				status->setString(awardReasonMessage(reason));
				return;
			}
			status->setColor(Color3B(180, 180, 180));
			status->setString("Sending...");
			LeaderboardManager::Instance()->writeAwardComment(level, text,
				[this, TAG, status, level](bool ok, const std::string& r) {
					if (ok) {
						this->removeChildByTag(TAG);
						LeaderboardManager::Instance()->invalidateComments(level);
						drawOnlineRank(level);
						// 전파 지연 보상 재갱신
						this->scheduleOnce([this, level](float) {
							LeaderboardManager::Instance()->invalidateComments(level);
							drawOnlineRank(level);
						}, 1.5f, "awardRefresh");
					} else {
						status->setColor(Color3B(255, 120, 120));
						status->setString(awardReasonMessage(r));
					}
				});
		});
	okBtn->setPosition(Vec2(DW / 2, 16));

	auto menu = Menu::create(okBtn, nullptr);
	menu->setPosition(Vec2::ZERO);
	dlg->addChild(menu);
}

// 수상소감 읽기 카드 (탭 시). 내 항목이면 '수정' 버튼 포함. 배경 탭하면 닫힘.
void MainScene::showAwardCardDialog(const LeaderboardEntry& e, int level)
{
	SoundFactory::Instance()->play("efs_click");
	const int TAG = 194;
	this->removeChildByTag(TAG);
	const float DW = 250, DH = 132;

	auto backdrop = LayerColor::create(Color4B(0, 0, 0, 0));
	backdrop->setTag(TAG);
	this->addChild(backdrop, 999);
	backdrop->runAction(FadeTo::create(0.2f, 150));

	// 배경 아무 곳이나 탭하면 닫힘 (수정 버튼 메뉴가 더 높은 우선순위로 먼저 처리됨)
	auto closeLs = EventListenerTouchOneByOne::create();
	closeLs->setSwallowTouches(true);
	closeLs->onTouchBegan = [this, TAG](Touch*, Event*) -> bool {
		SoundFactory::Instance()->play("efs_click");
		this->removeChildByTag(TAG);
		return true;
	};
	Director::getInstance()->getEventDispatcher()
		->addEventListenerWithSceneGraphPriority(closeLs, backdrop);

	auto dlg = LayerColor::create(Color4B(12, 16, 45, 240), DW, DH);
	dlg->setPosition(Vec2((RESOURCE_WIDTH - DW) / 2, (RESOURCE_HEIGHT - DH) / 2));
	dlg->setScale(0.7f);
	backdrop->addChild(dlg);
	dlg->runAction(Sequence::create(
		ScaleTo::create(0.15f, 1.05f), ScaleTo::create(0.08f, 1.0f), nullptr));

	auto outline = DrawNode::create();
	outline->drawRect(Vec2(0, 0), Vec2(DW, DH), Color4F(1.0f, 0.84f, 0.0f, 0.9f));
	dlg->addChild(outline);

	auto rankLbl = Label::createWithSystemFont(
		StringUtils::format("🏆  RANK %d", e.rank), "Arial", 16);
	rankLbl->setColor(Color3B(255, 215, 0));
	rankLbl->setPosition(Vec2(DW / 2, DH - 22));
	dlg->addChild(rankLbl);

	std::string flag = e.countryCode.empty() ? "" : countryToFlag(e.countryCode);
	auto nameLbl = Label::createWithSystemFont(flag + "  " + e.displayName, "Arial", 13);
	nameLbl->setColor(Color3B::WHITE);
	nameLbl->setAnchorPoint(Vec2(0, 0.5f));
	nameLbl->setPosition(Vec2(16, DH - 50));
	dlg->addChild(nameLbl);

	RecordTime rt = getRecordTime(e.scoreMs);
	auto timeLbl = Label::createWithSystemFont(
		StringUtils::format("%02d:%02d.%02d", rt.min, rt.sec, rt.ms), "Arial", 13);
	timeLbl->setColor(Color3B(180, 220, 255));
	timeLbl->setAnchorPoint(Vec2(1.0f, 0.5f));
	timeLbl->setPosition(Vec2(DW - 16, DH - 50));
	dlg->addChild(timeLbl);

	auto divider = DrawNode::create();
	divider->drawLine(Vec2(16, DH - 62), Vec2(DW - 16, DH - 62), Color4F(0.5f, 0.5f, 0.5f, 0.8f));
	dlg->addChild(divider);

	auto cmtLbl = Label::createWithSystemFont("\"" + e.comment + "\"", "Arial", 13);
	cmtLbl->setColor(Color3B(230, 235, 180));
	cmtLbl->setDimensions(DW - 30, 0);
	cmtLbl->setHorizontalAlignment(TextHAlignment::CENTER);
	cmtLbl->setAnchorPoint(Vec2(0.5f, 1.0f));
	cmtLbl->setPosition(Vec2(DW / 2, DH - 70));
	dlg->addChild(cmtLbl);

	// 내 항목이면 수정 버튼, 남의 항목이면 신고 버튼 (App Store 1.2 UGC 신고 수단)
	std::string myId = LeaderboardManager::Instance()->getPlayFabId();
	if (!myId.empty() && e.playFabId == myId) {
		auto editLabel = Label::createWithSystemFont(" EDIT ", "Arial", 12);
		editLabel->setColor(Color3B(120, 220, 255));
		std::string existing = e.comment;
		int rank = e.rank;
		auto editBtn = MenuItemLabel::create(editLabel,
			[this, TAG, level, rank, existing](Ref*) {
				this->removeChildByTag(TAG);
				this->showAwardInputDialog(level, rank, existing);
			});
		editBtn->setPosition(Vec2(DW / 2, 14));
		auto menu = Menu::create(editBtn, nullptr);
		menu->setPosition(Vec2::ZERO);
		dlg->addChild(menu, 5);
	} else {
		auto reportLabel = Label::createWithSystemFont("🚩 Report", "Arial", 12);
		reportLabel->setColor(Color3B(220, 140, 140));
		int rank = e.rank;
		std::string pid = e.playFabId, cmt = e.comment;
		auto reportBtn = MenuItemLabel::create(reportLabel,
			[this, TAG, level, rank, pid, cmt](Ref*) {
				SoundFactory::Instance()->play("efs_click");
				openAwardReportEmail(level, rank, pid, cmt);
				this->removeChildByTag(TAG);
			});
		reportBtn->setPosition(Vec2(DW / 2, 14));
		auto menu = Menu::create(reportBtn, nullptr);
		menu->setPosition(Vec2::ZERO);
		dlg->addChild(menu, 5);
	}
}
#endif // ENABLE_AWARD_COMMENT

void MainScene::showSettingsMenu()
{
	const int TAG = 196;
	if (this->getChildByTag(TAG)) return;
	SoundFactory::Instance()->play("efs_click");

	const float DW = 240, DH = 130.f;

	auto backdrop = LayerColor::create(Color4B(0, 0, 0, 0));
	backdrop->setTag(TAG);
	this->addChild(backdrop, 999);
	backdrop->runAction(FadeTo::create(0.15f, 150));
	attachModalBlocker(backdrop);

	auto dlg = LayerColor::create(Color4B(10, 15, 50, 230), DW, DH);
	dlg->setPosition(Vec2((RESOURCE_WIDTH - DW) / 2, (RESOURCE_HEIGHT - DH) / 2));
	dlg->setScale(0.7f);
	backdrop->addChild(dlg);
	dlg->runAction(Sequence::create(
		ScaleTo::create(0.15f, 1.05f),
		ScaleTo::create(0.08f, 1.0f),
		nullptr));

	auto outline = DrawNode::create();
	outline->drawRect(Vec2(-1, -1), Vec2(DW + 1, DH + 1), Color4F(0.75f, 0.85f, 1.0f, 0.85f));
	outline->drawRect(Vec2(0, 0),   Vec2(DW, DH),          Color4F(0.80f, 0.90f, 1.0f, 1.0f));
	dlg->addChild(outline);

	auto titleLabel = Label::createWithSystemFont("RESET ALL?", "Arial", 15);
	titleLabel->setColor(Color3B(255, 100, 80));
	titleLabel->setPosition(Vec2(DW / 2, DH - 22));
	dlg->addChild(titleLabel);

	auto divider = DrawNode::create();
	divider->drawLine(Vec2(15, DH - 40), Vec2(DW - 15, DH - 40), Color4F(0.5f, 0.5f, 0.5f, 0.8f));
	dlg->addChild(divider);

	auto msg = Label::createWithSystemFont(
		"This will reset your name\nand clear your ranking.",
		"Arial", 12);
	msg->setColor(Color3B(210, 210, 210));
	msg->setAlignment(TextHAlignment::CENTER);
	msg->setPosition(Vec2(DW / 2, DH - 70));
	dlg->addChild(msg);

	auto doReset = [this]() {
		auto* ud = UserDataManager::Instance();
		ud->ResetRecords();
		std::string empty;
		ud->SetUserName(empty);
		ud->SaveUserData();
		LeaderboardManager::Instance()->resetStats();

		if (m_rankBG)
		{
			m_rankBG->removeAllChildren();
			for (int level = 3; level <= MAX_PLAY_LEVEL; ++level)
			{
				int record = ud->GetBestRecord(level);
				RecordTime rt = getRecordTime(record);
				std::string str = StringUtils::format("%02d                                         %02d:%02d.%02d",
					level, rt.min, rt.sec, rt.ms);
				Label* lbl = Label::createWithSystemFont(str, "Arial", 14);
				lbl->setAnchorPoint(Vec2(0, 0));
				lbl->setPosition(Vec2(40, (level * 22) - 20));
				m_rankBG->addChild(lbl);
			}
		}
		showNameInputDialog();
	};

	auto okLabel = Label::createWithSystemFont("OK", "Arial", 14);
	okLabel->setColor(Color3B(255, 80, 80));
	auto okBtn = MenuItemLabel::create(okLabel, [this, TAG, doReset](Ref*) {
		SoundFactory::Instance()->play("efs_click");
		this->removeChildByTag(TAG);
		doReset();
	});
	okBtn->setPosition(Vec2(DW * 0.3f, 22));

	auto cancelLabel = Label::createWithSystemFont("Cancel", "Arial", 14);
	cancelLabel->setColor(Color3B(180, 180, 180));
	auto cancelBtn = MenuItemLabel::create(cancelLabel, [this, TAG](Ref*) {
		SoundFactory::Instance()->play("efs_click");
		this->removeChildByTag(TAG);
	});
	cancelBtn->setPosition(Vec2(DW * 0.7f, 22));

	auto menu = Menu::create(okBtn, cancelBtn, nullptr);
	menu->setPosition(Vec2::ZERO);
	dlg->addChild(menu);
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

// ── TopInfoBar 티커: 레벨 3~10 각 1위 → 우→좌 스크롤 ──────────────────
void MainScene::startTopTicker()
{
	if (!m_topTickerLabel) return;

	// 세대 증가 — 이 호출 이후의 늦게 오는 랭킹 콜백은 폐기(공지가 덮이지 않게).
	const int gen = ++m_topTickerGen;

	// 공지가 있으면 랭킹 스크롤 대신 공지를 표시 (앰버 + 📢). 없으면 기존 Top Players 로직.
	const std::string notice = LeaderboardManager::Instance()->getNotice();
	if (!notice.empty()) {
		m_topTickerBaseText = "\xF0\x9F\x93\xA2 " + notice;   // "📢 " (U+1F4E2) UTF-8
		m_topTickerLabel->setString(m_topTickerBaseText);
		m_topTickerLabel->setColor(Color3B(255, 191, 0));    // 앰버
		m_topTickerLabel->setVisible(true);
		tickTopStep();
		return;
	}
	m_topTickerLabel->setColor(Color3B::WHITE);              // 랭킹 스크롤은 흰색

	auto alive = m_aliveFlag;
	auto results = std::make_shared<std::map<int, std::string>>();
	auto pending = std::make_shared<int>(8);   // level 3~10

	for (int lv = 3; lv <= 10; ++lv) {
		LeaderboardManager::Instance()->fetchLeaderboard(lv, 1,
			[this, alive, gen, results, pending, lv](const std::vector<LeaderboardEntry>& entries) {
				if (!alive || !*alive) return;
				if (gen != m_topTickerGen) return;   // 더 최신 호출(공지 등)로 대체됨 → 폐기
				if (!entries.empty()) {
					const auto& e = entries[0];
					std::string cc = e.countryCode.empty() ? "--" : e.countryCode;
					for (char& c : cc) c = (char)toupper((unsigned char)c);
					(*results)[lv] = StringUtils::format("%dLv %s%s",
						lv, countryToFlag(cc).c_str(), e.displayName.c_str());
				}
				if (--(*pending) == 0) {
					std::string text = "Top Players  -  ";
					bool first = true;
					for (int l = 3; l <= 10; ++l) {
						auto it = results->find(l);
						if (it == results->end()) continue;
						if (!first) text += "   |   ";
						text += it->second;
						first = false;
					}
					if (first) text += "---  BE THE FIRST!  ---";
					if (m_topTickerLabel && gen == m_topTickerGen) {
						m_topTickerBaseText = text;
						m_topTickerLabel->setString(text);
						m_topTickerLabel->setVisible(true);
						tickTopStep();
					}
				}
			});
	}
}

void MainScene::tickTopStep()
{
	if (!m_topTickerLabel || !m_topTickerLabel->getParent()) return;
	const float SPEED = 60.0f;
	float labelW  = m_topTickerLabel->getContentSize().width;
	float startX  = RESOURCE_WIDTH + 5.0f;
	float endX    = -(labelW + 5.0f);
	float duration = (startX - endX) / SPEED;
	const float TH = 15, TICKER_Y = RESOURCE_HEIGHT - TH / 2 - 2;

	m_topTickerLabel->setPositionX(startX);
	m_topTickerLabel->stopAllActions();
	m_topTickerLabel->runAction(Sequence::create(
		MoveTo::create(duration, Vec2(endX, TICKER_Y)),
		CallFunc::create(CC_CALLBACK_0(MainScene::tickTopStep, this)),
		nullptr));
}

// ── BottomInfoBar 티커: 각국 국기 랜덤 나열 → 좌→우 스크롤 ──────────────
void MainScene::startBotTicker()
{
	static const char* COUNTRY_CODES[] = {
		"kr","us","jp","gb","de","fr","it","es","cn","br",
		"ru","au","ca","mx","in","nl","se","no","fi","dk",
		"pl","pt","tr","ar","za","ng","id","ph","sg","th",
		"vn","hk","tw","sa","ae","il","gr","pk","my","ua"
	};
	const int N = (int)(sizeof(COUNTRY_CODES) / sizeof(COUNTRY_CODES[0]));

	// Fisher-Yates 셔플
	std::vector<int> idx(N);
	for (int i = 0; i < N; ++i) idx[i] = i;
	for (int i = N - 1; i > 0; --i) {
		int j = rand() % (i + 1);
		std::swap(idx[i], idx[j]);
	}

	std::string text;
	for (int i = 0; i < N; ++i) {
		std::string cc = COUNTRY_CODES[idx[i]];
		for (char& c : cc) c = (char)toupper((unsigned char)c);
		if (!text.empty()) text += "      ";
		text += countryToFlag(cc);
	}

	if (m_botTickerLabel) {
		m_botTickerLabel->setString(text);
		m_botTickerLabel->setVisible(true);
		tickBotStep();
	}
}

void MainScene::tickBotStep()
{
	if (!m_botTickerLabel || !m_botTickerLabel->getParent()) return;
	const float SPEED = 50.0f;
	float labelW  = m_botTickerLabel->getContentSize().width;
	float startX  = -(labelW + 5.0f);
	float endX    = RESOURCE_WIDTH + 5.0f;
	float duration = (endX - startX) / SPEED;
	const float BH = 15, TICKER_Y = BH / 2 + 2;

	m_botTickerLabel->setPositionX(startX);
	m_botTickerLabel->stopAllActions();
	m_botTickerLabel->runAction(Sequence::create(
		MoveTo::create(duration, Vec2(endX, TICKER_Y)),
		CallFunc::create(CC_CALLBACK_0(MainScene::tickBotStep, this)),
		nullptr));
}

// ── BGM Player 메서드 ─────────────────────────────────────────────────────────

void MainScene::drawBgmPlayer()
{
	auto ud = UserDefault::getInstance();
	m_bgmIndex     = ud->getIntegerForKey(KEY_BGM_INDEX, 0);
	m_bgmSelection = ud->getIntegerForKey(KEY_BGM_SEL,   0);
	m_bgmPlaying   = false;
	m_speakerLNode = m_speakerRNode = nullptr;
	m_playBtnIcon  = nullptr;
	m_bgmTitleLabel = nullptr;

	this->removeChildByTag(TAG_CASSETTE);

	auto root = Node::create();
	root->setPosition(Vec2::ZERO);
	this->addChild(root, tagInfoText, TAG_CASSETTE);

	// 공백A = 공백B = 공백C = 9px (네임플레이트↔START↔카세트↔IAP 균일 간격)
	// IAP_Y=38  : h=22 → y=27..49  (gap↑9 controls, gap↓10 ticker)
	// CTL board : 58..78  (BTN_H=14 +3pad → top=78=LED bottom flush)
	// LED panel : 78.5..89.5  (H=11, top≈cassette bottom flush)
	// Cassette  : 89..133 (H=44, top=133, gap↑9 → START bottom=142, START top=188)
	// START     : 142..188 (SK_H=46, center=165, gap↑9 → NP bottom=197)
	const float CX=75, CY=111, CW=128, CH=44;
	const float SPK_R=17.f, SPK_LX=CX-37, SPK_RX=CX+37;
	const float LED_CY=84, LED_W=CW, LED_H=11;     // 카세트 바로 아래 flush
	const float CTL_Y=68, BTN_W=22, BTN_H=14;      // LED 바로 아래 flush
	const float IAP_Y=38, IBTN_W=50, IBTN_H=22;    // 아이콘 키캡
	const float IBTN_LX=CX-27, IBTN_RX=CX+27;
	const Color4F iconCol(80/255.f,220/255.f,180/255.f,1.f);

	// 1. 카세트 본체
	auto bodyDn = DrawNode::create();
	drawCassetteBody(bodyDn, CX, CY, CW, CH, true);
	root->addChild(bodyDn, 1);

	// 2. 스피커 L/R
	auto spkLDn = DrawNode::create();
	drawSpeakerGrille(spkLDn, SPK_R, true);
	auto spkL = Node::create(); spkL->setPosition(Vec2(SPK_LX, CY)); spkL->addChild(spkLDn);
	root->addChild(spkL, 2); m_speakerLNode = spkL;

	auto spkRDn = DrawNode::create();
	drawSpeakerGrille(spkRDn, SPK_R, true);
	auto spkR = Node::create(); spkR->setPosition(Vec2(SPK_RX, CY)); spkR->addChild(spkRDn);
	root->addChild(spkR, 2); m_speakerRNode = spkR;

	// 3. 플레이리스트 LED 패널 (카세트 바로 아래)
	auto ledBg = DrawNode::create();
	setupLedBackground(ledBg, LED_W, LED_H, 0.f);
	ledBg->setPosition(Vec2(CX, LED_CY));
	root->addChild(ledBg, 2);

	auto titleLbl = Label::createWithSystemFont("", "Arial", 9);
	titleLbl->setAnchorPoint(Vec2(0.5f, 0.5f));
	titleLbl->setPosition(Vec2(CX, LED_CY));
	root->addChild(titleLbl, 3);
	m_bgmTitleLabel = titleLbl;
	bgmUpdatePlaylistLed();

	auto ledDots = DrawNode::create();
	setupLedDotOverlay(ledDots, LED_W, LED_H, 2.0f, 0.4f);
	ledDots->setPosition(Vec2(CX, LED_CY));
	root->addChild(ledDots, 4);

	// 4. 컨트롤러 기판 배경 (LED 바로 아래, 폭=카세트와 동일)
	{
		auto brd = DrawNode::create();
		float bx0=CX-CW/2, bx1=CX+CW/2;
		brd->drawSolidRect(Vec2(bx0, CTL_Y-BTN_H/2-3), Vec2(bx1, CTL_Y+BTN_H/2+3),
			Color4F(0.07f,0.09f,0.11f,0.95f));
		brd->drawRect(Vec2(bx0, CTL_Y-BTN_H/2-3), Vec2(bx1, CTL_Y+BTN_H/2+3),
			Color4F(0.22f,0.45f,0.38f,0.35f));
		// 상단 경계선은 생략 (LED 패널과 맞닿아 있음)
		brd->drawSolidRect(Vec2(bx0, CTL_Y-BTN_H/2-3), Vec2(bx1, CTL_Y-BTN_H/2-2),
			Color4F(0.0f,0.0f,0.0f,0.85f));
		root->addChild(brd, 1);
	}

	// 5. 컨트롤러 버튼 3개: PREV / PLAY / NEXT
	float ic = BTN_H * 0.62f;
	// 좌측 끝 / 중앙 / 우측 끝 — 보드 폭(CW=128) 기준, 3px 내측 여백
	float btnXs[3] = { CX - CW/2 + BTN_W/2 + 3.f, CX, CX + CW/2 - BTN_W/2 - 3.f };

	auto makeCtlBtn = [&](const std::function<void(DrawNode*,float,float)>& iconFn,
	                      DrawNode** outDn) -> Node*
	{
		auto n = Node::create(); n->setContentSize(Size(BTN_W, BTN_H));
		auto dn = DrawNode::create();
		drawKeycap(dn, BTN_W/2, BTN_H/2, BTN_W, BTN_H);
		iconFn(dn, BTN_W/2, BTN_H/2);
		n->addChild(dn);
		if (outDn) *outDn = dn;
		return n;
	};

	auto prevNode = makeCtlBtn([&](DrawNode* d, float x, float y){ drawIconPrev(d,x,y,ic,iconCol); }, nullptr);
	auto prevBtn  = MenuItemLabel::create(prevNode, [this](Ref*){ bgmPrev(); });
	prevBtn->setPosition(Vec2(btnXs[0], CTL_Y));

	auto playNode = makeCtlBtn([&](DrawNode* d, float x, float y){ drawIconPlay(d,x,y,ic,iconCol); },
	                           &m_playBtnIcon);
	auto playBtn  = MenuItemLabel::create(playNode, [this](Ref*){ bgmTogglePlayPause(); });
	playBtn->setPosition(Vec2(btnXs[1], CTL_Y));

	auto nextNode = makeCtlBtn([&](DrawNode* d, float x, float y){ drawIconNext(d,x,y,ic,iconCol); }, nullptr);
	auto nextBtn  = MenuItemLabel::create(nextNode, [this](Ref*){ bgmNext(); });
	nextBtn->setPosition(Vec2(btnXs[2], CTL_Y));

	auto ctlMenu = Menu::create(prevBtn, playBtn, nextBtn, nullptr);
	ctlMenu->setPosition(Vec2::ZERO);
	root->addChild(ctlMenu, 3);

	// 6. Buy / Restore 키캡 아이콘 버튼 (LITE_VER, 컨트롤러 아래)
#ifdef LITE_VER
	{
		const Color4F iapCol(80/255.f, 220/255.f, 180/255.f, 1.f);
		bool hasPurchased = UserDataManager::Instance()->GetCart();
		if (hasPurchased) {
			auto badgeLbl = Label::createWithSystemFont("\xe2\x9c\x93 UNLOCKED", "Arial", 10);
			badgeLbl->setColor(Color3B(80, 220, 180));
			badgeLbl->setAnchorPoint(Vec2(0.5f, 0.5f));
			badgeLbl->setPosition(Vec2(CX, IAP_Y));
			root->addChild(badgeLbl, 3);
		} else {
			// 공유 키캡 배경
			auto caps = DrawNode::create();
			drawKeycap(caps, IBTN_LX, IAP_Y, IBTN_W, IBTN_H);
			drawKeycap(caps, IBTN_RX, IAP_Y, IBTN_W, IBTN_H);
			root->addChild(caps, 2);

			// Buy 버튼: 자물쇠 아이콘 (drawVecLock)
			Node* buyIcon = makeVecIcon(IBTN_W, IBTN_H, [iapCol](DrawNode* dn, float cx, float cy){
				drawVecLock(dn, cx, cy, 14.f, iapCol);
			});
			auto buyBtn = MenuItemLabel::create(buyIcon,
				CC_CALLBACK_1(MainScene::callbackOnPushed_buyMenuItem, this));
			buyBtn->setPosition(Vec2(IBTN_LX, IAP_Y));

			// Restore 버튼: 원형 화살표 (drawVecRestore_CircArrow)
			Node* restoreIcon = makeVecIcon(IBTN_W, IBTN_H, [iapCol](DrawNode* dn, float cx, float cy){
				drawVecRestore_CircArrow(dn, cx, cy, 12.f, iapCol);
			});
			auto restoreBtn = MenuItemLabel::create(restoreIcon, [this](Ref* s){
				this->callbackLockBtn(s);
			});
			restoreBtn->setPosition(Vec2(IBTN_RX, IAP_Y));

			auto iapMenu = Menu::create(buyBtn, restoreBtn, nullptr);
			iapMenu->setPosition(Vec2::ZERO);
			root->addChild(iapMenu, 3);
		}
		CMKStoreManager::Instance()->SetDelegate(this);
	}
#endif  // LITE_VER
}

void MainScene::bgmPlay(int index)
{
	m_bgmIndex   = ((index % BGM_TRACK_COUNT) + BGM_TRACK_COUNT) % BGM_TRACK_COUNT;
	m_bgmPlaying = true;
	UserDefault::getInstance()->setIntegerForKey(KEY_BGM_INDEX, m_bgmIndex);
	if (UserDataManager::Instance()->GetSoundOpt())
		SoundFactory::Instance()->crossfadeToTrack(s_bgmTracks[m_bgmIndex].file, 0.3f);
	bgmUpdatePlayBtn();
	bgmStartSpeakerAnim();
	startBgmTicker(s_bgmTracks[m_bgmIndex].title);
}

void MainScene::bgmPlaySelection()
{
	// 현재 선택(m_bgmSelection)에 따라 트랙 재생
	int trackIdx;
	if (m_bgmSelection == 0) {
		trackIdx = rand() % BGM_TRACK_COUNT;
	} else {
		trackIdx = m_bgmSelection - 1;
	}
	bgmPlay(trackIdx);
}

void MainScene::bgmTogglePlayPause()
{
	SoundFactory::Instance()->play("efs_click");
	if (m_bgmPlaying) {
		m_bgmPlaying = false;
		SoundFactory::Instance()->fadeOutBGM(0.5f);
		bgmStopSpeakerAnim();
		stopBgmTicker();
		bgmUpdatePlayBtn();
	} else {
		// fade-out 진행 중일 경우 취소 후 새 트랙 시작
		Director::getInstance()->getScheduler()->unschedule(
			"bgm_fadeout", (void*)SoundFactory::Instance());
		bgmPlaySelection();
	}
}

void MainScene::bgmNext()
{
	// 선택을 다음으로 이동 (Random → Space → Universe → Cosmos → Nova → Moon → Earth → Random)
	m_bgmSelection = (m_bgmSelection + 1) % BGM_SEL_COUNT;
	UserDefault::getInstance()->setIntegerForKey(KEY_BGM_SEL, m_bgmSelection);
	bgmUpdatePlaylistLed();
	if (m_bgmPlaying) bgmPlaySelection();
}

void MainScene::bgmPrev()
{
	m_bgmSelection = (m_bgmSelection - 1 + BGM_SEL_COUNT) % BGM_SEL_COUNT;
	UserDefault::getInstance()->setIntegerForKey(KEY_BGM_SEL, m_bgmSelection);
	bgmUpdatePlaylistLed();
	if (m_bgmPlaying) bgmPlaySelection();
}

void MainScene::bgmUpdatePlayBtn()
{
	if (!m_playBtnIcon) return;
	const float BW=22, BH=14;
	const Color4F col(80/255.f,220/255.f,180/255.f,1.f);
	m_playBtnIcon->clear();
	drawKeycap(m_playBtnIcon, BW/2, BH/2, BW, BH);
	float ic = BH * 0.62f;
	if (m_bgmPlaying) drawIconPause(m_playBtnIcon, BW/2, BH/2, ic, col);
	else              drawIconPlay (m_playBtnIcon, BW/2, BH/2, ic, col);
}

void MainScene::bgmUpdatePlaylistLed()
{
	if (!m_bgmTitleLabel) return;
	int sel = (m_bgmSelection >= 0 && m_bgmSelection < BGM_SEL_COUNT) ? m_bgmSelection : 0;
	m_bgmTitleLabel->setString(s_selNames[sel]);
	if (sel == 0) {
		// Random: 회색
		m_bgmTitleLabel->setColor(Color3B(160, 160, 160));
		m_bgmTitleLabel->setOpacity(180);
	} else if (m_bgmPlaying && m_bgmIndex == sel - 1) {
		// 현재 재생 중: 민트 밝게
		m_bgmTitleLabel->setColor(Color3B(80, 220, 180));
		m_bgmTitleLabel->setOpacity(230);
	} else {
		// 선택됐지만 미재생: 민트 어둡게
		m_bgmTitleLabel->setColor(Color3B(60, 160, 130));
		m_bgmTitleLabel->setOpacity(190);
	}
}

void MainScene::bgmStartSpeakerAnim()
{
	auto pulse = [](Node* n) {
		if (!n) return;
		n->stopAllActions();
		n->runAction(RepeatForever::create(Sequence::create(
			ScaleTo::create(0.09f, 1.00f, 1.04f),
			ScaleTo::create(0.09f, 1.00f, 1.00f),
			ScaleTo::create(0.11f, 1.00f, 1.04f),
			ScaleTo::create(0.08f, 1.00f, 1.00f),
			DelayTime::create(0.20f),
			nullptr)));
	};
	pulse(m_speakerLNode);
	pulse(m_speakerRNode);
}

void MainScene::bgmStopSpeakerAnim()
{
	auto stop = [](Node* n) {
		if (!n) return;
		n->stopAllActions();
		n->setScaleX(1.f); n->setScaleY(1.f);
	};
	stop(m_speakerLNode);
	stop(m_speakerRNode);
}

void MainScene::startBgmTicker(const std::string& /*title*/)
{
	// 플레이리스트 LED 패널을 업데이트 (TopInfoBar 수정 없음)
	bgmUpdatePlaylistLed();
}

void MainScene::stopBgmTicker()
{
	bgmUpdatePlaylistLed();
}
