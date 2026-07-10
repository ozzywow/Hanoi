# 리플레이 & 고스트 대결 기획안

> Tower of Hanoi Olympic — 터치노트 기록/재생 및 랭커 고스트 대결 기능
> 작성일: 2026-07-09 · 상태: 기획 (구현 전)

---

## 1. 개요

플레이어의 디스크 이동 시퀀스를 **타임스탬프와 함께 기록**하여, 판을 그대로 **리플레이 재생**한다. 나아가 랭커의 기록을 PlayFab에 공개 저장하여 다른 플레이어가 **관전**하거나 **고스트로 대결**할 수 있게 확장한다.

기록 갱신(타임어택) 게임이므로 **타이밍 재현이 설계의 중심**이다. 리플레이는 리더보드 기록과 바이트 단위로 정합해야 하며, 이 정합성 검증은 그대로 **부정 점수 방지(anti-cheat)** 수단이 된다.

### 목표
- **관전 가치**: 랭커의 리듬(망설임/폭발 구간)이 그대로 보이는 실시간 재생
- **경쟁 유발**: 랭커 고스트와 같은 시간축에서 대결 → 리텐션 강화
- **무결성**: 리플레이 = 기록의 증명서. 서버에서 합법성 + 시간 정합 검증

### 비목표 (이번 범위 아님)
- 마우스/터치 궤적 좌표 저장 (하노이는 from/to만으로 이동이 100% 결정됨 → 불필요)
- 15디스크 이상 대용량 리플레이 (2차 과제: 청킹/압축)

---

## 2. 핵심 설계 원칙 — 절대 타임스탬프

각 이동을 **시작 시점(`m_dateTime`) 기준 절대 경과 ms**로 저장한다. delta(간격) 방식이 아니다.

**이유:**
1. **드리프트 0** — delta 누적 시 반올림 오차가 쌓여 `리플레이 합계 ≠ 리더보드 기록`이 될 수 있음. 절대값은 마지막 이동의 `t_ms == m_mastTime`이 항상 성립.
2. **오버플로 없음** — delta uint16(최대 65초)는 플레이어가 멈추면 초과. 절대 ms(uint24/32)는 안전.
3. **재생 스케줄 그대로 사용** — `playbackClock`이 t_ms에 도달할 때 이동 실행. 별도 변환 불필요.
4. **검증 용이** — 마지막 t_ms를 제출 스코어와 직접 비교 가능.

delta가 필요하면 재생 시 `t[i] - t[i-1]`로 즉석 계산한다.

---

## 3. 데이터 포맷

### 3.1 이벤트 레코드 (터치노트)

단순 "이동"이 아니라 플레이어의 **의사결정 3종**을 기록한다. 이래야 "어디서 망설이고 어디서 실수했는지" 플레이 패턴이 재생에 드러난다.

```cpp
enum ReplayEventType : uint8_t {
    EV_SELECT    = 0,  // 폴 선택, 라이트 ON        (efs_select_disc)
    EV_MOVE_OK   = 1,  // 성공 이동                 (efs_move_disc_ok)
    EV_MOVE_FAIL = 2,  // 이동 실패, 라이트 OFF+실패음 (efs_cancel_select)
    EV_DESELECT  = 3,  // 같은 폴 재탭=선택 취소      (efs_move_disc_ok, 이동 없음)
};

struct ReplayEvent {
    uint8_t  packed;  // (type << 2) | pole   — type 2bit, pole 2bit(0~2)
    uint32_t t_ms;    // m_dateTime 기준 경과 ms (직렬화 시 uint24 → 이벤트당 4바이트)
};
```

- **출발 폴은 저장 안 함** — `EV_MOVE_OK`의 출발 폴 = 직전 `EV_SELECT`의 pole. 목적 폴만 저장.
- 디스크 ID도 저장 안 함 — selectedPole의 top이 곧 이동 대상.
- `t_ms` = `getMilliCount() - m_dateTime`. **`SELECT.t_ms` → `MOVE_OK.t_ms` 간격 = 망설인 시간**(핵심 관전 포인트).
- 애니메이션 데이터 없음 — 재생은 라이트/즉시 위치변경/효과음으로 재현 (§6.5, 들어올림 애니 불필요).

### 3.2 리플레이 헤더

```cpp
struct ReplayHeader {
    uint16_t version;      // 포맷 버전 (하위호환)
    uint8_t  discusCount;  // 디스크 수 (레벨)
    uint8_t  targetPole;   // 완성 목표 폴
    uint32_t finalTimeMs;  // == m_mastTime (== 마지막 MOVE_OK 의 t_ms)
    uint16_t eventCount;   // 이벤트 수 (선택+성공+실패 합)
    // 이후 eventCount 개의 ReplayEvent
};
```

### 3.3 직렬화 / 전송 포맷
- 바이너리 → **Base64** 문자열로 PlayFab 저장 (JSON/UserData 값은 문자열)
- 반복 패턴 많으므로 필요 시 RLE/varint로 추가 압축 (2차)

---

## 4. 크기 분석

3종 이벤트 모델은 이동당 최소 2이벤트(SELECT+MOVE_OK) + 실패/재선택이 붙어 **이동수의 약 2~3배 이벤트**가 발생. 이벤트당 4바이트(§3.1).

| 디스크 | 랭커 추정 이동 | 추정 이벤트(×2.5) | 원시(4B/ev) | Base64 | PlayFab UserData(1000자) |
|:---:|:---:|:---:|:---:|:---:|:---:|
| 5 | ~40 | ~100 | ~400B | ~540B | ✅ 여유 |
| 7 | ~160 | ~400 | ~1.6KB | ~2.1KB | ⚠️ 청크 분할 |
| 10 | ~1300 | ~3200 | ~12.8KB | ~17KB | ⚠️ 청크 분할 |
| 15 | ~40000 | ~100000 | ~400KB | ~530KB | ❌ 별도 저장소 |

- **1차(로컬)는 크기 무관** — 메모리에만 있음.
- 2차(네트워크)부터 의미: 5디스크 여유, 7디스크+는 UserData 청킹 또는 압축 필요.
- 15디스크는 범위 밖 (2차 이후).

---

## 5. 저장 / 전송 (PlayFab)

### 5.1 저장 위치
- 리더보드 엔트리에는 직접 첨부 불가 → **플레이어별 UserData**에 저장
- 키 예시: `replay_L05` = Base64 리플레이 (레벨별 자기 최고기록 1건)
- 큰 리플레이는 `replay_L10_0`, `replay_L10_1` … 청크 분할

### 5.2 공개 조회 (랭커 리플레이 관전) — 진입점 결정됨
- **진입점**: 랭킹보드에서 랭커 클릭 → **수상소감 카드 팝업**([MainScene::showAwardCardDialog](../Classes/MainScene.cpp#L1540))에 **`▶ WATCH` 버튼** 추가. 이 팝업은 이미 `e.playFabId` + `level`을 보유 → 조인 키 확보 완료. 하단 EDIT/Report 버튼 옆에 형제 버튼으로 배치(2버튼 행 or DH 확장).
- `GetUserData`는 기본 본인만 → **CloudScript(Server API)로 대상 랭커 조회**. `award_comment`의 **playFabId 조인 패턴 재활용** (`[[project_award_comment]]`).
- 흐름: 랭커 카드 → `▶ WATCH` → CloudScript로 해당 유저 `replay_Lxx` fetch(로딩 표시) → **PlayScene을 관전 모드로 실행**(외부 리플레이 주입 + 즉시 재생). ⭐ PlayScene 관전 실행 경로가 2차 실질 신규 작업(재생 엔진 자체는 1차 완성).
- **조건부 노출**: 랭커가 해당 레벨 리플레이를 가진 경우만 버튼 표시 → 리더보드 엔트리에 `hasReplay` 플래그 필요.
- 라벨 구분: 내 결과팝업=`▶ REPLAY`(로컬), 랭커 카드=`▶ WATCH`(관전).

### 5.3 업로드 조건 — 결정됨 (D9)
- **레벨 ≤ 5 AND Top10 달성** 일 때만 PlayFab 업로드. (크기·비용 최소화)
- **소스는 로컬 UserDefault `replay_Lx`** — 1차 영속화에서 이미 최고기록 리플레이가 로컬 저장됨. Top10 확정 시점([MainScene checkAndPromptAward](../Classes/MainScene.cpp#L408), award_comment와 동일 경로)에서 `level<=5`면 로컬 blob을 읽어 PlayFab UserData에 업로드.
- 결과 팝업의 "방금 플레이 Replay"는 업로드와 무관 — 항상 **로컬 메모리/UserDefault에서** 재생 (네트워크 0).

---

## 6. 코드 통합 지점

### 6.1 녹화 (Recording)

**⚠️ 중요 — 커밋 경로가 2개다.** 탭-탭은 [onTouchBegan](../Classes/PlaySceneTouchHandlerLayer.cpp#L118)에서, 드래그-드롭은 [onTouchEnded](../Classes/PlaySceneTouchHandlerLayer.cpp#L220)에서 이동이 확정된다. 성공/실패 이벤트는 각각 **두 군데**서 발생하므로 훅이 둘 다 잡아야 함.

- PlayScene에 헬퍼 하나 추가: `void recordReplayEvent(ReplayEventType t, int pole)` → `m_replay.push_back({(uint8_t)((t<<2)|pole), (uint32_t)(getMilliCount()-m_dateTime)});` (재생 중이면 무시)
- 훅 위치 (총 5개 호출, 각 1줄):

| 이벤트 | 호출 지점 |
|---|---|
| `EV_SELECT` | onTouchBegan 선택 성사 [handler:147](../Classes/PlaySceneTouchHandlerLayer.cpp#L147) (`efs_select_disc` 옆) |
| `EV_MOVE_OK` | **AttachDiscusToPole 내부** `++m_moveCount` 뒤 [PlayScene:1015](../Classes/PlayScene.cpp#L1015) — 탭/드래그 양경로가 여길 통과하므로 성공은 여기 1곳이면 충분. 목적폴 = `poleID` |
| `EV_MOVE_FAIL` | onTouchBegan 실패 [handler:131](../Classes/PlaySceneTouchHandlerLayer.cpp#L131) **및** onTouchEnded 실패 [handler:233](../Classes/PlaySceneTouchHandlerLayer.cpp#L233) (`efs_cancel_select` 옆, 2곳) |
| `EV_DESELECT` | **AttachDiscusToPole 내부** `currPoleID==poleID` [early return 직전](../Classes/PlayScene.cpp#L1000) — 같은 폴 재탭의 유일 통과점. pole = `poleID` |

  → 성공/취소는 `AttachDiscusToPole` 단일 통과점에서 분기(양 경로 커버): `++m_moveCount` 도달=MOVE_OK, `currPoleID==poleID` early-return=DESELECT. 선택/실패는 핸들러에서 직접 기록.
- 게임 시작 시 초기화: [Start()](../Classes/PlayScene.cpp#L539) `m_dateTime` 설정 지점에서 `m_replay.clear()`
- 완료 시 헤더 채우기: [Finished()](../Classes/PlayScene.cpp#L560) — `finalTimeMs = m_mastTime`, `targetPole` 확정

### 6.2 재생 엔진 (Playback) — 신규
게임 로직을 재실행하지 않고 **이벤트 상태기계**를 재생한다.
```cpp
// 매 프레임 (schedule)
playbackClock += dt * playbackSpeed;      // dt = Director::getDeltaTime()
while (i < events.size() && events[i].t_ms <= playbackClock) {
    applyReplayEvent(events[i]);
    ++i;
}
m_labelTime->setString(formatTime(playbackClock));  // getRecordTime 재사용

// applyReplayEvent — 재생 상태(m_replaySelectedPole) 기반
switch (type) {
  case EV_SELECT:    m_replaySelectedPole = pole;
                     SelectPole(pole, true);                 // 라이트 ON (기존 함수)
                     SoundFactory::play("efs_select_disc"); break;
  case EV_MOVE_OK:   /* m_pole[selected] top → m_pole[pole] */
                     redrawPoles();                          // 즉시 위치갱신(판정 없음)
                     SelectPole(-1, false);                  // 라이트 OFF
                     SoundFactory::play("efs_move_disc_ok");
                     m_replaySelectedPole = -1; break;
  case EV_MOVE_FAIL: SelectPole(pole, false);                // 실패 마크(불 꺼짐 연출)
                     SoundFactory::play("efs_cancel_select");
                     scheduleOnce(SelectPole(-1,false), 0.2);
                     m_replaySelectedPole = -1; break;
  case EV_DESELECT:  SelectPole(-1, false);                  // 선택 취소, 라이트 OFF
                     SoundFactory::play("efs_move_disc_ok");
                     m_replaySelectedPole = -1; break;
}
```
- 타이머 표시: [getRecordTime()](../Classes/common_define.h#L156) 그대로 재사용 → 리더보드와 동일 포맷
- 마지막 `EV_MOVE_OK`에서 `playbackClock → m_mastTime` 정확히 수렴
- **원본 효과음을 타이밍대로 재생** → 실제 플레이처럼 느껴짐 (몰입감)
- 재생 컨트롤(선택, D5): 일시정지 / 배속(`playbackSpeed`) / 특정 이벤트 점프

### 6.3 상태 격리
- 재생 모드에서는 터치 입력 비활성(`m_touchHanderLayer` 무시), RPM/이퀄라이저는 t_ms 간격 기반으로 재구동 가능 (`[[project_rpm_equalizer]]`)

---

## 6.5 ⭐ 1차 구현 노트 (오늘 착수 대상)

> 코드 확인 결과 발견한 함정과 재사용 자산. 1차(로컬 내 리플레이)에 한정.

### 상호작용 모델 (녹화 대상)
투탭 **and** 드래그-드롭 둘 다 지원. 3종 이벤트가 [onTouchBegan](../Classes/PlaySceneTouchHandlerLayer.cpp#L58)/[onTouchEnded](../Classes/PlaySceneTouchHandlerLayer.cpp#L197)에서 갈림:
- **선택**: 폴 탭 → 라이트 ON, `efs_select_disc` ([handler:146](../Classes/PlaySceneTouchHandlerLayer.cpp#L146))
- **성공**: 유효 폴 탭/드롭 → `AttachDiscusToPole` + `efs_move_disc_ok` ([handler:125](../Classes/PlaySceneTouchHandlerLayer.cpp#L125), [226](../Classes/PlaySceneTouchHandlerLayer.cpp#L226))
- **실패**: 무효 폴 탭/드롭 → 라이트 OFF + `efs_cancel_select` ([handler:131](../Classes/PlaySceneTouchHandlerLayer.cpp#L131), [233](../Classes/PlaySceneTouchHandlerLayer.cpp#L233))
- ⚠️ 성공/실패는 **탭 경로(onTouchBegan)와 드래그 경로(onTouchEnded) 2곳**에서 발생 → §6.1 훅 배치 참고.
- onTouchMoved 드래그 중 라이트 색 변화는 **녹화 안 함** (discrete 결정만 기록).

### 함정 — 라이브 함수 재사용 주의
1. **`DrawDiscus()`의 완료판정** — 끝에서 `CheckSuccess() → Finished()` 호출 ([DrawDiscus:852](../Classes/PlayScene.cpp#L852)). 재생에 그대로 쓰면 결과 팝업 재발생. → **가드 플래그 `m_isReplaying`** 추가해 `if(!m_isReplaying && CheckSuccess()) Finished();`로 바꾸면 재생에서도 안전하게 재사용 가능. (또는 위치갱신만 하는 `redrawPoles()` 분리)
2. **`AttachDiscusToPole()`를 재생에 쓰지 말 것** — 녹화 훅 + `m_moveCount++`까지 태움 ([1015](../Classes/PlayScene.cpp#L1015)). 재생은 `applyReplayEvent`(§6.2)로 `m_pole` 벡터만 직접 조작.
3. **재생 타이머 = 실시간 클럭 분리** — 라이브는 `m_actionTimeRun`(RepeatForever→`DrawTime`, `getMilliCount()` 실시간) ([554](../Classes/PlayScene.cpp#L554), [DrawTime:711](../Classes/PlayScene.cpp#L711)). 재생 진입 시 이 액션 **정지**하고 `playbackClock += dt` update로 교체.

### 재사용 자산 — 애니 제거로 신규 코딩 대폭 감소 ✅
들어올림/이동 애니 불필요(즉시 위치변경이 더 역동적). 재생이 **기존 함수를 그대로** 씀:
- **라이트 ON/OFF**: [SelectPole()](../Classes/PlayScene.cpp#L1024) 재사용 (재생 중 `m_isIng!=PLAY`이라 RPM 부작용 없음).
- **위치 갱신**: 가드 처리한 `DrawDiscus`/`redrawPoles`.
- **효과음**: `efs_select_disc`/`efs_move_disc_ok`/`efs_cancel_select` 원본 재생.
- **초기상태 복원**: [ResetGame():805](../Classes/PlayScene.cpp#L805) 전 디스크 폴 0 재배치.
- **타이머 포맷**: [getRecordTime](../Classes/common_define.h#L156). **기준점**: `m_dateTime` 카운트다운 이후([539](../Classes/PlayScene.cpp#L539)) → t_ms 깔끔.

### 상태 & 수명
- 재생 가드 플래그 `m_isReplaying` (+ 필요 시 `m_replaySelectedPole`). 재생 중 입력 차단(`m_touchHanderLayer` 무시) + 완료판정 억제.
- `m_replay`(이벤트 벡터)는 신판 시작([Start()](../Classes/PlayScene.cpp#L539))에서 `clear()`. `ResetGame()`은 `m_replay` 안 건드림 → 재생용 스냅샷 보존.

### 1차 진입점 (D7 = 결과 팝업 버튼, 결정됨)
- 결과 팝업([showHudResult ~2074](../Classes/PlayScene.cpp#L2074))에 **"리플레이 보기" 버튼** 추가. 재생 종료 후 팝업 복귀.

### 1차 최소 구현 체크리스트
- [ ] `ReplayEvent`/`m_replay` 멤버 + `recordReplayEvent()` 헬퍼
- [ ] 녹화 훅 6곳 (§6.1): SELECT×1, MOVE_OK×1·DESELECT×1(Attach 내부 분기), FAIL×2, + Start에서 clear
- [ ] `m_isReplaying` 가드 → `DrawDiscus` 완료판정 우회
- [ ] `applyReplayEvent()` 상태기계 (선택/성공/실패, §6.2)
- [ ] 재생 update 루프 (`playbackClock`, 타이머 라벨)
- [ ] 재생 진입: `m_actionTimeRun` 정지 + `ResetGame()` 초기화 + 입력 차단
- [ ] 결과 팝업 "리플레이 보기" 버튼 + 복귀 흐름

---

## 7. Anti-cheat 검증 (서버측, CloudScript)

리플레이 저장은 **기록 무결성 검증**을 겸한다. 스코어 제출 시 리플레이도 함께 보내 서버에서 검사:

1. **합법성**: 모든 이동이 하노이 규칙 준수 (큰 디스크가 작은 디스크 위에 못 감)
2. **완성**: 최종 상태가 `targetPole`에 전체 디스크 정렬
3. **시간 정합**: `마지막 move.t_ms == 제출 스코어(finalTimeMs)`
4. (선택) **하한**: 물리적으로 불가능한 초고속 이동 간격 필터

3개(+선택 4) 통과 시에만 리더보드 반영 → 가짜 API 스코어 차단. `BestTime_Lxx` 통계 집계 규칙은 변경 금지 (`[[project_record_expiry_playfab_safety]]`).

---

## 8. 단계별 로드맵 (3차 구조)

서버 검증(§7 anti-cheat)은 별도 단계가 아니라 각 차수에 녹아든다: 1차 불필요 → 2차 합법성 검증 시작 → 3차 순위가 바뀌므로 필수.

| 차수 | 기능 | 신규 작업 | 네트워크 | 검증 |
|:---:|:---|:---|:---:|:---:|
| **1차** ⭐ | **내 리플레이 보기** (최소 출시 단위) | 녹화 훅 + 재생 엔진 | 없음 | 불필요 |
| **2차** | **랭킹보드 연동 — 다른 유저 리플레이 공유/관전** | PlayFab 저장/조회 + CloudScript 조인 | 필요 | 합법성 검증 |
| **3차** | **고스트 대결 — 이기면 랭킹 빼앗기** | 오버레이 렌더 + 실시간 비교 HUD + 순위 교체 로직 | 필요 | **필수** |

**포맷을 한 번 확정하면 1→3차가 전부 같은 자료구조 위에 얹힌다.** 1차(로컬)만 제대로 만들면 나머지는 UI/네트워크 작업이지 재설계가 아님.

### 2차 — 랭킹보드 연동 (공유/관전)
- 랭킹보드에서 항목 선택 → playFabId로 해당 유저 `replay_Lxx` 조회 → 재생
- 남의 리플레이가 공개되므로 조회 시 **합법성(하노이 규칙) 검증** 시작

### 3차 — 고스트 대결 & 랭킹 쟁탈 상세
- 화면: 내 3폴 + 고스트 3폴(반투명 오버레이 또는 상단 미니뷰)
- 내 실제 경과시간에 맞춰 고스트 t_ms 재생 → "지금 랭커는 여기까지 왔다" 실시간 표시
- 같은 시점 비교(이동수/시간) → 앞섬/뒤처짐 HUD (레이싱 고스트 패턴 동일)
- **랭킹 쟁탈**: 고스트(대상 랭커)의 기록을 이기면 내 신기록으로 순위 교체
  - ⚠️ 순위가 실제로 바뀌므로 **서버 검증 필수** — 조작 리플레이/스코어로 랭킹 강탈 차단 (§7 3검증)
  - 결정 필요: 특정 랭커 지목 대결인지, 바로 위 순위 자동 매칭인지 (§9 D6)

---

## 9. 미해결 결정사항 / 리스크

| # | 항목 | 선택지 | 비고 |
|:---:|:---|:---|:---|
| D1 | ✅ 주 시나리오 우선순위 | **내 판 다시보기 (결정)** | 1단계=최소 출시 단위, 네트워크 0 |
| D2 | t_ms 직렬화 폭 | uint24(4.6시간) vs uint32 | 크기 vs 단순성 |
| D3 | 7~10디스크 저장 | UserData 청킹 vs 압축(RLE) vs 조건부 미지원 | 초기엔 5디스크만? |
| D4 | 검증 시점 | 클라 제출 시 필수 vs 관전 요청 시 지연 검증 | CloudScript 비용 |
| D5 | 재생 컨트롤 범위 | 단순 재생만 vs 배속/일시정지/점프 | 1차 스코프 |
| D6 | 3차 대결 상대 매칭 | 특정 랭커 지목 vs 바로 위 순위 자동 매칭 | 랭킹 쟁탈 UX |
| D7 | ✅ 1차 재생 진입점 | **결과 팝업 "리플레이 보기" 버튼 (결정)** | showHudResult에 추가 |
| D8 | ✅ 같은 폴 재탭(취소) 기록 | **EV_DESELECT 신설 (결정)** | Attach early-return에 훅 |
| D9 | ✅ 2차 업로드 조건 | **레벨≤5 AND Top10 (결정)** | 소스=로컬 replay_Lx, 진입점=수상소감 카드 ▶WATCH |
| R1 | 폭발 구간 애니메이션 추월 | 연출/진실 분리로 완화 | §6.2 |
| R3 | 3차 랭킹 강탈 조작 | 서버 검증 필수(§7)로 차단 | 순위 실변동 |
| R2 | 포맷 버전 하위호환 | 헤더 version 필드로 관리 | §3.2 |

---

## 부록 — 참고 코드 위치

| 항목 | 위치 |
|:---|:---|
| 이동 확정(녹화 훅) | [AttachDiscusToPole](../Classes/PlayScene.cpp#L992) |
| 타이머 시작 | [PlayScene.cpp:539](../Classes/PlayScene.cpp#L539) `m_dateTime` |
| 기록 확정/제출 | [PlayScene.cpp:581-621](../Classes/PlayScene.cpp#L581) |
| 완료 처리 | [Finished()](../Classes/PlayScene.cpp#L560) |
| 시간 포맷 유틸 | [getRecordTime](../Classes/common_define.h#L156) |
| 폴 상태 | `m_pole` (`vector<vector<Discus*>>`), [PlayScene.h:27](../Classes/PlayScene.h#L27) |
