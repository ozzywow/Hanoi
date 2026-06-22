#include "stdafx.h"
#include <algorithm>
#include "MainScene.h"
#include "SoundFactory.h"
#include "PlayScene.h"
#include "UserDataManager.h"
#include "LeaderboardManager.h"
#include "ui/CocosGUI.h"
#ifdef LITE_VER
#include "MKStoreManager_cpp.h"
#endif //LITE_VER

// 네온 민트 (PlayScene 도크 공통색) — 키캡/아이콘 틴트에 사용
static const Color3B MINT_C(80, 220, 180);

// PlayScene 도크와 동일한 베벨 키캡 (중심 cx,cy / 크기 w,h)
// grayStyle=true → 회색 톤 (기어 등 보조 버튼용)
static void drawKeycap(DrawNode* node, float cx, float cy, float w, float h, bool grayStyle = false)
{
	float x0 = cx - w / 2, x1 = cx + w / 2;
	float y0 = cy - h / 2, y1 = cy + h / 2;
	float c = (w < h ? w : h) * 0.10f; if (c < 2.0f) c = 2.0f;  // 챔퍼
	Color4F base   = grayStyle ? Color4F(0.13f, 0.14f, 0.16f, 1.0f) : Color4F(0.08f, 0.19f, 0.19f, 1.0f);
	Color4F hi     = grayStyle ? Color4F(0.65f, 0.65f, 0.65f, 0.4f) : Color4F(0.5f,  1.0f,  0.85f, 0.5f);
	Color4F shadow = Color4F(0.0f, 0.0f, 0.0f, 0.5f);
	Color4F border = grayStyle ? Color4F(0.42f, 0.46f, 0.50f, 0.60f) : Color4F(0.31f, 0.86f, 0.70f, 0.55f);
	// 12점 폴리곤 (코너당 3점 — 45° 호 중간점 추가로 자연스러운 라운딩)
	const float k = 0.293f;   // 1 - sin45° : 호 중간점 내측 오프셋 비율
	Vec2 poly[12] = {
		Vec2(x0+c,       y0),              // bottom-left
		Vec2(x1-c,       y0),              // bottom-right
		Vec2(x1-c*k,     y0+c*k),         // BR arc mid
		Vec2(x1,         y0+c),            // right-bottom
		Vec2(x1,         y1-c),            // right-top
		Vec2(x1-c*k,     y1-c*k),         // TR arc mid
		Vec2(x1-c,       y1),              // top-right
		Vec2(x0+c,       y1),              // top-left
		Vec2(x0+c*k,     y1-c*k),         // TL arc mid
		Vec2(x0,         y1-c),            // left-top
		Vec2(x0,         y0+c),            // left-bottom
		Vec2(x0+c*k,     y0+c*k),         // BL arc mid
	};
	node->drawSolidPoly(poly, 12, base);
	node->drawSolidRect(Vec2(x0+c, y0),   Vec2(x1-c, y0+2), shadow);  // 하단 그림자
	node->drawSolidRect(Vec2(x1-2, y0+c), Vec2(x1,   y1-c), shadow);  // 우측 그림자
	node->drawSolidRect(Vec2(x0+c, y1-2), Vec2(x1-c, y1),   hi);      // 상단 하이라이트
	node->drawSolidRect(Vec2(x0,   y0+c), Vec2(x0+2, y1-c), hi);      // 좌측 하이라이트
	// 코너 bevel 삼각형: 스트립이 끊기는 호(arc) 영역 보완
	Vec2 tl[3] = { Vec2(x0,    y1-c), Vec2(x0+c, y1),    Vec2(x0+2, y1-2) };  // TL 하이라이트
	Vec2 br[3] = { Vec2(x1-c,  y0),   Vec2(x1,   y0+c),  Vec2(x1-2, y0+2) };  // BR 그림자
	node->drawSolidPoly(tl, 3, hi);
	node->drawSolidPoly(br, 3, shadow);
	node->drawPoly(poly, 12, true, border);                             // 12점 라운드 외곽선
}

// 5x7 픽셀 폰트 (셀아트 글자) — 7행 x 5열, '#'=픽셀 on. START에 필요한 글자만 정의.
static const char* pixelGlyph(char c)
{
	switch (c) {
	case 'S': return ".####" "#...." "#...." ".###." "....#" "....#" "####.";
	case 'T': return "#####" "..#.." "..#.." "..#.." "..#.." "..#.." "..#..";
	case 'A': return ".###." "#...#" "#...#" "#####" "#...#" "#...#" "#...#";
	case 'R': return "####." "#...#" "#...#" "####." "#.#.." "#..#." "#...#";
	default:  return nullptr;
	}
}

static float pixelTextWidth(const std::string& s, float px)
{
	const float advance = 5 * px + px;          // 글자폭(5) + 간격(1)
	return s.empty() ? 0 : (advance * s.size() - px);
}

// 픽셀 글자들을 주어진 DrawNode에 한 색으로 그림 (recolor 시 clear 후 재호출).
static void drawPixelGlyphs(DrawNode* node, const std::string& s, float px,
                            const Color4F& col, const Vec2& off)
{
	const int GW = 5, GH = 7;
	const float advance = GW * px + px;
	const float cell = px - 0.6f;               // 셀 사이 미세 간격 → LED 도트 느낌
	float x0 = 0;
	for (char ch : s) {
		const char* g = pixelGlyph(ch);
		if (g) {
			for (int row = 0; row < GH; ++row)
				for (int c2 = 0; c2 < GW; ++c2) {
					if (g[row * GW + c2] != '#') continue;
					float bx = x0 + c2 * px;
					float by = (GH - 1 - row) * px;   // 행0이 위 → y 뒤집기
					node->drawSolidRect(Vec2(bx, by) + off, Vec2(bx + cell, by + cell) + off, col);
				}
		}
		x0 += advance;
	}
}

// HSV(h 0..1 순환, s,v 0..1) → RGB. 그라데이션/스파클 연출용.
static Color4F hsv2rgb(float h, float s, float v)
{
	h = h - floorf(h);
	float i = floorf(h * 6.0f);
	float f = h * 6.0f - i;
	float p = v * (1 - s), q = v * (1 - s * f), t = v * (1 - s * (1 - f));
	float r = v, g = v, b = v;
	switch (((int)i) % 6) {
	case 0: r = v; g = t; b = p; break;
	case 1: r = q; g = v; b = p; break;
	case 2: r = p; g = v; b = t; break;
	case 3: r = p; g = q; b = v; break;
	case 4: r = t; g = p; b = v; break;
	case 5: r = v; g = p; b = q; break;
	}
	return Color4F(r, g, b, 1.0f);
}

// 픽셀별 색 콜백 버전. fn(gx, gy, normX) → Color4F. gx,gy=전체 픽셀 격자 인덱스, normX=가로 진행도 0..1.
static void drawPixelGlyphsFn(DrawNode* node, const std::string& s, float px, const Vec2& off,
                              const std::function<Color4F(int gx, int gy, float normX)>& fn)
{
	const int GW = 5, GH = 7;
	const float advance = GW * px + px;
	const float cell = px - 0.6f;
	const int nLetters = (int)s.size();
	const float totalCols = nLetters > 0 ? (nLetters * (GW + 1) - 1) : 1;
	float x0 = 0;
	int letterIdx = 0;
	for (char ch : s) {
		const char* g = pixelGlyph(ch);
		if (g) {
			for (int row = 0; row < GH; ++row)
				for (int c2 = 0; c2 < GW; ++c2) {
					if (g[row * GW + c2] != '#') continue;
					float bx = x0 + c2 * px;
					float by = (GH - 1 - row) * px;
					int gx = letterIdx * (GW + 1) + c2;
					int gy = GH - 1 - row;
					float normX = gx / totalCols;
					node->drawSolidRect(Vec2(bx, by) + off, Vec2(bx + cell, by + cell) + off, fn(gx, gy, normX));
				}
		}
		x0 += advance;
		letterIdx++;
	}
}

// 셀아트 픽셀 텍스트 노드 (LED 도트 톤). 검은 드롭섀도(고정) + 본체(outMain로 반환 → 색상 순환).
// contentSize 지정 → MenuItemLabel 터치영역 확보.
static Node* makePixelText(const std::string& s, float px, Color3B col, DrawNode** outMain = nullptr)
{
	auto node = Node::create();
	node->setContentSize(Size(pixelTextWidth(s, px), 7 * px));
	node->setCascadeOpacityEnabled(true);

	auto shadow = DrawNode::create();
	auto main   = DrawNode::create();
	node->addChild(shadow, 0);
	node->addChild(main, 1);

	drawPixelGlyphs(shadow, s, px, Color4F(0, 0, 0, 0.6f), Vec2(1.5f, -1.5f));
	drawPixelGlyphs(main, s, px, Color4F(col.r / 255.f, col.g / 255.f, col.b / 255.f, 1.0f), Vec2::ZERO);

	if (outMain) *outMain = main;
	return node;
}

// 벡터 아이콘 헬퍼 — DrawNode에 민트 기하 도형으로 아이콘을 그림.
// cx,cy = 아이콘 중심 (node 로컬 좌표). 각 함수가 node에 직접 그림.

// ⚙ 기어: 외곽 원 + 8개 돌기 + 중앙 구멍
static void drawVecGear(DrawNode* node, float cx, float cy, float r, const Color4F& col)
{
	// 외곽 원
	node->drawCircle(Vec2(cx, cy), r, 0, 40, false, col);
	// 8방향 직사각형 돌기
	const int TEETH = 8;
	const float toothW = r * 0.38f, toothH = r * 0.42f;
	for (int i = 0; i < TEETH; ++i) {
		float angle = (float)i / TEETH * M_PI * 2.f;
		float ca = cosf(angle), sa = sinf(angle);
		// 돌기 중심 (원 외곽에서 toothH/2 만큼 더 나감)
		float tx = cx + (r + toothH * 0.35f) * ca;
		float ty = cy + (r + toothH * 0.35f) * sa;
		// 4개 꼭짓점 (로컬 ±w/2, ±h/2 → 회전)
		float hw = toothW / 2.f, hh = toothH / 2.f;
		Vec2 pts[4] = {
			Vec2(tx + (-hw * ca - (-hh) * sa), ty + (-hw * sa + (-hh) * ca)),
			Vec2(tx + ( hw * ca - (-hh) * sa), ty + ( hw * sa + (-hh) * ca)),
			Vec2(tx + ( hw * ca -   hh  * sa), ty + ( hw * sa +   hh  * ca)),
			Vec2(tx + (-hw * ca -   hh  * sa), ty + (-hw * sa +   hh  * ca)),
		};
		node->drawSolidPoly(pts, 4, col);
	}
	// 채워진 외곽 원 (돌기 안쪽 채움)
	node->drawSolidCircle(Vec2(cx, cy), r, 0, 32, col);
	// 중앙 구멍 (배경색으로 덮어 뚫린 것처럼)
	node->drawSolidCircle(Vec2(cx, cy), r * 0.38f, 0, 20,
		Color4F(0.08f, 0.19f, 0.19f, 1.0f));
}

// drawVecCartIcon → common_define.h 로 이동 (PlayScene 공용)

// ↻ 원형 화살표 Restore (시안 A)
// 15°에서 시계방향 285° 호, 끝(90°/12시)에서 오른쪽 가리키는 화살촉
static void drawVecRestore_CircArrow(DrawNode* node, float cx, float cy, float sz, const Color4F& col)
{
	float archR  = sz * 0.40f;
	float archIR = archR * 0.72f;
	const int N  = 20;

	float startA = M_PI * 15.f / 180.f;
	float sweepA = M_PI * 285.f / 180.f;

	for (int i = 0; i < N; ++i) {
		float a0 = startA - sweepA *  i      / N;
		float a1 = startA - sweepA * (i + 1) / N;
		Vec2 pts[4] = {
			{cx + cosf(a0)*archIR, cy + sinf(a0)*archIR},
			{cx + cosf(a0)*archR,  cy + sinf(a0)*archR },
			{cx + cosf(a1)*archR,  cy + sinf(a1)*archR },
			{cx + cosf(a1)*archIR, cy + sinf(a1)*archIR},
		};
		node->drawSolidPoly(pts, 4, col);
	}

	// 화살촉: 90°(12시) 위치, 시계방향 접선 → 오른쪽
	float midR = (archR + archIR) * 0.5f;
	float asz  = sz * 0.21f;
	Vec2  tip  = {cx + 0.f, cy + midR};    // cos(90°)=0, sin(90°)=1
	Vec2  fwd  = {1.f, 0.f};               // sin(90°)=1, -cos(90°)=0
	Vec2  perp = {0.f, 1.f};               // cos(90°)=0,  sin(90°)=1

	Vec2 arrowPts[3] = {
		{tip.x + fwd.x*asz,                          tip.y + fwd.y*asz},
		{tip.x - fwd.x*asz*0.3f + perp.x*asz*0.75f, tip.y - fwd.y*asz*0.3f + perp.y*asz*0.75f},
		{tip.x - fwd.x*asz*0.3f - perp.x*asz*0.75f, tip.y - fwd.y*asz*0.3f - perp.y*asz*0.75f},
	};
	node->drawSolidPoly(arrowPts, 3, col);
}

// 벡터 아이콘 Node 생성 — w×h contentSize로 터치영역 확보, 아이콘은 중앙에 그림
static Node* makeVecIcon(float w, float h,
                         const std::function<void(DrawNode*, float cx, float cy)>& drawFn)
{
	auto node = Node::create();
	node->setContentSize(Size(w, h));
	node->setCascadeOpacityEnabled(true);
	auto dn = DrawNode::create();
	drawFn(dn, w / 2.f, h / 2.f);
	node->addChild(dn);
	return node;
}

// 채워진 삼각형 화살표 (solid triangle)
static void drawVecTriangle(DrawNode* node, float cx, float cy, float sz, bool leftward, const Color4F& col)
{
	float hw = sz * 0.45f, hh = sz * 0.38f;
	Vec2 pts[3];
	if (leftward) {
		pts[0] = Vec2(cx + hw, cy + hh);
		pts[1] = Vec2(cx - hw, cy);
		pts[2] = Vec2(cx + hw, cy - hh);
	} else {
		pts[0] = Vec2(cx - hw, cy + hh);
		pts[1] = Vec2(cx + hw, cy);
		pts[2] = Vec2(cx - hw, cy - hh);
	}
	node->drawSolidPoly(pts, 3, col);
}

// 이중 삼각형 키캡 (랭킹보드 ◀◀ ▶▶ 버튼)
static Node* makeVecNavKeycap(float w, float h, bool leftward, bool enabled)
{
	auto node = Node::create();
	node->setContentSize(Size(w, h));
	auto dn = DrawNode::create();
	drawKeycap(dn, w / 2.f, h / 2.f, w, h);
	Color4F col = enabled
		? Color4F(80 / 255.f, 220 / 255.f, 180 / 255.f, 1.f)
		: Color4F(0.28f, 0.28f, 0.28f, 0.55f);
	drawVecTriangle(dn, w / 2.f, h / 2.f, h * 0.52f, leftward, col);
	node->addChild(dn);
	return node;
}

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
	const Vec2  START_POS(75, 150);
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
		m_topTickerLabel->setColor(MINT_C);
		m_topTickerLabel->setAnchorPoint(Vec2(0, 0.5f));
		m_topTickerLabel->setPosition(Vec2(RESOURCE_WIDTH + 5, TPOS.y));
		m_topTickerLabel->setVisible(false);
		this->addChild(m_topTickerLabel, 11);
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
		m_botTickerLabel = Label::createWithSystemFont("", "Arial", 9);
		m_botTickerLabel->setColor(MINT_C);
		m_botTickerLabel->setAnchorPoint(Vec2(0, 0.5f));
		m_botTickerLabel->setPosition(Vec2(-5, BPOS.y));
		m_botTickerLabel->setVisible(false);
		this->addChild(m_botTickerLabel, 11);
		auto bDots = DrawNode::create();
		setupLedDotOverlay(bDots, BW, BH);
		bDots->setPosition(BPOS);
		this->addChild(bDots, 12);   // dots는 라벨 위에 (LED 효과)
	}

#ifdef LITE_VER
	if (false == UserDataManager::Instance()->GetCart())
	{
		// ── STORE 스트립: 하단 바닥 정렬 (결제 완료 시 통째로 제거 → 하단만 트림) ──
		cocos2d::Node* buyRestoreNode = cocos2d::Node::create();
		buyRestoreNode->setPosition(cocos2d::Vec2(0, 0));
		this->addChild(buyRestoreNode, tagCart, tagCart);

		const float BUY_X = 50, RESTORE_X = 100, STRIP_Y = 80;
		const float ICON_W = 44, ICON_H = 44;

		auto caps = DrawNode::create();
		drawKeycap(caps, BUY_X,     STRIP_Y, ICON_W, ICON_H);
		drawKeycap(caps, RESTORE_X, STRIP_Y, ICON_W, ICON_H);
		buyRestoreNode->addChild(caps);

		// Buy 버튼: 자물쇠
		Node* lockIcon = makeVecIcon(ICON_W, ICON_H, [](DrawNode* dn, float cx, float cy) {
			drawVecLock(dn, cx, cy, 28.f, Color4F(80/255.f, 220/255.f, 180/255.f, 1.f));
		});
		auto buyMenuItem = MenuItemLabel::create(lockIcon, CC_CALLBACK_1(MainScene::callbackOnPushed_buyMenuItem, this));
		buyMenuItem->setPosition(Vec2(BUY_X, STRIP_Y));

		// Restore 버튼: 원형 화살표
		Node* restoreIcon = makeVecIcon(ICON_W, ICON_H, [](DrawNode* dn, float cx, float cy) {
			drawVecRestore_CircArrow(dn, cx, cy, 25.f, Color4F(80/255.f, 220/255.f, 180/255.f, 1.f));
		});
		auto restoreMenuItem = MenuItemLabel::create(restoreIcon, CC_CALLBACK_1(MainScene::callbackLockBtn, this));
		restoreMenuItem->setPosition(Vec2(RESTORE_X, STRIP_Y));

		Menu* subMenu = Menu::create(buyMenuItem, restoreMenuItem, NULL);
		subMenu->setPosition(Vec2(0, 0));
		buyRestoreNode->addChild(subMenu);

	}
	else
	{
		// 이미 구매 완료 — 빈 자리에 골드 배지
		showPremiumBadge();
	}
	CMKStoreManager::Instance()->SetDelegate(this);
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

	// 방금 이름 등록한 경우 PlayFab 전파 지연 보상을 위해 2초 후 재갱신
	if (UserDataManager::Instance()->m_justRegistered) {
		UserDataManager::Instance()->m_justRegistered = false;
		int lv = level;
		scheduleOnce([this, lv](float) { drawOnlineRank(lv); }, 2.0f, "refreshRank");
	}

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


void MainScene::callbackOnPushed_resetMenuItem(Ref* pSender)
{
	const int CONFIRM_TAG = 198;
	if (this->getChildByTag(CONFIRM_TAG)) return;

	const float DW = 220, DH = 110;

	auto backdrop = LayerColor::create(Color4B(0, 0, 0, 0));
	backdrop->setTag(CONFIRM_TAG);
	this->addChild(backdrop, 999);
	backdrop->runAction(FadeTo::create(0.2f, 160));

	auto dlg = LayerColor::create(Color4B(10, 15, 50, 230), DW, DH);
	dlg->setPosition(Vec2((RESOURCE_WIDTH - DW) / 2, (RESOURCE_HEIGHT - DH) / 2));
	dlg->setScale(0.7f);
	backdrop->addChild(dlg);
	dlg->runAction(Sequence::create(
		ScaleTo::create(0.15f, 1.05f),
		ScaleTo::create(0.08f, 1.0f),
		nullptr
	));

	auto titleLabel = Label::createWithSystemFont("RESET ALL?", "Arial", 15);
	titleLabel->setColor(Color3B(255, 100, 80));
	titleLabel->setPosition(Vec2(DW / 2, DH - 22));
	dlg->addChild(titleLabel);

	auto divider = DrawNode::create();
	divider->drawLine(Vec2(15, DH - 40), Vec2(DW - 15, DH - 40), Color4F(0.5f, 0.5f, 0.5f, 0.8f));
	dlg->addChild(divider);

	auto msg = Label::createWithSystemFont("Reset all records?", "Arial", 12);
	msg->setColor(Color3B(200, 200, 200));
	msg->setPosition(Vec2(DW / 2, DH - 65));
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
	auto okBtn = MenuItemLabel::create(okLabel, [this, doReset](Ref*) {
		this->removeChildByTag(198);
		doReset();
	});
	okBtn->setPosition(Vec2(DW * 0.3f, 22));

	auto cancelLabel = Label::createWithSystemFont("Cancel", "Arial", 14);
	cancelLabel->setColor(Color3B(180, 180, 180));
	auto cancelBtn = MenuItemLabel::create(cancelLabel, [this](Ref*) {
		this->removeChildByTag(198);
	});
	cancelBtn->setPosition(Vec2(DW * 0.7f, 22));

	auto menu = Menu::create(okBtn, cancelBtn, nullptr);
	menu->setPosition(Vec2::ZERO);
	dlg->addChild(menu);
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
		this->removeChildByTag(tagCart);
		showPremiumBadge();

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
	this->removeChildByTag(tagCart);
	showPremiumBadge();

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
				e.displayName.substr(0, 10), "Arial", rowFont);
			nm->setAnchorPoint(Vec2(0, 0.5f)); nm->setPosition(Vec2(55, y));
			nm->setColor(rowCol); addLbl(nm);

			auto tm = Label::createWithSystemFont(
				StringUtils::format("%02d:%02d.%02d", rt.min, rt.sec, rt.ms), "Arial", rowFont);
			tm->setAnchorPoint(Vec2(1.0f, 0.5f)); tm->setPosition(Vec2(PW - 10, y));
			tm->setColor(rowCol); addLbl(tm);
		}

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

void MainScene::showNameInputDialog()
{
	SoundFactory::Instance()->play("efs_click");
	const int DIALOG_TAG = 199;
	const float DW = 250, DH = 110;

	auto backdrop = LayerColor::create(Color4B(0, 0, 0, 0));
	backdrop->setTag(DIALOG_TAG);
	this->addChild(backdrop, 999);
	backdrop->runAction(FadeTo::create(0.2f, 160));

	auto dlg = LayerColor::create(Color4B(10, 15, 50, 230), DW, DH);
	dlg->setPosition(Vec2((RESOURCE_WIDTH - DW) / 2, (RESOURCE_HEIGHT - DH) / 2));
	dlg->setScale(0.7f);
	backdrop->addChild(dlg);
	dlg->runAction(Sequence::create(
		ScaleTo::create(0.15f, 1.05f),
		ScaleTo::create(0.08f, 1.0f),
		nullptr
	));

	auto titleLabel = Label::createWithSystemFont("SET YOUR NAME", "Arial", 14);
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

	auto okLabel = Label::createWithSystemFont("  OK  ", "Arial", 14);
	okLabel->setColor(Color3B(100, 100, 100));
	auto okBtn = MenuItemLabel::create(okLabel, [editBox](Ref*) {
		std::string name = editBox->getText();
		UserDataManager::Instance()->SetUserName(name);
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

void MainScene::showSettingsMenu()
{
	const int TAG = 196;
	if (this->getChildByTag(TAG)) return;
	SoundFactory::Instance()->play("efs_click");

	const float DW = 200, DH = 130;

	auto backdrop = LayerColor::create(Color4B(0, 0, 0, 0));
	backdrop->setTag(TAG);
	this->addChild(backdrop, 999);
	backdrop->runAction(FadeTo::create(0.15f, 150));

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

	auto titleLabel = Label::createWithSystemFont("SETTINGS", "Arial", 15);
	titleLabel->setColor(Color3B(255, 215, 0));
	titleLabel->setPosition(Vec2(DW / 2, DH - 22));
	dlg->addChild(titleLabel);

	auto divider = DrawNode::create();
	divider->drawLine(Vec2(15, DH - 40), Vec2(DW - 15, DH - 40), Color4F(0.5f, 0.5f, 0.5f, 0.8f));
	dlg->addChild(divider);

	// RESET 항목 (확인 다이얼로그로 연결)
	auto resetLbl = Label::createWithSystemFont("RESET  ALL  RECORDS", "Arial", 13);
	resetLbl->setColor(Color3B(255, 100, 80));
	auto resetBtn = MenuItemLabel::create(resetLbl, [this, TAG](Ref* s) {
		SoundFactory::Instance()->play("efs_click");
		this->removeChildByTag(TAG);
		this->callbackOnPushed_resetMenuItem(s);
	});
	resetBtn->setPosition(Vec2(DW / 2, DH - 62));

	auto closeLbl = Label::createWithSystemFont("CLOSE", "Arial", 13);
	closeLbl->setColor(Color3B(180, 180, 180));
	auto closeBtn = MenuItemLabel::create(closeLbl, [this, TAG](Ref*) {
		SoundFactory::Instance()->play("efs_click");
		this->removeChildByTag(TAG);
	});
	closeBtn->setPosition(Vec2(DW / 2, 22));

	auto menu = Menu::create(resetBtn, closeBtn, nullptr);
	menu->setPosition(Vec2::ZERO);
	dlg->addChild(menu);
}

void MainScene::showPremiumBadge()
{
	const int TAG_PREMIUM = 77;
	if (this->getChildByTag(TAG_PREMIUM)) return;

	auto node = Node::create();
	node->setPosition(Vec2::ZERO);
	this->addChild(node, tagInfoText, TAG_PREMIUM);

	// STORE 스트립이 있던 자리 (BUY~RESTORE 영역 중앙 x=75, y=80)
	const Vec2  BADGE_POS(75, 80);
	const float BW = 104, BH = 26;

	auto bg = DrawNode::create();
	bg->drawSolidRect(Vec2(-BW / 2, -BH / 2), Vec2(BW / 2, BH / 2), Color4F(0.10f, 0.08f, 0.02f, 1.0f)); // 어두운 LED 베이스
	bg->drawRect(Vec2(-BW / 2 - 2, -BH / 2 - 2), Vec2(BW / 2 + 2, BH / 2 + 2), Color4F(1.0f, 0.84f, 0.0f, 0.25f)); // 골드 글로우
	bg->drawRect(Vec2(-BW / 2, -BH / 2), Vec2(BW / 2, BH / 2), Color4F(1.0f, 0.84f, 0.0f, 0.9f));        // 골드 외곽
	bg->setPosition(BADGE_POS);
	node->addChild(bg);
	bg->runAction(RepeatForever::create(Sequence::create(
		ScaleTo::create(1.2f, 1.04f), ScaleTo::create(1.2f, 1.0f), nullptr)));

	auto label = Label::createWithSystemFont("\xE2\x9C\xA6 UNLOCKED", "Arial", 12); // ✦ UNLOCKED
	label->setColor(Color3B(255, 215, 0));
	label->enableOutline(Color4B(0, 0, 0, 200), 1);
	label->setPosition(BADGE_POS);
	node->addChild(label);
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
	auto alive = m_aliveFlag;
	auto results = std::make_shared<std::map<int, std::string>>();
	auto pending = std::make_shared<int>(8);   // level 3~10

	for (int lv = 3; lv <= 10; ++lv) {
		LeaderboardManager::Instance()->fetchLeaderboard(lv, 1,
			[this, alive, results, pending, lv](const std::vector<LeaderboardEntry>& entries) {
				if (!alive || !*alive) return;
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
					if (m_topTickerLabel) {
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
		if (!text.empty()) text += "  ";
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
