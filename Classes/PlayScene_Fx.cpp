#include "stdafx.h"
#include "PlayScene.h"
#include "PlaySceneTouchHandlerLayer.h"
#include "Discus.h"
#include "SoundFactory.h"
#include "MainScene.h"
#include "UserDataManager.h"
#include "LeaderboardManager.h"
#include "ReviewManager.h"
#include "DrawUtils.h"
#ifdef LITE_VER
#include "IAPManager.h"
#endif

using namespace cocos2d;

#include "PlaySceneInternal.h"

static const int   TAG_GUIDE_ANIM      = 201;

// ── 대기 화면 랜덤 연출 ──────────────────────────────────────────
void PlayScene::_showNextIdleScene()
{
    static int last = -1;
    const int N = 7;
    int pick;
    do { pick = rand() % N; } while (pick == last);
    last = pick;

    std::string lv = StringUtils::format("LV.%d", m_countOfDiscus);

    switch (pick)
    {
    case 0:
        // 소형(10) — 긴 메시지, 타이핑/슬라이드/깜빡임
        typingBottomPanel (0, "🎯 " + lv,        0.08f, Color3B( 80, 220, 180), 10.0f);
        slideInBottomPanel(1, "⚡ CHALLENGE",     true,  Color3B(255, 210,  50), 10.0f);
        setBottomPanel    (2, "🏆 READY?",               Color3B(255, 120,  80), 10.0f);
        blinkBottomPanel  (2, 0.5f);
        break;
    case 1:
        // 중형(16) — 이모지+단어 조합
        slideInBottomPanel(0, "🔥 EXPERT",        false, Color3B(255,  80,  80), 16.0f);
        setBottomPanel    (1, "⭐ " + lv,                Color3B(180, 180, 255), 16.0f);
        colorCycleBottomPanel(1);
        typingBottomPanel (2, "👆 START",         0.1f,  Color3B( 80, 220, 180), 16.0f);   // 중앙 START 버튼 유도
        break;
    case 2:
        // 대형(24) — 짧은 단어/이모지, pulse·슬라이드
        setBottomPanel    (0, "🌟 OLYMPIC",              Color3B(255, 215,   0), 24.0f);
        pulseBottomPanel  (0);
        typingBottomPanel (1, "GO!",              0.12f, Color3B( 80, 220, 180), 24.0f);
        slideInBottomPanel(2, "💪",               true,  Color3B(255, 120,  80), 24.0f);
        break;
    case 3:
        // 중형(15) — TOWER · OF · HANOI 분할
        setBottomPanel    (0, "🎮 TOWER",               Color3B(100, 200, 255), 15.0f);
        colorCycleBottomPanel(0);
        slideInBottomPanel(1, "⚡ OF",            false, Color3B(255, 210,  50), 15.0f);
        slideInBottomPanel(2, "🏰 HANOI",         true,  Color3B(255, 120,  80), 15.0f);
        break;
    case 4:
        // 혼합 — 소형 설명 + 대형 레벨번호
        typingBottomPanel (0, "⏱ BEST TIME?",   0.07f, Color3B(255, 210,  50), 10.0f);
        setBottomPanel    (1, "⭐ CHALLENGE",            Color3B( 80, 220, 180), 10.0f);
        blinkBottomPanel  (1, 0.4f);
        setBottomPanel    (2, lv,                        Color3B(255, 215,   0), 28.0f);
        pulseBottomPanel  (2);
        break;
    case 5:
        // 초대형(30) — 이모지 하나씩, 패널을 꽉 채움
        setBottomPanel    (0, "⚡",                      Color3B(255, 210,  50), 30.0f);
        shakeBottomPanel  (0);
        slideInBottomPanel(1, "START",            true,  Color3B( 80, 220, 180), 28.0f);
        setBottomPanel    (2, "🏆",                      Color3B(255, 215,   0), 30.0f);
        colorCycleBottomPanel(2);
        break;
    case 6:
        // 초대형(32) — 레벨번호 + 이모지 스폿라이트
        setBottomPanel    (0, "🎯",                      Color3B( 80, 220, 180), 32.0f);
        pulseBottomPanel  (0);
        setBottomPanel    (1, lv,                        Color3B(255, 255, 255), 26.0f);
        colorCycleBottomPanel(1);
        slideInBottomPanel(2, "🔥",               false, Color3B(255,  80,  80), 32.0f);
        break;
    }
}

void PlayScene::startIdleAnimation()
{
    clearBottomPanels();
    _showNextIdleScene();
    schedule([this](float){ _showNextIdleScene(); }, 3.5f, "idle_anim");
}

void PlayScene::stopIdleAnimation()
{
    unschedule("idle_anim");
    unschedule("podium_to_idle");
    unschedule("marquee_cycle");
    unschedule("marquee_next");
    unschedule("cheer_anim");
    unschedule("cheer_start");
    unschedule("eq_update");
    this->stopActionByTag(TAG_GUIDE_ANIM);
    clearBottomPanels();
}

// ── 플레이 중 응원 연출 ──────────────────────────────────────────────
void PlayScene::startCheerAnimation()
{
    clearBottomPanels();
    _showNextCheerScene();
    schedule([this](float){ _showNextCheerScene(); }, 3.5f, "cheer_anim");
}

void PlayScene::_showNextCheerScene()
{
    static const char* msgs[] = {
        "GO! GO! 🔥",
        "NICE! ⭐",
        "YOU GOT IT!",
        "KEEP GOING!",
        "AMAZING! 🔥",
        "BRILLIANT!",
        "DON'T STOP!",
        "FOCUS! 🎯",
        "NICE MOVE!",
        "UNSTOPPABLE!",
        "SO CLOSE! 🏆",
        "LEGEND! ⭐",
        "GO FOR IT!",
        "CRUSHING IT!",
        "GENIUS! 🔥",
    };
    static const Color3B cols[] = {
        Color3B(255, 100,  60),   // 오렌지레드
        Color3B(255, 215,   0),   // 골드
        Color3B( 80, 220, 180),   // 청록
        Color3B(255,  80, 180),   // 핫핑크
        Color3B(100, 200, 255),   // 스카이블루
    };
    static const int NMSG = 15, NCOL = 5;

    for (int i = 0; i < 3; ++i) {
        int m = rand() % NMSG;
        int c = rand() % NCOL;
        float fs = 9.5f + (float)(rand() % 3);  // 9.5~11.5px
        switch (rand() % 4) {
            case 0: typingBottomPanel(i, msgs[m], 0.07f, cols[c], fs);                      break;
            case 1: setBottomPanel(i, msgs[m], cols[c], fs); blinkBottomPanel(i, 0.4f);     break;
            case 2: slideInBottomPanel(i, msgs[m], i % 2 == 0, cols[c], fs);                break;
            case 3: setBottomPanel(i, msgs[m], cols[c], fs + 1); pulseBottomPanel(i);       break;
        }
    }
}

// ── Level 4+ 올림픽 시상대: 하단 전광판 A=2위(은) B=1위(금) C=3위(동) ─
// 올림픽 시상대: A=2위(은) B=1위(금) C=3위(동)
// 깃발: 1위만 높게, 2·3위 동일 높이, 위에서 슬라이드다운
// 이름: 3패널 동일 Y 고정 표시, 순위번호 없음
void PlayScene::showPodiumRanking(const std::vector<LeaderboardEntry>& entries)
{
    clearBottomPanels();
    if (entries.empty()) {
        // 랭커 없음 → 마르퀴↔패널 순차 연출 (병렬 없음)
        m_noRankPhase = 0;
        _noRankNextScene(m_countOfDiscus);
        return;
    }

    struct Slot {
        int         pole;
        int         entryIdx;
        Color3B     color;
        float       flagY;   // 깃발 최종 Y
        const char* medal;   // 깃발 앞 메달 (🥇🥈🥉)
    };

    // 깃발 Y: 1위만 높게, 2·3위 동일
    const float FLAG_Y_GOLD   = BOTTOM_PANEL_Y + 10.0f;
    const float FLAG_Y_SILVER = BOTTOM_PANEL_Y - 1.0f;
    const float FLAG_Y_BRONZE = BOTTOM_PANEL_Y - 1.0f;  // 2위와 동일
    const float NAME_Y        = BOTTOM_PANEL_Y - 14.0f; // 전 패널 동일

    const Slot slots3[] = {
        { 0, 1, Color3B(192, 192, 192), FLAG_Y_SILVER, "🥈" },  // 패널A = 2위(은)
        { 1, 0, Color3B(255, 215,   0), FLAG_Y_GOLD,   "🥇" },  // 패널B = 1위(금)
        { 2, 2, Color3B(205, 127,  50), FLAG_Y_BRONZE, "🥉" },  // 패널C = 3위(동)
    };

    // START 버튼이 중앙 셀을 덮는 동안에는 패널A에 1위만 크게 두고,
    // 2·3위는 패널C에 작게 두 줄로 몰아 넣는다 (가려지는 자리에 1위를 둘 수 없으므로).
    const Slot slots2[] = {
        { 0, 0, Color3B(255, 215,   0), FLAG_Y_GOLD,   "🥇" },  // 패널A = 1위(금)
    };

    const bool  centerBlocked = (m_startMenu != nullptr);
    const Slot* slots = centerBlocked ? slots2 : slots3;
    const int   slotCount = centerBlocked ? 1 : 3;

    Label* const mainLabels[] = { m_bottomLabelA, m_bottomLabelB, m_bottomLabelC };

    for (int si = 0; si < slotCount; ++si) {
        const Slot& s = slots[si];
        if (s.entryIdx >= (int)entries.size()) {
            // 해당 순위 랭커 없음 → 패널에 랜덤 예제 연출
            const char* texts[]  = { "🏆 PLAY!", "⭐ RANK IT", "🔥 BE #1!", "🎮 JOIN!" };
            const Color3B cols[] = {
                Color3B( 80, 220, 180), Color3B(255, 210,  50),
                Color3B(255, 100,  80), Color3B(100, 200, 255)
            };
            int r = rand() % 4;
            switch (r) {
                case 0: typingBottomPanel(s.pole, texts[r], 0.1f, cols[r], 11.0f);                          break;
                case 1: setBottomPanel(s.pole, texts[r], cols[r], 11.0f); blinkBottomPanel(s.pole, 0.5f);   break;
                case 2: slideInBottomPanel(s.pole, texts[r], s.pole % 2 == 0, cols[r], 11.0f);              break;
                case 3: setBottomPanel(s.pole, texts[r], cols[r], 13.0f); pulseBottomPanel(s.pole);         break;
            }
            continue;
        }
        const auto& e = entries[s.entryIdx];

        std::string cc   = e.countryCode;
        for (char& c : cc) c = (char)toupper((unsigned char)c);
        // 메달 + 국기를 한 라벨에 — 이 라벨은 WHITE라 두 이모지 모두 원본색으로 나온다
        std::string flag = std::string(s.medal) + (cc.empty() ? "" : countryToFlag(cc));
        std::string name = e.displayName.empty() ? "???" : e.displayName;

        float cx = arrPosOfPole[s.pole].x;

        // ── 깃발: 위에서 슬라이드다운 + FadeIn ──────────────────
        const float flagStartY = s.flagY - 50.0f;  // 50px 아래에서 상승 시작

        Label* flagLbl = Label::createWithSystemFont(flag, "Arial", 22.0f);
        flagLbl->setColor(Color3B::WHITE);  // 이모지 원본색 유지
        flagLbl->setAnchorPoint(Vec2(0.5f, 0.5f));
        flagLbl->setHorizontalAlignment(TextHAlignment::CENTER);
        flagLbl->setOpacity(0);
        flagLbl->setPosition(Vec2(cx, flagStartY));
        flagLbl->setTag(TAG_PODIUM_BASE + s.pole * 2);
        this->addChild(flagLbl, 2);  // LED 도트(z=3) 아래에 렌더링

        // A→B→C 순으로 0.4s 시차
        float delay = s.pole * 0.4f;
        flagLbl->runAction(Sequence::create(
            DelayTime::create(delay),
            Spawn::create(
                EaseOut::create(MoveTo::create(2.8f, Vec2(cx, s.flagY)), 3.0f),
                FadeIn::create(1.4f),
                nullptr),
            nullptr));

        // ── 이름: 전 패널 동일 Y, 애니 없음 ─────────────────────
        Label* nameLbl = mainLabels[s.pole];
        nameLbl->stopAllActions();
        nameLbl->setVisible(true);
        nameLbl->setOpacity(255);
        nameLbl->setScale(1.0f);
        nameLbl->setSystemFontSize(11.0f);
        nameLbl->setAnchorPoint(Vec2(0.5f, 0.5f));
        nameLbl->setHorizontalAlignment(TextHAlignment::CENTER);
        nameLbl->setPosition(Vec2(cx, NAME_Y));
        nameLbl->setColor(s.color);
        nameLbl->setString(name);
    }

    // ── START로 중앙이 막힌 배치: 패널C에 2·3위를 소형 2줄로 ──────────────
    // 깃발과 이름을 한 라벨에 합치면 setColor가 이모지까지 물들이므로 라벨을 분리하고,
    // 깃발은 우측정렬·이름은 좌측정렬로 붙여 한 줄처럼 보이게 한다.
    if (centerBlocked)
    {
        const float cx = arrPosOfPole[2].x;
        const struct { int idx; float y; Color3B col; const char* medal; } rows[] = {
            { 1, BOTTOM_PANEL_Y +  9.0f, Color3B(192, 192, 192), "🥈" },   // 2위(은)
            { 2, BOTTOM_PANEL_Y -  9.0f, Color3B(205, 127,  50), "🥉" },   // 3위(동)
        };

        for (int r = 0; r < 2; ++r)
        {
            if (rows[r].idx >= (int)entries.size()) continue;
            const auto& e = entries[rows[r].idx];

            std::string cc = e.countryCode;
            for (char& c : cc) c = (char)toupper((unsigned char)c);

            std::string badge = std::string(rows[r].medal) + (cc.empty() ? "" : countryToFlag(cc));
            Label* fl = Label::createWithSystemFont(badge, "Arial", 13.0f);
            fl->setColor(Color3B::WHITE);                 // 메달·국기 이모지 원본색 유지
            fl->setAnchorPoint(Vec2(1.0f, 0.5f));
            fl->setOpacity(0);
            fl->setPosition(Vec2(cx - 15.0f, rows[r].y));   // 메달+국기 폭(≈28)만큼 좌측으로
            fl->setTag(TAG_PODIUM_BASE + 5 + r * 2);
            this->addChild(fl, 2);

            Label* nl = Label::createWithSystemFont(
                e.displayName.empty() ? "???" : e.displayName, "Arial", 10.0f);
            nl->setColor(rows[r].col);
            nl->setAnchorPoint(Vec2(0.0f, 0.5f));
            nl->setOpacity(0);
            nl->setPosition(Vec2(cx - 10.0f, rows[r].y));
            nl->setTag(TAG_PODIUM_BASE + 6 + r * 2);
            this->addChild(nl, 2);

            // 1위(패널A) 등장 후 은→동 순으로 페이드인
            float delay = 0.8f + r * 0.35f;
            fl->runAction(Sequence::create(DelayTime::create(delay), FadeIn::create(0.9f), nullptr));
            nl->runAction(Sequence::create(DelayTime::create(delay), FadeIn::create(0.9f), nullptr));
        }
    }

    // 5~8초 시상대 감상 후 idle 애니로 전환
    auto alive = m_aliveFlag;
    float podiumSec = 5.0f + static_cast<float>(rand() % 4);
    this->scheduleOnce([this, alive](float) {
        if (!alive || !*alive) return;
        if (m_isIng != NONE) return;
        startIdleAnimation();
    }, podiumSec, "podium_to_idle");
}

// ── 랭커 없는 레벨: 마르퀴 ↔ 패널 개별 연출 순차 전환 ──────────────
void PlayScene::_noRankNextScene(int lv)
{
    if (m_isIng != NONE) return;

    this->removeChildByTag(TAG_PODIUM_BASE, true);
    for (int i = 0; i < 3; ++i) stopBottomPanel(i);

    auto alive = m_aliveFlag;

    if (m_noRankPhase % 2 == 0) {
        // ── 짝수: 대형 마르퀴 슬라이딩 ────────────────────────────
        struct M { const char* fmt; Color3B color; };
        const M msgs[3] = {
            { "🔥  LEVEL %d : NO RECORD  ·  BE THE 1ST!  🏆  ·  ", Color3B(255, 215,   0) },
            { "⭐  YOUR NAME ON TOP  ·  DARE TO WIN?  🔥  THRONE AWAITS  ·  ", Color3B( 80, 220, 180) },
            { "🏆  NO CHAMPION YET  ·  WILL IT BE YOU?  ·  MAKE HISTORY!  🔥  ·  ", Color3B(255, 120,  60) },
        };
        int idx = (m_noRankPhase / 2) % 3;
        std::string txt = (idx == 0)
            ? StringUtils::format(msgs[0].fmt, lv)
            : std::string(msgs[idx].fmt);

        const float startX = 660.0f, endX = -380.0f, slideSec = 9.0f;

        Label* lbl = Label::createWithSystemFont(txt, "Arial", 22.0f);
        lbl->setColor(msgs[idx].color);
        lbl->setAnchorPoint(Vec2(0.5f, 0.5f));
        lbl->setOpacity(0);
        lbl->setPosition(Vec2(startX, BOTTOM_PANEL_Y));
        lbl->setTag(TAG_PODIUM_BASE);
        this->addChild(lbl, 2);  // LED 도트(z=3) 아래

        lbl->runAction(Sequence::create(
            FadeIn::create(0.5f),
            MoveTo::create(slideSec, Vec2(endX, BOTTOM_PANEL_Y)),
            FadeOut::create(0.3f),
            nullptr));

        ++m_noRankPhase;
        this->scheduleOnce([this, lv, alive](float) {
            if (!alive || !*alive) return;
            _noRankNextScene(lv);
        }, slideSec + 0.5f, "marquee_next");

    } else {
        // ── 홀수: 각 패널 개별 랜덤 연출 ──────────────────────────
        const char* texts[] = { "🏆 PLAY!", "⭐ RANK IT", "🔥 BE #1!", "🎮 JOIN!" };
        const Color3B cols[] = {
            Color3B( 80, 220, 180), Color3B(255, 210,  50),
            Color3B(255, 100,  80), Color3B(100, 200, 255)
        };
        for (int i = 0; i < 3; ++i) {
            int r = rand() % 4;
            switch (r) {
                case 0: typingBottomPanel(i, texts[r], 0.1f, cols[r], 11.0f);                       break;
                case 1: setBottomPanel(i, texts[r], cols[r], 11.0f); blinkBottomPanel(i, 0.5f);     break;
                case 2: slideInBottomPanel(i, texts[r], i % 2 == 0, cols[r], 11.0f);                break;
                case 3: setBottomPanel(i, texts[r], cols[r], 13.0f); pulseBottomPanel(i);           break;
            }
        }
        ++m_noRankPhase;
        this->scheduleOnce([this, lv, alive](float) {
            if (!alive || !*alive) return;
            _noRankNextScene(lv);
        }, 4.5f, "marquee_next");
    }
}

// ── Level 3 전용 가이드: A→B→C 순서로 페이징 ───────────────────────
// 정보1(청록) → 정보2(황금) → 정보3(주황) 순환
// 같은 페이지 안에서는 동일 색상, 페이지 전환 시 색상 변경
void PlayScene::startGuideAnimation()
{
    clearBottomPanels();
    auto alive = m_aliveFlag;

    const float T_IN   = 1.2f;   // 패널 하나씩 등장하는 간격
    const float T_HOLD = 3.5f;   // 세 패널 모두 보이는 유지 시간
    const float T_GAP  = 0.6f;   // 페이지 사이 암전 간격

    // 페이지별 색상 — 페이징 느낌이 확실히 나도록 뚜렷하게 구분
    const Color3B c1( 80, 220, 180);   // 정보1: 청록  (디스크 선택)
    const Color3B c2(255, 210,  50);   // 정보2: 황금  (디스크 이동)
    const Color3B c3(255, 100,  80);   // 정보3: 붉은주황 (디스크 배치)

    auto seq = Sequence::create(

        // ══ 정보1: 전광판을 터치해서 디스크를 선택하세요 ══
        CallFunc::create([this, alive, c1]{
            if (!*alive) return;
            clearBottomPanels();
            setBottomPanel(0, "👆 TOUCH HERE", c1, 12.0f);
            blinkBottomPanel(0, 0.7f);              // A 깜빡임 — "이곳을 눌러보세요"
        }),
        DelayTime::create(T_IN),
        CallFunc::create([this, alive, c1]{
            if (!*alive) return;
            setBottomPanel(1, "TO SELECT",     c1, 12.0f);
        }),
        DelayTime::create(T_IN),
        CallFunc::create([this, alive, c1]{
            if (!*alive) return;
            setBottomPanel(2, "A DISC",        c1, 12.0f);
        }),
        DelayTime::create(T_HOLD),

        // ══ 정보2: 선택한 디스크를 옆 폴로 이동하세요 ══
        CallFunc::create([this, alive, c2]{
            if (!*alive) return;
            clearBottomPanels();
            setBottomPanel(0, "DISC SELECTED!", c2, 11.0f);
        }),
        DelayTime::create(T_IN),
        CallFunc::create([this, alive, c2]{
            if (!*alive) return;
            setBottomPanel(1, "MOVE ➡",        c2, 13.0f);
        }),
        DelayTime::create(T_IN),
        CallFunc::create([this, alive, c2]{
            if (!*alive) return;
            setBottomPanel(2, "THIS POLE",     c2, 11.0f);
        }),
        DelayTime::create(T_HOLD),

        // ══ 정보3: 전광판을 터치해서 디스크를 내려놓으세요 ══
        CallFunc::create([this, alive, c3]{
            if (!*alive) return;
            clearBottomPanels();
            setBottomPanel(0, "PLACE DISC",    c3, 12.0f);
        }),
        DelayTime::create(T_IN),
        CallFunc::create([this, alive, c3]{
            if (!*alive) return;
            setBottomPanel(1, "TAP BELOW",     c3, 12.0f);
        }),
        DelayTime::create(T_IN),
        CallFunc::create([this, alive, c3]{
            if (!*alive) return;
            setBottomPanel(2, "👇 HERE!",      c3, 12.0f);
            blinkBottomPanel(2, 0.7f);              // C 깜빡임 — "이곳을 눌러보세요"
        }),
        DelayTime::create(T_HOLD),

        // 페이지 암전 후 다음 루프
        CallFunc::create([this, alive]{ if (*alive) clearBottomPanels(); }),
        DelayTime::create(T_GAP),
        nullptr
    );

    auto repeatAction = RepeatForever::create(seq);
    repeatAction->setTag(TAG_GUIDE_ANIM);
    this->runAction(repeatAction);
}
