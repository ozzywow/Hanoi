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

// ── TopInfoBar 티커: 레벨 3~10 각 1위 → 우→좌 스크롤 ──────────────────
void MainScene::startTopTicker()
{
	if (!m_topTickerLabel) return;

	// 세대 증가 — 이 호출 이후의 늦게 오는 랭킹 콜백은 폐기(공지가 덮이지 않게).
	const int gen = ++m_topTickerGen;

	// 상단 티커 우선순위: 1)공지 notice → 2)업데이트 안내(current<latest) → 3)레벨별 랭킹.
	// 1·2순위는 앰버 + 📢, 3순위(랭킹)는 흰색.
	const std::string notice = LeaderboardManager::Instance()->getNotice();
	if (!notice.empty()) {
		m_topTickerBaseText = "\xF0\x9F\x93\xA2 " + notice;   // "📢 " (U+1F4E2) UTF-8
		m_topTickerColor    = Color3B(255, 191, 0);          // 앰버
		ensureTopTickerRunning();   // 현재 스크롤 패스를 끝낸 뒤 교체(끊김 없음)
		return;
	}

	// 2순위: 공지가 없고 current_version < latest_version → 업데이트 권장 안내.
	// (서버 조회로 latest 확보된 경우에만. 콜드스타트 지연 도착분은 fetchTitleConfig 콜백이 재시작.)
	{
		std::string        cur    = Application::getInstance()->getVersion();
		const std::string& latest = LeaderboardManager::Instance()->getLatestVersion();
		if (!cur.empty() && !latest.empty()
			&& LeaderboardManager::compareVersion(cur, latest) < 0) {
			m_topTickerBaseText =
				"\xF0\x9F\x93\xA2 A new version is now available. We recommend updating.";
			m_topTickerColor = Color3B(255, 191, 0);   // 앰버
			ensureTopTickerRunning();
			return;
		}
	}

	// 3순위: 레벨별 랭킹. 도착 전엔 기존 스크롤(공지/이전 랭킹)을 유지하다가 패스 경계에서 교체.
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
						m_topTickerColor    = Color3B::WHITE;   // 랭킹 스크롤은 흰색
						ensureTopTickerRunning();
					}
				}
			});
	}
}

// 루프가 아직 안 돌고 있을 때만 시작. 이미 돌고 있으면 문자열/색만 갱신되고
// 다음 패스 경계(tickTopStep 진입)에서 자연스럽게 교체된다(재시작으로 인한 끊김 방지).
void MainScene::ensureTopTickerRunning()
{
	if (m_topTickerRunning) return;
	if (!m_topTickerLabel) return;
	m_topTickerRunning = true;
	m_topTickerLabel->setVisible(true);
	tickTopStep();
}

void MainScene::tickTopStep()
{
	if (!m_topTickerLabel || !m_topTickerLabel->getParent()) return;
	m_topTickerLabel->setString(m_topTickerBaseText);   // 매 패스 최신 문자열/색 반영
	m_topTickerLabel->setColor(m_topTickerColor);
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

// ── BottomInfoBar 티커: 최근 접속 플레이어(최근이 우측) → 좌→우 스크롤 ──────────
//  슬라이딩이 좌→우라 문자열 우측이 먼저 등장 → 가장 최근 접속자를 우측 끝에 배치.
//  형식: "... Player2🇺🇸      Player1🇰🇷" (이름 뒤 국기). 목록 없으면 랜덤 국기 폴백.

// 폴백: 각국 국기 랜덤 셔플 나열 (서버 목록이 비었을 때만 사용)
std::string MainScene::buildRandomFlagTicker() const
{
	static const char* COUNTRY_CODES[] = {
		"kr","us","jp","gb","de","fr","it","es","cn","br",
		"ru","au","ca","mx","in","nl","se","no","fi","dk",
		"pl","pt","tr","ar","za","ng","id","ph","sg","th",
		"vn","hk","tw","sa","ae","il","gr","pk","my","ua"
	};
	const int N = (int)(sizeof(COUNTRY_CODES) / sizeof(COUNTRY_CODES[0]));

	std::vector<int> idx(N);
	for (int i = 0; i < N; ++i) idx[i] = i;
	for (int i = N - 1; i > 0; --i) {
		int j = rand() % (i + 1);
		std::swap(idx[i], idx[j]);
	}

	std::string text;
	for (int i = 0; i < N; ++i) {
		std::string cc = COUNTRY_CODES[idx[i]];
		if (!text.empty()) text += "      ";
		text += countryToFlag(cc);
	}
	return text;
}

// 최근 접속 플레이어: list는 ts 내림차순(최근 우선). 최근 BOT_TICKER_SHOW명을 취해
// "🎮 HAVE A NICE PLAY 🍀   오래된 … 최신   ✈️ WELCOME 🌍" 순으로 잇는다. 좌→우 스크롤이라
// 우측 끝(HEAD 배너)이 먼저 등장 → 화면 순서 [WELCOME → 최신 → … → 오래된 → PLAY] 반복.
std::string MainScene::buildRecentPlayerTicker(const std::vector<RecentPlayerEntry>& list) const
{
	// 우측 끝(먼저 등장): ✈️(U+2708 U+FE0F) WELCOME 🌍(U+1F30D)
	static const char* HEAD_BANNER = "\xE2\x9C\x88\xEF\xB8\x8F WELCOME \xF0\x9F\x8C\x8D";
	// WELCOME 바로 뒤 + 접속자 목록 앞: 🔴(U+1F534) PLAYING NOW — 라이브 진행감.
	static const char* LIVE_LABEL  = "\xF0\x9F\x94\xB4 PLAYING NOW";
	// 좌측 끝(맨 뒤): 🎮(U+1F3AE) HAVE A GOOD RUN 🔥(U+1F525) — 문구/이모지는 이 줄만 바꾸면 됨.
	static const char* TAIL_BANNER = "\xF0\x9F\x8E\xAE HAVE A GOOD RUN \xF0\x9F\x94\xA5";

	if (list.empty()) return "";   // 목록 없음 → 배너도 생략(호출측이 폴백 국기 유지)

	int n = (int)list.size();
	if (n > BOT_TICKER_SHOW) n = BOT_TICKER_SHOW;   // 최근 N명만
	std::string text = TAIL_BANNER;                 // 맨 왼쪽(=맨 뒤)에 PLAY 배너
	for (int i = n - 1; i >= 0; --i) {              // 오래된→최신 (최신이 우측)
		const auto& e = list[i];
		text += "      ";
		text += e.name;
		if (!e.countryCode.empty()) text += countryToFlag(e.countryCode);
	}
	text += "      ";
	text += LIVE_LABEL;                             // 접속자 목록 바로 뒤(화면상 WELCOME 다음)
	text += "      ";
	text += HEAD_BANNER;                            // 우측 끝(=먼저 등장)에 WELCOME 배너
	return text;
}

// 루프가 아직 안 돌고 있을 때만 시작. 이미 돌고 있으면 문자열만 갱신되고 다음 패스에서 교체.
void MainScene::ensureBotTickerRunning()
{
	if (m_botTickerRunning) return;
	if (!m_botTickerLabel) return;
	m_botTickerRunning = true;
	m_botTickerLabel->setVisible(true);
	tickBotStep();
}

void MainScene::startBotTicker()
{
	auto alive = m_aliveFlag;

	// 목록을 이미 띄웠는지 — 만국기 폴백/타임아웃과 공유.
	auto shown = std::make_shared<bool>(false);

	// 0) 디스크에 영속화된 지난 목록이 있으면 콜드 스타트에서도 즉시 표시(만국기 안 봄).
	//    이후 refresh가 서버 최신본으로 조용히 갱신(stale-while-revalidate).
	{
		auto persisted = LeaderboardManager::Instance()->peekPersistedRecent();
		std::string t0 = buildRecentPlayerTicker(persisted);
		if (!t0.empty()) {
			m_botTickerText = t0;
			*shown = true;
			ensureBotTickerRunning();
		}
	}

	// 최근 접속 플레이어 조회. 유효 캐시가 있으면 콜백이 즉시(다음 프레임) 목록을 반환하고,
	// 없으면(로그인 미완 등) 로그인·서버 조회 후 반환. 목록이 있으면 그때 스크롤 시작.
	auto refresh = [this, alive, shown]() {
		LeaderboardManager::Instance()->fetchRecentPlayers(
			[this, alive, shown](const std::vector<RecentPlayerEntry>& players) {
				if (!alive || !*alive) return;
				std::string t = buildRecentPlayerTicker(players);
				if (t.empty()) return;            // 빈 목록 → 아무 것도 안 함(타임아웃이 만국기 처리)
				m_botTickerText = t;
				*shown = true;
				ensureBotTickerRunning();
			});
	};

	// 이번 세션 1회: 이름이 있으면 접속 기록(touch) → 성공 후 2초 뒤 재조회(전파 지연 보상).
	static bool s_touched = false;
	std::string myName = UserDataManager::Instance()->GetUserName();
	if (!s_touched && !myName.empty()) {
		s_touched = true;
		LeaderboardManager::Instance()->touchRecentPlayer(myName,
			[this, alive, refresh](bool /*ok*/) {
				if (!alive || !*alive) return;
				this->scheduleOnce([alive, refresh](float) {
					if (!alive || !*alive) return;
					LeaderboardManager::Instance()->invalidateRecent();  // 전파 지연 보상
					refresh();
				}, 2.0f, "recentTouchRefresh");
			});
	}

	// 즉시 조회 시도(유효 캐시면 바로 목록 출력).
	refresh();

	// 3초 타임아웃: 그때까지 목록을 못 띄웠으면(로그인/조회 지연) 만국기 폴백으로 시작.
	this->scheduleOnce([this, alive, shown](float) {
		if (!alive || !*alive) return;
		if (*shown) return;                       // 이미 목록 표시됨 → 만국기 불필요
		m_botTickerText = buildRandomFlagTicker();
		ensureBotTickerRunning();
	}, 3.0f, "botTickerTimeout");
}

void MainScene::tickBotStep()
{
	if (!m_botTickerLabel || !m_botTickerLabel->getParent()) return;
	m_botTickerLabel->setString(m_botTickerText);   // 매 패스 최신 문자열 반영(심리스 갱신)
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
		IAPManager::Instance()->SetDelegate(this);
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
