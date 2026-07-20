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

static const float BOTTOM_FONT_DEFAULT = 10.0f;

// ─── HUD 티커 / 결과 표시 ───────────────────────────────────────

void PlayScene::startRankTicker(int level)
{
    auto alive = m_aliveFlag;

    // 인삿말 + 플레이어명 prefix — 비동기 콜백 전에 동기적으로 구성
    std::string greeting   = getLocalGreeting();
    std::string playerName = UserDataManager::Instance()->GetUserName();
    if (playerName.empty()) playerName = "PLAYER";
    std::string prefix = greeting + "!  " + playerName + ".          ";

    LeaderboardManager::Instance()->fetchLeaderboard(level, 10,
        [this, alive, prefix, level](const std::vector<LeaderboardEntry>& entries) {
            if (!alive || !*alive) return;
            if (m_isIng != NONE) return;

            // 상단 티커
            std::string rankText;
            if (entries.empty()) {
                rankText = "---  BE THE FIRST TO RANK!  ---";
            } else {
                for (const auto& e : entries) {
                    if (!rankText.empty()) rankText += "   |   ";
                    std::string cc = e.countryCode.empty() ? "--" : e.countryCode;
                    for (char& c : cc) c = (char)toupper((unsigned char)c);
                    rankText += StringUtils::format("%d.%s%s", e.rank, countryToFlag(cc).c_str(), e.displayName.c_str());
                }
            }

            if (m_hudTickerLabel) {
                m_hudTickerLabel->setString(prefix + rankText);
                m_hudTickerLabel->setVisible(true);
                tickerScrollStep();
            }

            // 하단 전광판 — Level 4 이상: 올림픽 시상대
            if (level >= 4) {
                stopIdleAnimation();
                showPodiumRanking(entries);
            }
        });
}

void PlayScene::tickerScrollStep()
{
    if (!m_hudTickerLabel || !m_hudTickerLabel->getParent()) return;
    if (m_isIng != NONE) return;

    const float SPEED = 80.0f;
    float labelW   = m_hudTickerLabel->getContentSize().width;
    float startX   = RESOURCE_WIDTH + 5.0f;
    float endX     = -(labelW + 5.0f);
    float duration = (startX - endX) / SPEED;

    m_hudTickerLabel->setAnchorPoint(Vec2(0, 0.5f));
    m_hudTickerLabel->setPositionX(startX);
    m_hudTickerLabel->stopAllActions();
    m_hudTickerLabel->runAction(Sequence::create(
        MoveTo::create(duration, Vec2(endX, RESOURCE_HEIGHT - 13.0f)),
        CallFunc::create(CC_CALLBACK_0(PlayScene::tickerScrollStep, this)),
        nullptr
    ));
}

void PlayScene::stopRankTicker()
{
    if (m_hudTickerLabel) {
        m_hudTickerLabel->stopAllActions();
        m_hudTickerLabel->setVisible(false);
    }
}

void PlayScene::setBottomPanel(int pole, const std::string& text, Color3B color, float fontSize)
{
    Label* const labels[] = { m_bottomLabelA, m_bottomLabelB, m_bottomLabelC };
    if (pole < 0 || pole > 2 || !labels[pole]) return;
    Label* lbl = labels[pole];
    lbl->stopAllActions();
    lbl->setVisible(true);
    lbl->setOpacity(255);
    lbl->setScale(1.0f);
    lbl->setPosition(Vec2(arrPosOfPole[pole].x, BOTTOM_PANEL_Y));
    lbl->setSystemFontSize(fontSize);
    lbl->setString(text);
    lbl->setColor(color);
}

void PlayScene::blinkBottomPanel(int pole, float interval)
{
    Label* const labels[] = { m_bottomLabelA, m_bottomLabelB, m_bottomLabelC };
    if (pole < 0 || pole > 2 || !labels[pole]) return;
    Label* lbl = labels[pole];
    lbl->stopAllActions();
    lbl->setVisible(true);
    lbl->runAction(RepeatForever::create(
        Sequence::create(
            FadeOut::create(interval),
            FadeIn::create(interval),
            nullptr)));
}

void PlayScene::stopBottomPanel(int pole)
{
    Label* const labels[] = { m_bottomLabelA, m_bottomLabelB, m_bottomLabelC };
    if (pole < 0 || pole > 2 || !labels[pole]) return;
    Label* lbl = labels[pole];
    lbl->stopAllActions();
    lbl->setVisible(true);
    lbl->setOpacity(255);
    lbl->setScale(1.0f);
    lbl->setSystemFontSize(BOTTOM_FONT_DEFAULT);
    lbl->setPosition(Vec2(arrPosOfPole[pole].x, BOTTOM_PANEL_Y));
}

void PlayScene::clearBottomPanels()
{
    for (int i = 0; i < 3; ++i)
    {
        stopBottomPanel(i);
        setBottomPanel(i, "");
    }
    // 시상대 동적 레이블 제거
    for (int t = TAG_PODIUM_BASE; t < TAG_PODIUM_BASE + 6; ++t)
        this->removeChildByTag(t, true);
}

// 한 글자씩 타이핑되듯 등장 (UTF-8 멀티바이트/이모지 지원)
void PlayScene::typingBottomPanel(int pole, const std::string& text, float charInterval, cocos2d::Color3B color, float fontSize)
{
    Label* const labels[] = { m_bottomLabelA, m_bottomLabelB, m_bottomLabelC };
    if (pole < 0 || pole > 2 || !labels[pole]) return;
    Label* lbl = labels[pole];
    stopBottomPanel(pole);
    lbl->setSystemFontSize(fontSize);
    lbl->setColor(color);
    lbl->setString("");

    std::vector<size_t> ends;
    for (size_t i = 0; i < text.size(); ) {
        unsigned char c = (unsigned char)text[i];
        size_t n = (c < 0x80) ? 1 : (c < 0xE0) ? 2 : (c < 0xF0) ? 3 : 4;
        i += n;
        ends.push_back(i);
    }

    Vector<FiniteTimeAction*> acts;
    for (size_t idx = 0; idx < ends.size(); ++idx) {
        std::string partial = text.substr(0, ends[idx]);
        acts.pushBack(DelayTime::create(charInterval));
        acts.pushBack(CallFunc::create([lbl, partial]{ lbl->setString(partial); }));
    }
    lbl->runAction(Sequence::create(acts));
}

// 좌/우에서 미끄러져 들어오기
void PlayScene::slideInBottomPanel(int pole, const std::string& text, bool fromLeft, cocos2d::Color3B color, float fontSize)
{
    Label* const labels[] = { m_bottomLabelA, m_bottomLabelB, m_bottomLabelC };
    if (pole < 0 || pole > 2 || !labels[pole]) return;
    Label* lbl = labels[pole];
    stopBottomPanel(pole);
    lbl->setSystemFontSize(fontSize);
    lbl->setColor(color);
    lbl->setString(text);

    Vec2 rest(arrPosOfPole[pole].x, BOTTOM_PANEL_Y);
    lbl->setPosition(rest + Vec2(fromLeft ? -130.0f : 130.0f, 0));
    lbl->runAction(EaseOut::create(MoveTo::create(0.35f, rest), 2.5f));
}

// 크기가 쿵쿵 맥박치듯 강조
void PlayScene::pulseBottomPanel(int pole)
{
    Label* const labels[] = { m_bottomLabelA, m_bottomLabelB, m_bottomLabelC };
    if (pole < 0 || pole > 2 || !labels[pole]) return;
    Label* lbl = labels[pole];
    lbl->stopAllActions();
    lbl->setScale(1.0f);
    lbl->runAction(RepeatForever::create(
        Sequence::create(
            ScaleTo::create(0.2f, 1.35f),
            ScaleTo::create(0.2f, 1.0f),
            DelayTime::create(0.7f),
            nullptr)));
}

// 색상이 무지개처럼 순환
void PlayScene::colorCycleBottomPanel(int pole)
{
    Label* const labels[] = { m_bottomLabelA, m_bottomLabelB, m_bottomLabelC };
    if (pole < 0 || pole > 2 || !labels[pole]) return;
    Label* lbl = labels[pole];
    lbl->stopAllActions();
    lbl->runAction(RepeatForever::create(
        Sequence::create(
            TintTo::create(0.35f, 255,  80,  80),
            TintTo::create(0.35f, 255, 200,  50),
            TintTo::create(0.35f,  80, 220, 180),
            TintTo::create(0.35f, 100, 160, 255),
            TintTo::create(0.35f, 200,  80, 255),
            nullptr)));
}

// 좌우로 짧게 흔들기 (경고/실수 연출)
void PlayScene::shakeBottomPanel(int pole)
{
    Label* const labels[] = { m_bottomLabelA, m_bottomLabelB, m_bottomLabelC };
    if (pole < 0 || pole > 2 || !labels[pole]) return;
    Label* lbl = labels[pole];
    lbl->stopAllActions();
    lbl->runAction(RepeatForever::create(
        Sequence::create(
            MoveBy::create(0.04f, Vec2( 4, 0)),
            MoveBy::create(0.04f, Vec2(-8, 0)),
            MoveBy::create(0.04f, Vec2( 8, 0)),
            MoveBy::create(0.04f, Vec2(-4, 0)),
            DelayTime::create(1.5f),
            nullptr)));
}

// 세 패널에 서로 다른 효과를 동시에 실행하는 데모
void PlayScene::demoBottomPanels()
{
    auto alive = m_aliveFlag;

    // 패널A: 타이핑 등장 → 완료 후 pulse
    typingBottomPanel(0, "🎯 HANOI", 0.09f, Color3B(80, 220, 180));
    float typingDur = 8 * 0.09f + 0.1f;   // 글자수 * interval
    this->runAction(Sequence::create(
        DelayTime::create(typingDur),
        CallFunc::create([this, alive]{ if (*alive) pulseBottomPanel(0); }),
        nullptr));

    // 패널B: 왼쪽에서 슬라이드 인 → 완료 후 blink
    slideInBottomPanel(1, "⚡ READY?", true, Color3B(255, 210, 50));
    this->runAction(Sequence::create(
        DelayTime::create(0.5f),
        CallFunc::create([this, alive]{ if (*alive) blinkBottomPanel(1, 0.45f); }),
        nullptr));

    // 패널C: 색상 사이클 (shake와 colorCycle은 stopAllActions 충돌로 동시 불가 — 별도 사용)
    setBottomPanel(2, "🏆 PLAY!", Color3B(255, 80, 80));
    colorCycleBottomPanel(2);
}

// ── 이퀄라이저 ──────────────────────────────────────────────────────

void PlayScene::startEqualizerAnimation()
{
    clearBottomPanels();
    memset(m_eqH,    0, sizeof(m_eqH));
    memset(m_eqPeak, 0, sizeof(m_eqPeak));

    if (!m_eqNode) {
        m_eqNode = DrawNode::create();
        m_eqNode->setPosition(Vec2::ZERO);
        this->addChild(m_eqNode, 4);  // LED dot overlay(z=3) 위
    }
    schedule([this](float dt){ _updateEqualizer(dt); }, 0.05f, "eq_update");
}

void PlayScene::stopEqualizerAnimation()
{
    unschedule("eq_update");
    if (m_eqNode) {
        m_eqNode->removeFromParent();
        m_eqNode = nullptr;
    }
    memset(m_eqH,    0, sizeof(m_eqH));
    memset(m_eqPeak, 0, sizeof(m_eqPeak));
}

void PlayScene::_updateEqualizer(float dt)
{
    if (!m_eqNode || !m_eqNode->getParent()) return;
    m_eqNode->clear();

    static const int   BARS     = 6;
    static const float BAR_BOT  = 5.0f;   // 패널 하단 여백
    static const float BAR_MAXH = 42.0f;  // 최대 막대 높이
    static const float DECAY    = 95.0f;  // 막대 낙하 속도 px/s
    static const float PK_DECAY = 28.0f;  // peak 낙하 속도 px/s

    const float DIV1 = (arrPosOfPole[0].x + arrPosOfPole[1].x) * 0.5f;
    const float DIV2 = (arrPosOfPole[1].x + arrPosOfPole[2].x) * 0.5f;
    const float sectX[3] = { 2.0f, DIV1 + 2.0f, DIV2 + 2.0f };
    const float sectW[3] = { DIV1 - 4.0f, DIV2 - DIV1 - 4.0f, RESOURCE_WIDTH - DIV2 - 4.0f };

    float maxH = BAR_MAXH * m_bgmCurrentVol;

    for (int s = 0; s < 3; ++s) {
        float barW = sectW[s] / (BARS * 2 - 1);  // bar 폭 = gap 폭
        float x0   = sectX[s];

        for (int i = 0; i < BARS; ++i) {
            // 낙하
            m_eqH[s][i] = std::max(0.0f, m_eqH[s][i] - DECAY * dt);

            // 볼륨 비례 랜덤 kick (50% 확률)
            if (rand() % 2 == 0) {
                float kick = ((float)(rand() % 1000) / 1000.0f) * maxH;
                if (kick > m_eqH[s][i]) m_eqH[s][i] = kick;
            }

            // peak: 막대가 올라가면 갱신, 아니면 천천히 낙하
            if (m_eqH[s][i] >= m_eqPeak[s][i])
                m_eqPeak[s][i] = m_eqH[s][i];
            else
                m_eqPeak[s][i] = std::max(0.0f, m_eqPeak[s][i] - PK_DECAY * dt);

            float h  = m_eqH[s][i];
            float pk = m_eqPeak[s][i];
            float x  = x0 + i * barW * 2.0f;
            float w  = barW * 0.82f;

            // 막대 색: 18개 전체를 하나의 레드→퍼플 연속 그라데이션
            if (h > 0.5f) {
                static const Color4F STOPS[6] = {
                    Color4F(1.00f, 0.22f, 0.22f, 1.0f),  // 레드
                    Color4F(1.00f, 0.55f, 0.05f, 1.0f),  // 오렌지
                    Color4F(1.00f, 0.92f, 0.05f, 1.0f),  // 옐로우
                    Color4F(0.15f, 1.00f, 0.40f, 1.0f),  // 그린
                    Color4F(0.10f, 0.82f, 1.00f, 1.0f),  // 시안
                    Color4F(0.78f, 0.18f, 1.00f, 1.0f),  // 퍼플
                };
                // 전체 18막대 기준 위치(0→1)로 인접 stop 보간
                float gt  = (float)(s * BARS + i) / (3 * BARS - 1);
                float pos = gt * 5.0f;
                int   ci  = std::min((int)pos, 4);
                float ct  = pos - (float)ci;
                Color4F n(
                    STOPS[ci].r + (STOPS[ci+1].r - STOPS[ci].r) * ct,
                    STOPS[ci].g + (STOPS[ci+1].g - STOPS[ci].g) * ct,
                    STOPS[ci].b + (STOPS[ci+1].b - STOPS[ci].b) * ct,
                    1.0f);
                float t   = h / BAR_MAXH;
                float dim = 0.22f + t * 0.78f;
                float a   = 0.45f + t * 0.55f;
                m_eqNode->drawSolidRect(Vec2(x, BAR_BOT), Vec2(x + w, BAR_BOT + h),
                    Color4F(n.r * dim, n.g * dim, n.b * dim, a));
            }

            // peak 흰 선 (색 대비 극대화)
            if (pk > 2.0f) {
                float py = BAR_BOT + pk;
                m_eqNode->drawSolidRect(
                    Vec2(x, py - 1.0f), Vec2(x + w, py + 1.0f),
                    Color4F(1.0f, 1.0f, 1.0f, 0.95f));
            }
        }
    }
}

// ────────────────────────────────────────────────────────────────────

void PlayScene::showHudResult(bool isNewRecord, const RecordTime& rt)
{
    if (!m_hudTickerLabel) return;

    m_hudTickerLabel->stopAllActions();
    m_hudTickerLabel->setOpacity(255);
    m_hudTickerLabel->setVisible(true);

    const char* praise = isNewRecord ? "Perfect!" : "Great!";
    Color3B color      = isNewRecord ? Color3B(255, 215, 80) : Color3B(80, 220, 180);
    const char* result = isNewRecord ? "NEW RECORD!" : "LEVEL CLEAR";
    std::string text = StringUtils::format("%s  %02d:%02d.%02d  %s",
        praise, rt.min, rt.sec, rt.ms, result);

    m_hudTickerLabel->setString(text);
    m_hudTickerLabel->setColor(color);
    m_hudTickerLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
    m_hudTickerLabel->setPosition(Vec2(RESOURCE_WIDTH * 0.65f, RESOURCE_HEIGHT - 13.0f));

    // 1초 간격 깜빡임
    m_hudTickerLabel->runAction(RepeatForever::create(Sequence::create(
        DelayTime::create(1.0f), Hide::create(),
        DelayTime::create(1.0f), Show::create(),
        nullptr
    )));
}
