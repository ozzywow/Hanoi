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

// A-1 잡음 이벤트 디바운스 임계(ms) — 이보다 빠른 FAIL/DESELECT는 녹화 스킵(봇 스팸 억제).
static const int REPLAY_JUNK_DEBOUNCE_MS = 40;

// 리플레이 배속 단계 (기본 x1 = 인덱스 1). 마지막 선택은 UserDefault "replay_speed"에 글로벌 저장.
static const float kReplaySpeeds[]  = { 0.5f, 1.0f, 1.5f, 2.0f, 4.0f };
static const int   kReplaySpeedCount = 5;

// 컨트롤 바 일시정지 칩 문구 (❚❚ PAUSE ↔ ▶ PLAY)
static const char* kReplayPauseText = "\xE2\x9D\x9A\xE2\x9D\x9A PAUSE";
static const char* kReplayPlayText  = "\xE2\x96\xB6 PLAY";

// 배속 화살표 — 더 조절할 단계가 있으면 꽉 찬 ◀/▶, 경계(x0.5·x4)면 속 빈 ◁/▷
static const char* kSpeedArrowLFull  = "\xE2\x97\x80";   // ◀
static const char* kSpeedArrowLEmpty = "\xE2\x97\x81";   // ◁
static const char* kSpeedArrowRFull  = "\xE2\x96\xB6";   // ▶
static const char* kSpeedArrowREmpty = "\xE2\x96\xB7";   // ▷
static const Color3B kSpeedArrowOn (180, 210, 255);
static const Color3B kSpeedArrowOff(110, 125, 150);      // 경계 — 흐리게

// 레이스 마커 스프링(가속·감속). K=강성(클수록 강한 가속), D=감쇠.
// 감쇠비 ζ = D/(2√K): ζ<1이면 목표를 살짝 지나쳤다 돌아옴(역동적). 여기선 ζ≈0.71 → 부드럽되 탄력.
// 더 튕기게 하려면 D를 줄이고, 더 빠릿하게 하려면 K를 키운다. RACE_SPRING_VMAX는 폭주 방지 상한(px/s).
static const float RACE_SPRING_K    = 300.0f;
static const float RACE_SPRING_D    = 24.5f;
static const float RACE_SPRING_VMAX = 900.0f;


// ---------------------------------------------------------------------------
// 리플레이 (1차: 내 리플레이 보기 + 로컬 영속화) — docs/replay_ghost_plan.md §5,§6
// ---------------------------------------------------------------------------

// Base64 (자체 구현 — 2차 PlayFab 저장에서도 재사용)
static const char* kB64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string b64encode(const std::vector<unsigned char>& d)
{
	std::string out;
	int val = 0, bits = -6;
	for (unsigned char c : d) {
		val = (val << 8) + c; bits += 8;
		while (bits >= 0) { out.push_back(kB64[(val >> bits) & 0x3F]); bits -= 6; }
	}
	if (bits > -6) out.push_back(kB64[((val << 8) >> (bits + 8)) & 0x3F]);
	while (out.size() % 4) out.push_back('=');
	return out;
}

static bool b64decode(const std::string& s, std::vector<unsigned char>& out)
{
	int T[256]; for (int i = 0; i < 256; ++i) T[i] = -1;
	for (int i = 0; i < 64; ++i) T[(unsigned char)kB64[i]] = i;
	int val = 0, bits = -8;
	for (unsigned char c : s) {
		if (c == '=') break;
		if (T[c] == -1) return false;
		val = (val << 6) + T[c]; bits += 6;
		if (bits >= 0) { out.push_back((unsigned char)((val >> bits) & 0xFF)); bits -= 8; }
	}
	return true;
}

// 리플레이 직렬화 → Base64  (docs/replay_ghost_plan.md §3)
//  헤더 6B: [u8 discus][u16 count][u24 finalMs]
//  이벤트 (병합 delta-varint, ms 해상도):
//    첫 바이트 = [type:2][pole:2][cont:1][Δ[0:2]:3]
//    cont=1 이면 이어서 LEB128 로 (Δ>>3) 의 상위 비트
//  ※ Δ = 직전 이벤트와의 시간차. 정수라 드리프트 0, 누적 합 = finalMs 로 정확 수렴.
//    120초 무입력이면 게임 자동 포기되므로 Δ ≤ 120s → 이벤트 최대 3바이트 (인코딩 시 clamp로 보장).
//    (REPLAY_MAX_DELTA_MS 정의는 파일 상단)
static void putVarint(std::vector<unsigned char>& b, uint32_t v)   // LEB128 unsigned
{
	while (v >= 0x80) { b.push_back((unsigned char)(v | 0x80)); v >>= 7; }
	b.push_back((unsigned char)v);
}

std::string encodeReplayBlob(const std::vector<ReplayEvent>& ev, int discus, int finalMs)
{
	std::vector<unsigned char> b;
	b.push_back((uint8_t)discus);
	uint16_t cnt = (uint16_t)ev.size();
	b.push_back((uint8_t)(cnt & 0xFF)); b.push_back((uint8_t)((cnt >> 8) & 0xFF));
	uint32_t fm = (uint32_t)finalMs;
	for (int i = 0; i < 3; ++i) b.push_back((uint8_t)((fm >> (i * 8)) & 0xFF));   // u24

	uint32_t encAcc = 0;   // 인코딩 누적(= 디코딩 재현값). clamp 시에도 재현 일관성 유지.
	for (const auto& e : ev) {
		uint32_t d = (e.t_ms > encAcc) ? (e.t_ms - encAcc) : 0;
		if (d > REPLAY_MAX_DELTA_MS) d = REPLAY_MAX_DELTA_MS;   // 방어적 clamp (정상 플레이엔 발생 안 함)
		encAcc += d;

		uint32_t rest = d >> 3;
		uint8_t first = (uint8_t)(((e.type & 0x3) << 6) | ((e.pole & 0x3) << 4)
		                          | ((rest ? 1u : 0u) << 3) | (d & 0x7));
		b.push_back(first);
		if (rest) putVarint(b, rest);
	}
	return b64encode(b);
}

static bool decodeReplayBlob(const std::string& b64, std::vector<ReplayEvent>& ev, int& discus, int& finalMs)
{
	std::vector<unsigned char> b;
	if (!b64decode(b64, b) || b.size() < 6) return false;
	size_t p = 0;
	auto avail = [&](size_t n){ return p + n <= b.size(); };

	discus = b[p++];
	if (discus < 3 || discus > MAX_PLAY_LEVEL) return false;   // 범위 밖 → 손상/구포맷 거부(fallback)
	uint16_t cnt = (uint16_t)(b[p] | (b[p+1] << 8)); p += 2;
	uint32_t fm  = b[p] | (b[p+1] << 8) | ((uint32_t)b[p+2] << 16); p += 3;   // u24
	finalMs = (int)fm;

	ev.clear();
	ev.reserve(cnt);
	uint32_t acc = 0;
	for (uint16_t i = 0; i < cnt; ++i) {
		if (!avail(1)) return false;
		uint8_t first = b[p++];
		uint32_t d = (uint32_t)(first & 0x7);
		if (first & 0x08) {                          // 연속 → LEB128 상위 비트
			uint32_t leb = 0; int shift = 0;
			while (true) {
				if (!avail(1) || shift > 28) return false;
				uint8_t c = b[p++];
				leb |= (uint32_t)(c & 0x7F) << shift;
				if (!(c & 0x80)) break;
				shift += 7;
			}
			d |= leb << 3;
		}
		acc += d;
		ReplayEvent e;
		e.type = (uint8_t)((first >> 6) & 0x3);
		e.pole = (uint8_t)((first >> 4) & 0x3);
		e.t_ms = acc;
		ev.push_back(e);
	}
	return true;
}

std::string replayKey(int level) { return StringUtils::format("replay_L%d", level); }

void PlayScene::recordReplayEvent(ReplayEventType type, int pole)
{
	if (m_isReplaying) return;            // 재생 중 발생 이벤트는 재녹화 안 함
	if (pole < 0 || pole > 2) return;
	if (m_replayOverflow) return;         // A-2: 이미 캡 초과 → 녹화 중단

	int t = getMilliCount() - m_dateTime;

	// A-1: 잡음 이벤트(FAIL/DESELECT)만 40ms 레이트 디바운스 — 봇 스팸 억제.
	//      게임 동작(사운드/UX)은 그대로, 녹화만 스킵. SELECT/MOVE_OK는 재생 정합성 위해 항상 녹화.
	if ((type == EV_MOVE_FAIL || type == EV_DESELECT) &&
	    (t - m_lastReplayEventMs) < REPLAY_JUNK_DEBOUNCE_MS)
		return;

	// A-2: 레벨별 하드 이벤트 캡. 초과 시 녹화 중단 + 저장불가 플래그(기록은 유효).
	//      cap = (최소이동 2^N-1)×4 + 256 → 정상 Top10(≈이동×2.1)엔 절대 안 걸림.
	int cap = ((1 << m_countOfDiscus) - 1) * 4 + 256;
	if ((int)m_replay.size() >= cap) { m_replayOverflow = true; return; }

	ReplayEvent ev;
	ev.type = (uint8_t)type;
	ev.pole = (uint8_t)pole;
	ev.t_ms = (uint32_t)t;
	m_replay.push_back(ev);
	m_lastReplayEventMs = t;
}

void PlayScene::startReplay()
{
	if (m_isReplaying) return;

	// 저장된 최고기록 리플레이 우선 로드 (없거나 손상 시 현재 세션 폴백)
	std::vector<ReplayEvent> loaded;
	int lvl = 0, finalMs = 0;
	std::string blob = UserDefault::getInstance()->getStringForKey(replayKey(m_countOfDiscus).c_str(), "");
	if (!blob.empty() && decodeReplayBlob(blob, loaded, lvl, finalMs)
	    && lvl == m_countOfDiscus && !loaded.empty()) {
		m_replay        = loaded;         // 최고기록 리플레이로 교체
		m_replayFinalMs = finalMs;
	} else if (!m_replay.empty()) {
		m_replayFinalMs = m_mastTime;     // 폴백: 방금 플레이한 세션
	} else {
		return;                           // 재생할 것이 없음
	}

	m_isReplaying = true;

	// 결과 팝업은 제거하지 않고 숨긴다 — 제거 후 재생성 시 MessagePopup이 점수를 재제출하기 때문 (docs §6.5)
	auto ov = this->getChildByTag(tagPopup);
	if (ov) ov->setVisible(false);

	// 보드 초기 상태 복원 (전 디스크 폴 0) 후 즉시 렌더 — 가드로 Finished 안 뜸
	this->ResetGame();
	this->DrawDiscus();
	this->SelectPole(-1, false);

	// 타이머 재생용으로 다시 표시
	if (m_labelTime) { m_labelTime->setVisible(true); m_labelTime->setString("00:00.00"); }

	if (m_isRace)
	{
		// 레이스 리플레이: 고스트 대결 화면 그대로 재현 — 트랙 바/마커 다시 켜고 재생 클럭에 동기.
		if (m_labelLevel)     m_labelLevel->setVisible(false);
		if (m_hudTickerLabel) m_hudTickerLabel->setVisible(false);   // 티커가 트랙과 겹치므로 숨김
		if (m_labelTime) {                                            // 시간은 레이스처럼 오른쪽 정렬 유지
			m_labelTime->setAnchorPoint(Vec2(1.0f, 0.5f));
			m_labelTime->setPosition(Vec2(RESOURCE_WIDTH - 10, RESOURCE_HEIGHT - 13.0f));
		}
		if (m_raceBar)       m_raceBar->setVisible(true);
		if (m_raceYouIcon)   m_raceYouIcon->setVisible(true);
		if (m_raceGhostIcon) m_raceGhostIcon->setVisible(true);
		m_youTargetX = m_ghostTargetX = -1.0f;   // 첫 프레임에 시작점으로 즉시 스냅 후 스프링
		m_youVel = m_ghostVel = 0.0f;
		m_ghostFinishedShown = true;             // 리플레이 중에는 GHOST FINISHED 연출 생략
		this->schedule([this](float dt){         // 마커 스프링(재스케줄, Finished에서 해제됐으므로)
			if (dt > 0.033f) dt = 0.033f;
			const float K = RACE_SPRING_K, D = RACE_SPRING_D;
			if (m_raceYouIcon && m_youTargetX >= 0.0f) {
				float x = m_raceYouIcon->getPositionX();
				m_youVel += ((m_youTargetX - x) * K - m_youVel * D) * dt;
				if (m_youVel >  RACE_SPRING_VMAX) m_youVel =  RACE_SPRING_VMAX;
				if (m_youVel < -RACE_SPRING_VMAX) m_youVel = -RACE_SPRING_VMAX;
				m_raceYouIcon->setPositionX(x + m_youVel * dt);
			}
			if (m_raceGhostIcon && m_ghostTargetX >= 0.0f) {
				float x = m_raceGhostIcon->getPositionX();
				m_ghostVel += ((m_ghostTargetX - x) * K - m_ghostVel * D) * dt;
				if (m_ghostVel >  RACE_SPRING_VMAX) m_ghostVel =  RACE_SPRING_VMAX;
				if (m_ghostVel < -RACE_SPRING_VMAX) m_ghostVel = -RACE_SPRING_VMAX;
				m_raceGhostIcon->setPositionX(x + m_ghostVel * dt);
			}
		}, "race_anim");
	}
	else
	{
		// 상단바 정리: 결과 텍스트/레벨 숨기고 REPLAY 인디케이터만 표시 (타이머와 겹침 방지)
		if (m_labelLevel) m_labelLevel->setVisible(false);
		if (m_hudTickerLabel)
		{
			m_hudTickerLabel->stopAllActions();
			m_hudTickerLabel->setString("\xE2\x96\xB6 REPLAY");   // ▶ REPLAY
			m_hudTickerLabel->setColor(Color3B(120, 200, 255));
			m_hudTickerLabel->setOpacity(255);
			m_hudTickerLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
			m_hudTickerLabel->setPosition(Vec2(RESOURCE_WIDTH * 0.5f, RESOURCE_HEIGHT - 13.0f));
			m_hudTickerLabel->setVisible(true);
		}
	}

	m_replayClock        = 0.0;
	m_replayIndex        = 0;
	m_replaySelectedPole = -1;
	m_replaySpeed        = 1.0f;

	// 하단 컨트롤 바 ([❚❚ PAUSE][◀ SPEED xN ▶][■ STOP])
	this->_buildReplayControlBar();

	this->schedule([this](float dt){ this->_updateReplay(dt); }, 0.0f, "replay_update");
}

void PlayScene::_updateReplay(float dt)
{
	if (m_replayPaused) return;                                     // 일시정지 — 클럭 정지

	m_replayClock += (double)dt * 1000.0 * (double)m_replaySpeed;   // ms (배속 반영)

	// playbackClock 에 도달한 이벤트를 순서대로 적용
	while (m_replayIndex < m_replay.size() &&
	       (double)m_replay[m_replayIndex].t_ms <= m_replayClock)
	{
		this->applyReplayEvent(m_replay[m_replayIndex]);
		++m_replayIndex;
	}

	// 타이머 갱신 (재생본의 최종 시간을 넘지 않게 캡)
	int shown = (int)m_replayClock;
	if (shown > m_replayFinalMs) shown = m_replayFinalMs;
	if (m_labelTime)
	{
		RecordTime rt = getRecordTime(shown);
		m_labelTime->setString(StringUtils::format("%02d:%02d.%02d", rt.min, rt.sec, rt.ms));
	}

	// 레이스 리플레이: 트랙 바/마커를 재생 클럭에 맞춰 갱신 (고스트도 함께 달림)
	if (m_isRace) this->_updateRaceHud();

	// 모든 이벤트 소진 + 최종 시간 도달 → 종료
	if (m_replayIndex >= m_replay.size() && m_replayClock >= (double)m_replayFinalMs)
	{
		this->endReplay();
	}
}

void PlayScene::applyReplayEvent(const ReplayEvent& ev, bool silent)
{
	// silent=true → 건너뛰기(fast-forward) 시 상태만 갱신, 소리/라이트/딜레이 생략
	int pole = ev.pole;
	switch (ev.type)
	{
	case EV_SELECT:
		m_replaySelectedPole = pole;
		if (!silent) {
			this->SelectPole(pole, true);                   // 라이트 ON
			SoundFactory::Instance()->play("efs_select_disc");
		}
		break;

	case EV_MOVE_OK:
		// 직전 SELECT 폴의 top 디스크를 목적 폴로 이동 (즉시 위치 변경 — 애니 없음)
		if (m_replaySelectedPole >= 0 && m_replaySelectedPole != pole &&
		    m_replaySelectedPole < (int)m_pole.size() && pole < (int)m_pole.size())
		{
			auto& src = m_pole[m_replaySelectedPole];
			if (!src.empty())
			{
				Discus* d = src.back();
				src.pop_back();
				d->SetCurrPoleID(pole);
				m_pole[pole].push_back(d);
			}
		}
		if (!silent) {
			this->DrawDiscus();                             // 위치 갱신 (가드로 Finished 안 뜸)
			this->SelectPole(-1, false);                    // 라이트 OFF
			SoundFactory::Instance()->play("efs_move_disc_ok");
		}
		m_replaySelectedPole = -1;
		break;

	case EV_MOVE_FAIL:
		if (!silent) {
			this->SelectPole(pole, false);                  // 실패 마크 (불 꺼짐 연출)
			SoundFactory::Instance()->play("efs_cancel_select");
			this->scheduleOnce([this](float){ this->SelectPole(-1, false); }, 0.2f, "replay_fail_off");
		}
		m_replaySelectedPole = -1;
		break;

	case EV_DESELECT:
		if (!silent) {
			this->SelectPole(-1, false);                    // 선택 취소, 라이트 OFF
			SoundFactory::Instance()->play("efs_move_disc_ok");
		}
		m_replaySelectedPole = -1;
		break;
	}
}

void PlayScene::skipReplay()
{
	if (!m_isReplaying) return;
	// 남은 이벤트를 무음으로 즉시 적용 → 최종(클리어) 상태로 점프
	for (; m_replayIndex < m_replay.size(); ++m_replayIndex)
		this->applyReplayEvent(m_replay[m_replayIndex], true);

	this->DrawDiscus();                                     // 최종 위치 렌더 (가드로 Finished 안 뜸)
	m_replayClock = (double)m_replayFinalMs;
	this->endReplay();
}

// 리플레이 컨트롤 바 (BottomInfoBar) — [❚❚ PAUSE][◀ SPEED xN ▶][■ STOP], 재생/관전 공용.
// 빈 화면 탭으로는 재생이 멈추지 않으므로(절전 방지 터치 오인) 조작은 이 버튼들로만 한다.
void PlayScene::_buildReplayControlBar()
{
	if (m_replaySpeedLabel) { m_replaySpeedLabel->removeFromParent(); m_replaySpeedLabel = nullptr; }
	if (m_replaySpeedMenu)  { m_replaySpeedMenu->removeFromParent();  m_replaySpeedMenu  = nullptr; }
	m_replayPauseItem   = nullptr;
	m_replaySpeedArrowL = nullptr;
	m_replaySpeedArrowR = nullptr;
	m_replayPaused      = false;

	// 하단 패널의 기존 표시(아이들 문구·시상대)를 비워 버튼과 겹치지 않게 한다
	this->clearBottomPanels();

	// 저장된 배속 로드 → 인덱스 매칭 (없거나 불일치 시 x1)
	float saved = UserDefault::getInstance()->getFloatForKey("replay_speed", 1.0f);
	m_replaySpeedIdx = 1;
	for (int i = 0; i < kReplaySpeedCount; ++i) {
		float d = kReplaySpeeds[i] - saved; if (d < 0) d = -d;
		if (d < 0.01f) { m_replaySpeedIdx = i; break; }
	}
	m_replaySpeed = kReplaySpeeds[m_replaySpeedIdx];

	const float cx = RESOURCE_WIDTH / 2, y = 26.0f;
	const float CHIP_W = 86.0f, CHIP_H = 26.0f, CHIP_FS = 12.0f;

	// 좌: 일시정지 ↔ 재개 토글
	m_replayPauseItem = makePopupChipButton(kReplayPauseText, kBtnFunc,
		[this](Ref*){ this->_toggleReplayPause(); }, CHIP_W, CHIP_H, CHIP_FS);

	// 중앙: 배속 스테퍼 (화살표 모양은 _updateReplaySpeedArrows가 경계에 맞춰 확정)
	auto leftLbl = Label::createWithSystemFont(kSpeedArrowLFull, "Arial", 16);   // ◀ 감속
	auto leftItem = MenuItemLabel::create(leftLbl, [this](Ref*){ this->_stepReplaySpeed(-1); });

	auto rightLbl = Label::createWithSystemFont(kSpeedArrowRFull, "Arial", 16);  // ▶ 가속
	auto rightItem = MenuItemLabel::create(rightLbl, [this](Ref*){ this->_stepReplaySpeed(+1); });

	m_replaySpeedArrowL = leftLbl;
	m_replaySpeedArrowR = rightLbl;

	// 우: 정지 — 재생 중단(최종 상태로 점프) → 결과/관전 종료 팝업 복원
	auto stopItem = makePopupChipButton("\xE2\x96\xA0 STOP", kBtnDismiss, [this](Ref*){
		SoundFactory::Instance()->play("efs_click");
		this->skipReplay();
	}, CHIP_W, CHIP_H, CHIP_FS);

	m_replaySpeedMenu = Menu::create(m_replayPauseItem, leftItem, rightItem, stopItem, nullptr);
	m_replaySpeedMenu->setPosition(Vec2::ZERO);
	m_replayPauseItem->setPosition(Vec2(arrPosOfPole[0].x, y));   // 좌측 구역 중앙
	leftItem->setPosition(Vec2(cx - 52, y));
	rightItem->setPosition(Vec2(cx + 52, y));
	stopItem->setPosition(Vec2(arrPosOfPole[2].x, y));            // 우측 구역 중앙

	m_replaySpeedLabel = Label::createWithSystemFont(
		StringUtils::format("SPEED x%g", (double)m_replaySpeed), "Arial", 14);
	m_replaySpeedLabel->setColor(Color3B(255, 220, 120));
	m_replaySpeedLabel->setPosition(Vec2(cx, y));

	this->addChild(m_replaySpeedMenu, 200);   // 터치레이어 위 → 빈 화면 탭과 분리
	this->addChild(m_replaySpeedLabel, 201);  // 라벨은 Menu 자식 불가(MenuItem만 허용) → 씬에 직접 부착

	this->_updateReplaySpeedArrows();
}

// 배속 경계 표시 — 더 낮출 수 없으면 ◁, 더 높일 수 없으면 ▷ (속 빈 + 흐린 색)
void PlayScene::_updateReplaySpeedArrows()
{
	bool canDown = (m_replaySpeedIdx > 0);
	bool canUp   = (m_replaySpeedIdx < kReplaySpeedCount - 1);

	if (m_replaySpeedArrowL) {
		m_replaySpeedArrowL->setString(canDown ? kSpeedArrowLFull : kSpeedArrowLEmpty);
		m_replaySpeedArrowL->setColor(canDown ? kSpeedArrowOn : kSpeedArrowOff);
	}
	if (m_replaySpeedArrowR) {
		m_replaySpeedArrowR->setString(canUp ? kSpeedArrowRFull : kSpeedArrowREmpty);
		m_replaySpeedArrowR->setColor(canUp ? kSpeedArrowOn : kSpeedArrowOff);
	}
}

void PlayScene::_stepReplaySpeed(int dir)
{
	int idx = m_replaySpeedIdx + dir;
	if (idx < 0) idx = 0;
	if (idx > kReplaySpeedCount - 1) idx = kReplaySpeedCount - 1;
	if (idx == m_replaySpeedIdx) { SoundFactory::Instance()->play("efs_cancel_select", 0.4f); return; }  // 경계

	m_replaySpeedIdx = idx;
	m_replaySpeed    = kReplaySpeeds[idx];
	if (m_replaySpeedLabel)
		m_replaySpeedLabel->setString(StringUtils::format("SPEED x%g", (double)m_replaySpeed));
	this->_updateReplaySpeedArrows();

	UserDefault::getInstance()->setFloatForKey("replay_speed", m_replaySpeed);   // 글로벌 저장
	UserDefault::getInstance()->flush();
	SoundFactory::Instance()->play("efs_click");
}

// 일시정지 ↔ 재개 — 클럭만 멈추므로 보드/타이머는 현재 프레임 그대로 유지
void PlayScene::_toggleReplayPause()
{
	if (!m_isReplaying) return;
	m_replayPaused = !m_replayPaused;

	if (m_replayPauseItem) {
		auto lbl = dynamic_cast<Label*>(m_replayPauseItem->getChildByTag(kChipLabelTag));
		if (lbl) lbl->setString(m_replayPaused ? kReplayPlayText : kReplayPauseText);
	}
	SoundFactory::Instance()->play("efs_click");
}

// ── 3차: 고스트 레이스 ────────────────────────────────────────────────
// distance-to-solved: poleOfSize[1..N]=디스크 크기별 폴, target=목표폴 → 최소 이동수
static int hanoiRemaining(const int* poleOfSize, int N, int target)
{
	int t = target, c = 0;
	for (int k = N; k >= 1; --k) {
		int p = poleOfSize[k];
		if (p != t) { c += (1 << (k - 1)); t = 3 - p - t; }
	}
	return c;
}

int PlayScene::_movesRemaining()
{
	int N = m_countOfDiscus;
	int ids[16], poles[16], n = 0;
	for (int p = 0; p < 3; ++p)
		for (auto d : m_pole[p]) { ids[n] = d->GetDiscusID(); poles[n] = p; ++n; }
	int poleOfSize[16];
	for (int i = 0; i < n; ++i) {                 // ⚠ 이 게임은 ID 클수록 작은 디스크 → 큰 ID일수록 rank 낮음
		int rank = 1;                             // rank N = 물리적으로 가장 큰 디스크 (= 가장 작은 ID)
		for (int j = 0; j < n; ++j) if (ids[j] > ids[i]) ++rank;
		poleOfSize[rank] = poles[i];
	}
	return hanoiRemaining(poleOfSize, N, 2);      // 목표 = 폴 2
}

void PlayScene::_setupRace()
{
	std::vector<ReplayEvent> ev; int lvl = 0, fm = 0;
	if (!decodeReplayBlob(m_ghostBlob, ev, lvl, fm) || ev.empty()) { m_isRace = false; return; }

	// 블롭과 점수를 항상 일치시킨다. 리더보드 scoreMs는 신기록 직후 캐시 지연으로
	// blob(구 기록)과 어긋날 수 있어 "고스트가 골 도달 전 GHOST FINISHED" 버그를 유발한다.
	// 실제 재생되는 고스트 blob의 최종 시간을 마커/골인/승패 판정의 단일 기준으로 사용. (docs §10)
	m_ghostScoreMs = fm;

	int N = m_countOfDiscus;
	m_raceTotalMoves = (1 << N) - 1;
	m_ghostReach.assign(m_raceTotalMoves + 1, 0x7fffffff);
	m_ghostReach[m_raceTotalMoves] = 0;           // 시작 시 남은수=total, t=0

	// 가상 보드(크기 1..N)로 고스트 이벤트 재생 → 각 MOVE_OK 후 남은수 기록
	std::vector<int> board[3];
	for (int s = N; s >= 1; --s) board[0].push_back(s);   // 폴0에 큰 것부터 바닥
	int sel = -1, minR = m_raceTotalMoves;
	for (const auto& e : ev) {
		if (e.type == EV_SELECT) sel = e.pole;
		else if (e.type == EV_MOVE_OK) {
			if (sel >= 0 && sel < 3 && !board[sel].empty() && e.pole < 3) {
				int d = board[sel].back(); board[sel].pop_back(); board[e.pole].push_back(d);
			}
			sel = -1;
			int poleOfSize[16];
			for (int p = 0; p < 3; ++p) for (int d : board[p]) poleOfSize[d] = p;
			int r = hanoiRemaining(poleOfSize, N, 2);
			if (r < minR) { for (int rr = r; rr < minR; ++rr) m_ghostReach[rr] = (int)e.t_ms; minR = r; }
		} else sel = -1;   // FAIL/DESELECT
	}

	m_ghostFinishedShown = false;

	// HUD: 트랙 바를 상단 정보바 안에. 레벨 숨김 + 시간 맨 오른쪽으로 공간 확보.
	if (m_labelLevel) m_labelLevel->setVisible(false);
	if (m_labelRPM)   m_labelRPM->setVisible(false);
	if (m_labelTime) {
		m_labelTime->setAnchorPoint(Vec2(1.0f, 0.5f));
		m_labelTime->setPosition(Vec2(RESOURCE_WIDTH - 10, RESOURCE_HEIGHT - 13.0f));
	}
	m_raceBar = DrawNode::create();
	this->addChild(m_raceBar, 5);
	m_raceGhostIcon = Label::createWithSystemFont("\xF0\x9F\x91\xBB", "Arial", 14);  // 👻
	this->addChild(m_raceGhostIcon, 6);
	m_raceGapLabel = Label::createWithSystemFont("", "Arial", 9);
	this->addChild(m_raceGapLabel, 6);
	m_raceYouIcon = Label::createWithSystemFont("\xF0\x9F\x8F\x83", "Arial", 14);    // 🏃 (항상 위)
	this->addChild(m_raceYouIcon, 7);
	m_raceYouIcon->setScaleX(-1.0f);     // 진행 방향(왼→오)을 보도록 좌우 반전
	m_raceGhostIcon->setScaleX(-1.0f);
	m_youTargetX = m_ghostTargetX = -1.0f;
	m_youVel = m_ghostVel = 0.0f;

	// 마커 스프링 이동(매 프레임, 임계감쇠) — 부드러운 가속·감속
	this->schedule([this](float dt){
		if (dt > 0.033f) dt = 0.033f;
		const float K = 180.0f, D = 26.0f;
		if (m_raceYouIcon && m_youTargetX >= 0.0f) {
			float x = m_raceYouIcon->getPositionX();
			m_youVel += ((m_youTargetX - x) * K - m_youVel * D) * dt;
			m_raceYouIcon->setPositionX(x + m_youVel * dt);
		}
		if (m_raceGhostIcon && m_ghostTargetX >= 0.0f) {
			float x = m_raceGhostIcon->getPositionX();
			m_ghostVel += ((m_ghostTargetX - x) * K - m_ghostVel * D) * dt;
			m_raceGhostIcon->setPositionX(x + m_ghostVel * dt);
		}
	}, "race_anim");

	_updateRaceHud();
}

void PlayScene::_updateRaceHud()
{
	if (!m_isRace || m_ghostReach.empty() || !m_raceBar) return;
	int R  = _movesRemaining();
	if (R < 0) R = 0; if (R > m_raceTotalMoves) R = m_raceTotalMoves;
	// 라이브: 벽시계 경과. 리플레이: 재생 클럭(배속 반영) — 고스트 마커를 리플레이 시간축에 맞춤.
	int t  = m_isReplaying ? (int)m_replayClock : (getMilliCount() - m_dateTime);

	// 고스트 골인 순간 연출 (내가 아직 플레이 중일 때)
	if (!m_ghostFinishedShown && t >= m_ghostScoreMs && m_isIng == PLAY) {
		m_ghostFinishedShown = true;
		auto f = Label::createWithSystemFont("\xF0\x9F\x91\xBB GHOST FINISHED", "Arial", 20);  // 👻
		f->setColor(Color3B(200, 200, 255));
		f->setPosition(Vec2(RESOURCE_WIDTH / 2, RESOURCE_HEIGHT / 2 + 40));
		f->setScale(0.6f); f->setOpacity(0);
		this->addChild(f, 56);
		f->runAction(Sequence::create(
			Spawn::create(ScaleTo::create(0.2f, 1.0f), FadeIn::create(0.2f), nullptr),
			DelayTime::create(0.9f), FadeOut::create(0.5f), RemoveSelf::create(), nullptr));
		SoundFactory::Instance()->play("efs_cancel_select", 0.5f);
	}

	// 트랙(정보바 내, 최대한 넓게 — 시간 침범 안 함): |끝점|━━ 델타값 ━━
	const float x0 = 18, x1 = 378, ty = RESOURCE_HEIGHT - 13.0f;
	const Color4F trackCol(0.45f, 0.45f, 0.52f, 0.85f);
	float total = (float)m_raceTotalMoves;
	int gR = m_raceTotalMoves;
	for (int r = 0; r <= m_raceTotalMoves; ++r) if (m_ghostReach[r] <= t) { gR = r; break; }
	float youX = x0 + (x1 - x0) * ((total - R)  / total);
	float ghX  = x0 + (x1 - x0) * ((total - gR) / total);
	float mx   = (youX + ghX) * 0.5f;              // 델타값 위치(두 마커 중간)

	int d  = m_ghostReach[R] - t;                  // + = 내가 더 빨리 도달 = 앞섬
	int ad = d < 0 ? -d : d;
	bool showDelta = (ad >= 3000);                 // 3초 이상 차이날 때만 델타값 표시
	if (m_raceGapLabel) {
		m_raceGapLabel->setVisible(showDelta);
		if (showDelta) {
			m_raceGapLabel->setString(StringUtils::format("%d.%01ds", ad / 1000, (ad % 1000) / 100));
			m_raceGapLabel->setColor(d >= 0 ? Color3B(90, 220, 130) : Color3B(255, 100, 100));
			m_raceGapLabel->setPosition(Vec2(mx, ty));
		}
	}
	const float gap = showDelta ? 14.0f : 0.0f;    // 델타 표시 시만 라인 끊김, 아니면 연속

	m_raceBar->clear();
	m_raceBar->drawSegment(Vec2(x0, ty - 4), Vec2(x0, ty + 4), 1.0f, trackCol);  // | 시작 끝점
	m_raceBar->drawSegment(Vec2(x1, ty - 4), Vec2(x1, ty + 4), 1.0f, trackCol);  // | 골 끝점
	if (mx - gap > x0) m_raceBar->drawSegment(Vec2(x0, ty),       Vec2(mx - gap, ty), 1.0f, trackCol);
	if (mx + gap < x1) m_raceBar->drawSegment(Vec2(mx + gap, ty), Vec2(x1, ty),       1.0f, trackCol);

	// 마커: 목표만 설정(실제 이동은 race_anim 스프링이 매 프레임). 최초는 즉시 배치.
	if (m_raceYouIcon   && m_youTargetX   < 0) m_raceYouIcon->setPosition(Vec2(youX, ty));
	if (m_raceGhostIcon && m_ghostTargetX < 0) m_raceGhostIcon->setPosition(Vec2(ghX,  ty));
	m_youTargetX   = youX;
	m_ghostTargetX = ghX;
}

void PlayScene::endReplay()
{
	this->unschedule("replay_update");
	this->unschedule("replay_fail_off");
	m_isReplaying        = false;
	m_replayPaused       = false;
	m_replaySelectedPole = -1;
	this->SelectPole(-1, false);

	// 컨트롤 바 제거 (배속 라벨은 씬에 직접 부착되므로 따로 제거)
	if (m_replaySpeedLabel) { m_replaySpeedLabel->removeFromParent(); m_replaySpeedLabel = nullptr; }
	if (m_replaySpeedMenu)  { m_replaySpeedMenu->removeFromParent();  m_replaySpeedMenu  = nullptr; }
	m_replayPauseItem   = nullptr;   // 메뉴와 함께 제거됨
	m_replaySpeedArrowL = nullptr;
	m_replaySpeedArrowR = nullptr;

	// 관전 모드: 재생 종료 → 선택 팝업(REPLAY/HOME). 바로 나가지 않음.
	if (m_isSpectate) {
		this->showSpectateEndPopup();
		return;
	}

	// 레이스 리플레이: 트랙 HUD 정리 후 결과 팝업만 복원 (레벨/티커 복원 안 함 — 레이스는 원래 숨김)
	if (m_isRace) {
		this->unschedule("race_anim");
		if (m_labelTime)     m_labelTime->setVisible(false);
		if (m_raceBar)       m_raceBar->setVisible(false);
		if (m_raceYouIcon)   m_raceYouIcon->setVisible(false);
		if (m_raceGhostIcon) m_raceGhostIcon->setVisible(false);
		if (m_raceGapLabel)  m_raceGapLabel->setVisible(false);
		auto ov = this->getChildByTag(tagPopup);
		if (ov) ov->setVisible(true);
		return;
	}

	// 타이머 숨기고 상단바(레벨/결과 텍스트) 복원
	if (m_labelTime) m_labelTime->setVisible(false);
	if (m_labelLevel) m_labelLevel->setVisible(true);
	showHudResult(m_lastIsNewRecord, getRecordTime(m_mastTime));   // 결과 텍스트 원복

	// 결과 팝업 복원 (다시보기 재요청 가능)
	auto ov = this->getChildByTag(tagPopup);
	if (ov) ov->setVisible(true);
}

void PlayScene::startSpectate()
{
	if (m_spectateStarted) return;
	m_spectateStarted = true;

	std::vector<ReplayEvent> loaded;
	int lvl = 0, finalMs = 0;
	if (!decodeReplayBlob(m_spectateBlob, loaded, lvl, finalMs) || loaded.empty()) {
		// 손상/빈 데이터 → 랭킹보드 복귀
		Director::getInstance()->replaceScene(
			TransitionFade::create(0.3f, MainScene::createScene()));
		return;
	}
	m_replay        = loaded;
	m_replayFinalMs = finalMs;

	// 진입 화면 정리 (카운트다운/아이들/티커 억제) + 좌측 도크 숨김(관전 중 메뉴 미표시)
	this->removeChildByTag(tagCountDown, true);
	stopRankTicker();
	stopIdleAnimation();
	if (auto dock = this->getChildByTag(1234)) dock->setVisible(false);

	this->_beginSpectatePlayback();
}

// 관전 재생 시작(재호출 가능 — 종료 팝업의 REPLAY 버튼에서도 호출)
void PlayScene::_beginSpectatePlayback()
{
	m_isReplaying = true;

	// 보드 초기 상태 복원 + 렌더
	this->ResetGame();
	this->DrawDiscus();
	this->SelectPole(-1, false);

	// 상단바: 관전 인디케이터(랭커 이름) + 타이머
	if (m_labelTime)  { m_labelTime->setVisible(true); m_labelTime->setString("00:00.00"); }
	if (m_labelLevel) m_labelLevel->setVisible(false);
	if (m_hudTickerLabel) {
		m_hudTickerLabel->stopAllActions();
		std::string ind = "\xE2\x96\xB6 " + (m_spectateName.empty() ? std::string("REPLAY") : m_spectateName);
		m_hudTickerLabel->setString(ind);
		m_hudTickerLabel->setColor(Color3B(120, 200, 255));
		m_hudTickerLabel->setOpacity(255);
		m_hudTickerLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
		m_hudTickerLabel->setPosition(Vec2(RESOURCE_WIDTH * 0.5f, RESOURCE_HEIGHT - 13.0f));
		m_hudTickerLabel->setVisible(true);
	}

	// 하단 컨트롤 바 ([❚❚ PAUSE][◀ SPEED xN ▶][■ STOP])
	this->_buildReplayControlBar();

	m_replayClock        = 0.0;
	m_replayIndex        = 0;
	m_replaySelectedPole = -1;
	this->schedule([this](float dt){ this->_updateReplay(dt); }, 0.0f, "replay_update");
}

// 관전 종료 팝업 — REPLAY(다시 재생) / HOME(랭킹보드 복귀)
void PlayScene::showSpectateEndPopup()
{
	const float PW = 260, PH = 152;
	// 유틸 톤 = 그레이 테두리. 타이틀 = 랭커 이름(스카이블루 강조). 헤더가 이름+LEVEL·RANK라 divider=false.
	std::string who = m_spectateName.empty() ? "REPLAY" : m_spectateName;
	auto f = makePopupFrame(who, kBorderGray, kTextAccent, PW, PH, 16.f, 170, true, false);
	this->addChild(f.backdrop, tagPopup, tagPopup);
	auto box = f.box;

	// LEVEL · RANK
	std::string lr = StringUtils::format("LEVEL %d", m_countOfDiscus);
	if (m_spectateRank > 0) lr += StringUtils::format("   \xC2\xB7   RANK %d", m_spectateRank);  // ·
	auto lrLbl = Label::createWithSystemFont(lr, "Arial", 11);
	lrLbl->setColor(kTextMuted);
	lrLbl->setPosition(Vec2(PW / 2, PH - 48));
	box->addChild(lrLbl);

	// 기록 시간 (헤드라인)
	RecordTime rt = getRecordTime(m_spectateScoreMs);
	auto timeLbl = Label::createWithSystemFont(
		StringUtils::format("%02d:%02d.%02d", rt.min, rt.sec, rt.ms), "Arial", 24);
	timeLbl->setColor(kTextData);
	timeLbl->setPosition(Vec2(PW / 2, PH - 80));
	box->addChild(timeLbl);

	// 좋아요 가능 여부 — 관전 대상이 있고 내가 아니어야(자기 좋아요 차단, replay_like)
	bool canLike = false;
#ifdef ENABLE_REPLAY_LIKE
	{
		std::string myId = LeaderboardManager::Instance()->getPlayFabId();
		canLike = !m_spectatePlayFabId.empty() && m_spectatePlayFabId != myId
		          && LeaderboardManager::Instance()->isLikeEnabled();
	}
#endif

	// 버튼 행 — 좋아요 가능하면 3열(REPLAY/👍/HOME), 아니면 2열(REPLAY/HOME)
	const float xReplay = canLike ? PW * 0.20f : PW * 0.28f;
	const float xHome   = canLike ? PW * 0.80f : PW * 0.72f;
	const float btnW    = canLike ? 76.f : 104.f;
	const float btnFont = canLike ? 12.f : 13.f;

	// 버튼: ▶ REPLAY / ⌂ HOME
	auto replayItem = makePopupChipButton("\xE2\x96\xB6 REPLAY", kBtnFunc, [this](Ref*) {
		SoundFactory::Instance()->play("efs_click");
		this->removeChildByTag(tagPopup);
		this->_beginSpectatePlayback();
	}, btnW, 34.f, btnFont);
	replayItem->setPosition(Vec2(xReplay, kPopupBtnY));

	auto homeItem = makePopupChipButton("\xE2\x8C\x82 HOME", kBtnDismiss, [](Ref*) {
		SoundFactory::Instance()->play("efs_click");
		Director::getInstance()->replaceScene(
			TransitionFade::create(0.3f, MainScene::createScene()));
	}, btnW, 34.f, btnFont);
	homeItem->setPosition(Vec2(xHome, kPopupBtnY));

	Menu* menu;
#ifdef ENABLE_REPLAY_LIKE
	if (canLike) {
		const int lv = m_countOfDiscus;          // 관전 레벨 = 디스크 수
		const std::string target = m_spectatePlayFabId;

		// 콜백은 자기 버튼 포인터가 필요하므로 빈 콜백으로 만들고 setCallback로 지정
		auto likeItem = makePopupChipButton("\xF0\x9F\x91\x8D LIKE", kBtnFunc,
			[](Ref*) {}, btnW, 34.f, btnFont);
		likeItem->setPosition(Vec2(PW * 0.50f, kPopupBtnY));

		auto alive      = m_aliveFlag;
		auto submitting = std::make_shared<bool>(false);
		likeItem->setCallback([this, alive, lv, target, likeItem, submitting](Ref*) {
			if (*submitting) return;
			*submitting = true;
			likeItem->setEnabled(false);
			SoundFactory::Instance()->play("efs_click");
			LeaderboardManager::Instance()->likeReplay(lv, target,
				[this, alive, likeItem, submitting](bool ok, int n, bool already) {
					*submitting = false;
					if (!alive || !*alive) return;
					if (!this->getChildByTag(tagPopup)) return;   // 팝업 닫힘 → likeItem 무효
					auto lbl = dynamic_cast<Label*>(likeItem->getChildByTag(kChipLabelTag));
					if (ok) {
						// 성공/이미함 모두 카운트 반영 + 버튼 잠금(재탭 방지)
						if (lbl) lbl->setString(StringUtils::format(
							"\xF0\x9F\x91\x8D %s", formatLikeCount(n).c_str()));
						likeItem->setEnabled(false);
						if (!already) {
							SoundFactory::Instance()->play("efs_move_disc_ok");
							likeItem->runAction(Sequence::create(
								ScaleTo::create(0.08f, 1.18f),
								ScaleTo::create(0.10f, 1.0f), nullptr));
						}
					} else {
						likeItem->setEnabled(true);   // 실패 → 재시도 허용
					}
				});
		});

		// 현재 좋아요 수 프리로드 → 있으면 "👍 N" 표시(아직 안 누른 상태)
		auto aliveP = m_aliveFlag;
		LeaderboardManager::Instance()->fetchLikes(lv,
			[this, aliveP, likeItem, target](const std::map<std::string, int>& m) {
				if (!aliveP || !*aliveP) return;
				if (!this->getChildByTag(tagPopup)) return;
				auto it = m.find(target);
				if (it == m.end() || it->second <= 0) return;
				auto lbl = dynamic_cast<Label*>(likeItem->getChildByTag(kChipLabelTag));
				if (lbl) lbl->setString(StringUtils::format(
					"\xF0\x9F\x91\x8D %s", formatLikeCount(it->second).c_str()));
			});

		menu = Menu::create(replayItem, likeItem, homeItem, nullptr);
	} else
#endif // ENABLE_REPLAY_LIKE
	{
		menu = Menu::create(replayItem, homeItem, nullptr);
	}
	menu->setPosition(Vec2::ZERO);
	box->addChild(menu, 5);
}
